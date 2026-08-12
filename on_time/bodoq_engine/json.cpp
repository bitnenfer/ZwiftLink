#include "json.h"
#include <string>

// Implement custom atoi
#include <stdlib.h>

// Tokens
enum class JSONTokenType
{
    TOKEN_FLOAT,
    TOKEN_INT,
    TOKEN_STRING,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NULL,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACK,
    TOKEN_RBRACK,
    TOKEN_COMMA,
    TOKEN_COLON
};
struct JSONToken
{
    JSONTokenType type;
    union
    {
        struct
        {
            const char* pBuffer;
            int32_t length;
        } stringData;
        float floatData;
        int32_t intData;
    } data;
    int32_t line;
    int32_t index;
};

bool ParseJSONNull(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex);
bool ParseJSONBool(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex);
bool ParseJSONNumber(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex);
bool ParseJSONString(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex);
bool ParseJSONKeyValuePair(JsonKeyValuePair* pJSONKeyValuePair, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex);
bool ParseJSONObject(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex);
bool ParseJSONArray(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex);
bool ParseJSONValue(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex);
void FreeJSONData(JsonValue* pJSONValue);

bool ParseJSONNull(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex)
{
    int32_t tokenIndex = *pCurrentTokenIndex;
    JSONToken* pToken = &pTokenStream->operator[](tokenIndex);
    if (pToken->type == JSONTokenType::TOKEN_NULL)
    {
        pJSONValue->type = JsonValueType::VALUE_NULL;
        tokenIndex += 1;
        *pCurrentTokenIndex = tokenIndex;
        return true;
    }
    return false;
}

bool ParseJSONBool(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex)
{
    int32_t tokenIndex = *pCurrentTokenIndex;
    JSONToken* pToken = &pTokenStream->operator[](tokenIndex);
    if (pToken->type == JSONTokenType::TOKEN_TRUE ||
        pToken->type == JSONTokenType::TOKEN_FALSE)
    {
        pJSONValue->type = JsonValueType::VALUE_BOOL;
        pJSONValue->boolean = (pToken->type == JSONTokenType::TOKEN_TRUE);
        tokenIndex += 1;
        *pCurrentTokenIndex = tokenIndex;
        return true;
    }
    return false;
}

bool ParseJSONNumber(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex)
{
    int32_t tokenIndex = *pCurrentTokenIndex;
    JSONToken* pToken = &pTokenStream->operator[](tokenIndex);
    if (pToken->type == JSONTokenType::TOKEN_INT ||
        pToken->type == JSONTokenType::TOKEN_FLOAT)
    {
        pJSONValue->type = JsonValueType::VALUE_NUMBER;

        if (pToken->type == JSONTokenType::TOKEN_INT)
        {
            pJSONValue->number.type = JsonNumberType::NUMBER_INT;
            pJSONValue->number.intData = pToken->data.intData;
        }
        else if (pToken->type == JSONTokenType::TOKEN_FLOAT)
        {
            pJSONValue->number.type = JsonNumberType::NUMBER_FLOAT;
            pJSONValue->number.floatData = pToken->data.floatData;
        }
        tokenIndex += 1;
        *pCurrentTokenIndex = tokenIndex;
        return true;
    }
    return false;
}

bool ParseJSONString(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex)
{
    int32_t tokenIndex = *pCurrentTokenIndex;
    JSONToken* pToken = &pTokenStream->operator[](tokenIndex);
    if (pToken->type == JSONTokenType::TOKEN_STRING)
    {
        pJSONValue->type = JsonValueType::VALUE_STRING;
        pJSONValue->string.pBuffer = (char*)malloc(pToken->data.stringData.length + 1);
        pJSONValue->string.length = pToken->data.stringData.length;
        pJSONValue->string.pBuffer[pJSONValue->string.length] = 0;
        memcpy(pJSONValue->string.pBuffer, pToken->data.stringData.pBuffer, pToken->data.stringData.length);
        tokenIndex += 1;
        *pCurrentTokenIndex = tokenIndex;
        return true;
    }
    return false;
}

bool ParseJSONKeyValuePair(JsonKeyValuePair* pJSONKeyValuePair, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex)
{
    // Key value pair is TOKNE_STRING TOKEN_COLOR TOKEN_*
    int32_t tokenIndex = *pCurrentTokenIndex;
    JSONToken* pToken = &pTokenStream->operator[](tokenIndex);
    if (pToken->type == JSONTokenType::TOKEN_STRING)
    {
        ParseJSONString(&pJSONKeyValuePair->key, pTokenStream, &tokenIndex);
        pToken = &pTokenStream->operator[](tokenIndex);
        if (pToken->type != JSONTokenType::TOKEN_COLON)
        {
            DBG_PANIC("Missing colon (:) token");
            return false;
        }
        tokenIndex += 1;
        ParseJSONValue(&pJSONKeyValuePair->value, pTokenStream, &tokenIndex);
        *pCurrentTokenIndex = tokenIndex;
        return true;
    }
    return false;
}

bool ParseJSONObject(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex)
{
    int32_t tokenIndex = *pCurrentTokenIndex;
    JSONToken* pToken = &pTokenStream->operator[](tokenIndex);
    if (pToken->type == JSONTokenType::TOKEN_LBRACE)
    {
        JsonObject* pJSONObject = (JsonObject*)malloc(sizeof(JsonObject));
        // Call constructor. Fucking C++.
        TArray<JsonKeyValuePair>* pKeyValuePairArray = &pJSONObject->data;
        new(pKeyValuePairArray) TArray<JsonKeyValuePair>();
        tokenIndex += 1;
        int32_t count = 0;

        bool result = false;
        do
        {
            JsonKeyValuePair keyValuePair;
            result = ParseJSONKeyValuePair(&keyValuePair, pTokenStream, &tokenIndex);
            if (result)
            {
                pKeyValuePairArray->add(keyValuePair);
                pToken = &pTokenStream->operator[](tokenIndex);
                if (pToken->type == JSONTokenType::TOKEN_COMMA)
                {
                    tokenIndex += 1;
                    count += 1;
                    continue;
                }
                else
                {
                    result = false;
                }
            }
        } while (result);
        pJSONObject->count = (int32_t)pKeyValuePairArray->getNum();

        pToken = &pTokenStream->operator[](tokenIndex);
        if (pToken->type != JSONTokenType::TOKEN_RBRACE)
        {
            DBG_PANIC("Missing }");
            return false;
        }
        tokenIndex += 1;
        *pCurrentTokenIndex = tokenIndex;
        pJSONValue->type = JsonValueType::VALUE_OBJECT;
        pJSONValue->pObject = pJSONObject;
        return true;
    }
    return false;
}

bool ParseJSONArray(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex)
{
    int32_t tokenIndex = *pCurrentTokenIndex;
    JSONToken* pToken = &pTokenStream->operator[](tokenIndex);
    if (pToken->type == JSONTokenType::TOKEN_LBRACK)
    {
        JsonArray* pJSONArray = (JsonArray*)malloc(sizeof(JsonArray));
        pJSONArray->type = JsonArrayType::ARRAY_ANY;

        int32_t arraySize = 0;
        int32_t testTokenIndex = tokenIndex + 1;
        JSONToken* pTestToken = &pTokenStream->operator[](testTokenIndex);

        if (pTestToken->type == JSONTokenType::TOKEN_INT)
        {
            bool isInt = true;
            while (pTestToken->type == JSONTokenType::TOKEN_INT)
            {
                testTokenIndex += 1;
                arraySize += 1;
                pTestToken = &pTokenStream->operator[](testTokenIndex);
                if (pTestToken->type == JSONTokenType::TOKEN_COMMA)
                {
                    testTokenIndex += 1;
                    pTestToken = &pTokenStream->operator[](testTokenIndex);
                }
                else if (pTestToken->type == JSONTokenType::TOKEN_RBRACK)
                {
                    break;
                }
                if (pTestToken->type != JSONTokenType::TOKEN_INT)
                {
                    isInt = false;
                    break;
                }
            }
            if (isInt)
            {
                pJSONArray->type = JsonArrayType::ARRAY_INT;
            }
        }
        else if (pTestToken->type == JSONTokenType::TOKEN_FLOAT)
        {
            bool isFloat = true;
            while (pTestToken->type == JSONTokenType::TOKEN_FLOAT)
            {
                testTokenIndex += 1;
                arraySize += 1;
                pTestToken = &pTokenStream->operator[](testTokenIndex);
                if (pTestToken->type == JSONTokenType::TOKEN_COMMA)
                {
                    testTokenIndex += 1;
                    pTestToken = &pTokenStream->operator[](testTokenIndex);
                }
                else if (pTestToken->type == JSONTokenType::TOKEN_RBRACK)
                {
                    break;
                }
                if (pTestToken->type != JSONTokenType::TOKEN_FLOAT)
                {
                    isFloat = false;
                    break;
                }
            }
            if (isFloat)
            {
                pJSONArray->type = JsonArrayType::ARRAY_FLOAT;
            }
        }

        if (pJSONArray->type == JsonArrayType::ARRAY_ANY)
        {
            TArray<JsonValue>* pValueArray = &pJSONArray->anyArray;
            new(pValueArray) TArray<JsonValue>();
            tokenIndex += 1;

            // Parse TArray<JSONValue> (this means it can't be optimized)
            bool result = false;
            do
            {
                JsonValue value;
                result = ParseJSONValue(&value, pTokenStream, &tokenIndex);
                if (result)
                {
                    pValueArray->add(value);
                    pToken = &pTokenStream->operator[](tokenIndex);
                    if (pToken->type == JSONTokenType::TOKEN_COMMA)
                    {
                        tokenIndex += 1;
                        continue;
                    }
                    else
                    {
                        result = false;
                    }
                }
            } while (result);
            pJSONArray->count = (int32_t)pValueArray->getNum();
        }
        else if (pJSONArray->type == JsonArrayType::ARRAY_INT)
        {
            TArray<int32_t>* pValueArray = &pJSONArray->intArray;
            new(pValueArray) TArray<int32_t>();
            tokenIndex += 1;

            pValueArray->resize(arraySize);

            // Parse TArray<int32_t> (this means it can't be optimized)
            bool result = true;
            do
            {
                JSONToken* pToken = &pTokenStream->operator[](tokenIndex);
                pValueArray->add(pToken->data.intData);
                tokenIndex += 1;
                pToken = &pTokenStream->operator[](tokenIndex);
                if (pToken->type == JSONTokenType::TOKEN_COMMA)
                {
                    tokenIndex += 1;
                    continue;
                }
                else
                {
                    result = false;
                }
            } while (result);
            pJSONArray->count = (int32_t)pValueArray->getNum();
        }
        else if (pJSONArray->type == JsonArrayType::ARRAY_FLOAT)
        {
            TArray<float>* pValueArray = &pJSONArray->floatArray;
            new(pValueArray) TArray<float>();
            tokenIndex += 1;

            pValueArray->resize(arraySize);

            // Parse TArray<float> (this means it can't be optimized)
            bool result = true;
            do
            {
                JSONToken* pToken = &pTokenStream->operator[](tokenIndex);
                pValueArray->add(pToken->data.floatData);
                tokenIndex += 1;
                pToken = &pTokenStream->operator[](tokenIndex);
                if (pToken->type == JSONTokenType::TOKEN_COMMA)
                {
                    tokenIndex += 1;
                    continue;
                }
                else
                {
                    result = false;
                }
            } while (result);
            pJSONArray->count = (int32_t)pValueArray->getNum();
        }
        else
        {
            DBG_PANIC("Invalid JSON array type");
        }
        pToken = &pTokenStream->operator[](tokenIndex);
        if (pToken->type != JSONTokenType::TOKEN_RBRACK)
        {
            DBG_PANIC("Missing ]");
            return false;
        }
        tokenIndex += 1;
        *pCurrentTokenIndex = tokenIndex;
        pJSONValue->type = JsonValueType::VALUE_ARRAY;
        pJSONValue->pArray = pJSONArray;
        return true;
    }
    return false;
}

bool ParseJSONValue(JsonValue* pJSONValue, TArray<JSONToken>* pTokenStream, int32_t* pCurrentTokenIndex)
{
    if (ParseJSONNull(pJSONValue, pTokenStream, pCurrentTokenIndex)) return true;
    else if (ParseJSONString(pJSONValue, pTokenStream, pCurrentTokenIndex)) return true;
    else if (ParseJSONNumber(pJSONValue, pTokenStream, pCurrentTokenIndex)) return true;
    else if (ParseJSONBool(pJSONValue, pTokenStream, pCurrentTokenIndex)) return true;
    else if (ParseJSONNull(pJSONValue, pTokenStream, pCurrentTokenIndex)) return true;
    else if (ParseJSONObject(pJSONValue, pTokenStream, pCurrentTokenIndex)) return true;
    else if (ParseJSONArray(pJSONValue, pTokenStream, pCurrentTokenIndex)) return true;
    return false;
}
JsonValue* JsonObject::GetValue(const char* pKey)
{
    size_t len = strlen(pKey);
    int32_t count = (int32_t)data.getNum();

    for (int32_t index = 0; index < count; ++index)
    {
        if (strcmp(data[index].key.string.pBuffer, pKey) == 0)
        {
            return &data[index].value;
        }
    }
    DBG_ASSERT(0, "Failed to find value with key \"%s\"\n", pKey);
    return NULL;
}

JsonValue* JsonObject::TryGetValue(const char* pKey)
{
    size_t len = strlen(pKey);
    int32_t count = (int32_t)data.getNum();

    for (int32_t index = 0; index < count; ++index)
    {
        if (strcmp(data[index].key.string.pBuffer, pKey) == 0)
        {
            return &data[index].value;
        }
    }
    DBG_LOG("Failed to find value with key \"%s\"\n", pKey);
    return NULL;
}

JsonObject* JsonObject::TryGetObject(const char* pKey)
{
    return TryGetValue(pKey)->GetObj();
}

JsonArray* JsonObject::TryGetArray(const char* pKey)
{
    return TryGetValue(pKey)->GetArray();
}

JsonString* JsonObject::TryGetString(const char* pKey)
{
    return TryGetValue(pKey)->GetString();
}

JsonNumber* JsonObject::TryGetNumber(const char* pKey)
{
    return TryGetValue(pKey)->GetNumber();
}

bool JsonObject::TryGetBoolean(const char* pKey)
{
    return TryGetValue(pKey)->GetBoolean();
}

JsonObject* JsonObject::GetObj(const char* pKey) { return GetValue(pKey)->GetObj(); }
JsonArray* JsonObject::GetArray(const char* pKey) { return GetValue(pKey)->GetArray(); }
JsonString* JsonObject::GetString(const char* pKey) { return GetValue(pKey)->GetString(); }
JsonNumber* JsonObject::GetNumber(const char* pKey) { return GetValue(pKey)->GetNumber(); }
bool JsonObject::GetBoolean(const char* pKey) { return GetValue(pKey)->GetBoolean(); }

JsonValue* JsonArray::GetValueAt(int32_t index) { return &anyArray[index]; }
JsonObject* JsonArray::GetObjectAt(int32_t index) { return GetValueAt(index)->GetObj(); }
JsonArray* JsonArray::GetArrayAt(int32_t index) { return GetValueAt(index)->GetArray(); }
JsonString* JsonArray::GetStringAt(int32_t index) { return GetValueAt(index)->GetString(); }
JsonNumber* JsonArray::GetNumberAt(int32_t index) { return GetValueAt(index)->GetNumber(); }
bool JsonArray::GetBooleanAt(int32_t index) { return GetValueAt(index)->GetBoolean(); }
int32_t JsonArray::GetIntAt(int32_t index) { return intArray[index]; }
float JsonArray::GetFloatAt(int32_t index) { return floatArray[index]; }
TArray<JsonValue>* JsonArray::GetAnyArray() { return &anyArray; }
TArray<int32_t>* JsonArray::GetIntArray() { return &intArray; }
TArray<float>* JsonArray::GetFloatArray() { return &floatArray; }

bool json::parse(JsonValue* pOut, const void* pBuffer, size_t bufferSize, size_t lexerPreallocTokenCount)
{
    if (pBuffer == NULL || bufferSize == 0) return false;

    const char* pString = (const char*)pBuffer;
    char currentChar = 0;
    uint32_t currentLine = 1;
    uint32_t currentCharIndex = 0;
    TArray<JSONToken> tokenStream{};
    JsonObject root;

    if (lexerPreallocTokenCount > 0)
    {
        tokenStream.resize(lexerPreallocTokenCount);
    }

    DBG_LOG("Lexing JSON buffer\n");
    // Lexer
    while (currentCharIndex < bufferSize)
    {
        // Create Token Stream
        currentChar = pString[currentCharIndex];

        // Read number
        if (currentChar == '-' || (currentChar >= '0' && currentChar <= '9'))
        {
            char* pNumStr = (char*)&pString[currentCharIndex];
            uint32_t numSize = 1;
            JSONToken token;
            token.type = JSONTokenType::TOKEN_INT;
            currentCharIndex += 1;
            currentChar = pString[currentCharIndex];

            if (currentChar == '.' && token.type == JSONTokenType::TOKEN_INT)
            {
                token.type = JSONTokenType::TOKEN_FLOAT;
                numSize += 1;
                currentCharIndex += 1;
                currentChar = pString[currentCharIndex];
            }

            while (currentCharIndex < bufferSize && currentChar >= '0' && currentChar <= '9')
            {
                currentCharIndex += 1;
                currentChar = pString[currentCharIndex];
                if (currentChar == '.' && token.type == JSONTokenType::TOKEN_INT)
                {
                    token.type = JSONTokenType::TOKEN_FLOAT;
                    numSize += 1;
                    currentCharIndex += 1;
                    currentChar = pString[currentCharIndex];
                }
                else if (currentChar == '.')
                {
                    DBG_PANIC("Invalid token at line %u", currentLine);
                    return false;
                }

                numSize += 1;
            }

            if (token.type == JSONTokenType::TOKEN_INT)
            {
                char oldValue = pNumStr[numSize];
                pNumStr[numSize] = 0;
                int32_t value = atoi(pNumStr);
                pNumStr[numSize] = oldValue;
                token.data.intData = value;
                tokenStream.add(token);
            }
            else if (token.type == JSONTokenType::TOKEN_FLOAT)
            {
                char oldValue = pNumStr[numSize];
                pNumStr[numSize] = 0;
                float value = (float)atof(pNumStr);
                pNumStr[numSize] = oldValue;
                token.data.floatData = value;
                tokenStream.add(token);
            }
            else
            {
                DBG_PANIC("Invalid token at line %u", currentLine);
                return false;
            }
        }
        // Read String
        else if (currentChar == '"')
        {
            JSONToken token;
            token.type = JSONTokenType::TOKEN_STRING;

            // Move past opening quote
            currentCharIndex += 1;

            // Check if we are on bounds
            if (currentCharIndex >= bufferSize)
            {
                DBG_PANIC("Incomplete string at line %u", currentLine);
                return false;
            }

            token.data.stringData.pBuffer = &pString[currentCharIndex];
            token.data.stringData.length = 0;

            for (;;)
            {
                // Out of bounds while still inside string
                if (currentCharIndex >= bufferSize)
                {
                    DBG_PANIC("Incomplete string at line %u", currentLine);
                    return false;
                }

                currentChar = pString[currentCharIndex];

                // Handle escape sequences: \" \\ \n \t etc.
                if (currentChar == '\\')
                {
                    // Count '\' as part of the token
                    token.data.stringData.length++;
                    currentCharIndex++;

                    if (currentCharIndex >= bufferSize)
                    {
                        DBG_PANIC("Incomplete escape sequence at line %u", currentLine);
                        return false;
                    }

                    currentChar = pString[currentCharIndex];
                    if (currentChar == '\n')
                        currentLine += 1;

                    // Count escaped char as part of the token
                    token.data.stringData.length++;
                    currentCharIndex++;
                    continue;
                }

                // Unescaped quote -> end of string
                if (currentChar == '"')
                {
                    break;
                }

                // Normal character
                if (currentChar == '\n')
                    currentLine += 1;

                token.data.stringData.length++;
                currentCharIndex++;
            }

            // currentCharIndex is at the closing quote
            // (string buffer + length do NOT include the closing quote)
            tokenStream.add(token);

            // Move past closing quote
            currentCharIndex += 1;
        }
        // check for null value
        else if (currentChar == 'n')
        {
            if (currentCharIndex + 3 >= bufferSize)
            {
                DBG_PANIC("Undefine token at line %u", currentLine);
                return false;
            }
            if (pString[currentCharIndex + 1] == 'u' &&
                pString[currentCharIndex + 2] == 'l' &&
                pString[currentCharIndex + 3] == 'l')
            {
                JSONToken token;
                token.type = JSONTokenType::TOKEN_NULL;
                tokenStream.add(token);
                currentCharIndex += 4;
            }
            else
            {
                DBG_PANIC("Undefine token at line %u", currentLine);
                return false;
            }
        }
        // check for true value
        else if (currentChar == 't')
        {
            if (currentCharIndex + 3 >= bufferSize)
            {
                DBG_PANIC("Undefine token at line %u", currentLine);
                return false;
            }
            if (pString[currentCharIndex + 1] == 'r' &&
                pString[currentCharIndex + 2] == 'u' &&
                pString[currentCharIndex + 3] == 'e')
            {
                JSONToken token;
                token.type = JSONTokenType::TOKEN_TRUE;
                tokenStream.add(token);
                currentCharIndex += 4;
            }
            else
            {
                DBG_PANIC("Undefine token at line %u", currentLine);
                return false;
            }
        }
        // check for false value
        else if (currentChar == 'f')
        {
            if (currentCharIndex + 4 >= bufferSize)
            {
                DBG_PANIC("Undefine token at line %u", currentLine);
                return false;
            }
            if (pString[currentCharIndex + 1] == 'a' &&
                pString[currentCharIndex + 2] == 'l' &&
                pString[currentCharIndex + 3] == 's' &&
                pString[currentCharIndex + 4] == 'e')
            {
                JSONToken token;
                token.type = JSONTokenType::TOKEN_FALSE;
                tokenStream.add(token);
                currentCharIndex += 5;
            }
            else
            {
                DBG_PANIC("Undefine token at line %u", currentLine);
                return false;
            }
        }
        // Read special chars
        else if (currentChar == '[')
        {
            JSONToken token;
            token.type = JSONTokenType::TOKEN_LBRACK;
            tokenStream.add(token);
            currentCharIndex += 1;
        }
        else if (currentChar == ']')
        {
            JSONToken token;
            token.type = JSONTokenType::TOKEN_RBRACK;
            tokenStream.add(token);
            currentCharIndex += 1;
        }
        else if (currentChar == '{')
        {
            JSONToken token;
            token.type = JSONTokenType::TOKEN_LBRACE;
            tokenStream.add(token);
            currentCharIndex += 1;
        }
        else if (currentChar == '}')
        {
            JSONToken token;
            token.type = JSONTokenType::TOKEN_RBRACE;
            tokenStream.add(token);
            currentCharIndex += 1;
        }
        else if (currentChar == ':')
        {
            JSONToken token;
            token.type = JSONTokenType::TOKEN_COLON;
            tokenStream.add(token);
            currentCharIndex += 1;
        }
        else if (currentChar == ',')
        {
            JSONToken token;
            token.type = JSONTokenType::TOKEN_COMMA;
            tokenStream.add(token);
            currentCharIndex += 1;
        }
        else if (currentChar == ' ' ||
            currentChar == '\n' ||
            currentChar == '\r' ||
            currentChar == '\t' ||
            currentChar == '\0')
        {
            if (currentChar == '\n') currentLine += 1;
            currentCharIndex += 1;
        }
        else
        {
            DBG_PANIC("Undefine token at line %u", currentLine);
            return false;
        }
        tokenStream[tokenStream.getNum() - 1].line = currentLine;
        tokenStream[tokenStream.getNum() - 1].index = currentCharIndex;
    }

    // Parser
    DBG_LOG("Parsing JSON token stream. Token count: %u\n", (uint32_t)tokenStream.getNum());
    int32_t tokenIndex = 0;
    bool result = ParseJSONValue(pOut, &tokenStream, &tokenIndex);

    return result;
}

void FreeJSONData(JsonValue* pJSONValue)
{
    JsonValueType type = pJSONValue->type;

    switch (type)
    {
    case JsonValueType::VALUE_STRING:
        free(pJSONValue->string.pBuffer);
        break;
    case JsonValueType::VALUE_NUMBER:
        // nothig to do here
        break;
    case JsonValueType::VALUE_OBJECT:
    {
        int32_t count = (int32_t)pJSONValue->pObject->data.getNum();

        for (int32_t index = 0; index < count; ++index)
        {
            JsonKeyValuePair* pKeyValuePair = &pJSONValue->pObject->data[index];
            free(pKeyValuePair->key.string.pBuffer);
            FreeJSONData(&pKeyValuePair->value);
        }

        pJSONValue->pObject->data.~TArray();
        free(pJSONValue->pObject);
        break;
    }
    case JsonValueType::VALUE_ARRAY:
    {
        if (pJSONValue->pArray->type == JsonArrayType::ARRAY_ANY)
        {
            int32_t count = (int32_t)pJSONValue->pArray->anyArray.getNum();

            for (int32_t index = 0; index < count; ++index)
            {
                FreeJSONData(&pJSONValue->pArray->anyArray[index]);
            }

            pJSONValue->pArray->anyArray.~TArray();
        }
        else if (pJSONValue->pArray->type == JsonArrayType::ARRAY_INT)
        {
            pJSONValue->pArray->intArray.~TArray();
        }
        else if (pJSONValue->pArray->type == JsonArrayType::ARRAY_FLOAT)
        {
            pJSONValue->pArray->floatArray.~TArray();
        }
        else
        {
            DBG_PANIC("WAT");
        }
        free(pJSONValue->pArray);
        break;
    }
    case JsonValueType::VALUE_BOOL:
        // nothig to do here
        break;
    case JsonValueType::VALUE_NULL:
        // nothig to do here
        break;
    }
}

void json::release(JsonValue* pJSONRoot)
{
    FreeJSONData(pJSONRoot);
}
