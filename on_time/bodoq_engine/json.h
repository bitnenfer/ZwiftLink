#pragma once

#include "array.h"

struct JsonString;
struct JsonNumber;
struct JsonKeyValuePair;
struct JsonObject;
struct JsonArray;
struct JsonValue;

enum class JsonValueType
{
    VALUE_STRING,
    VALUE_NUMBER,
    VALUE_OBJECT,
    VALUE_ARRAY,
    VALUE_BOOL,
    VALUE_NULL
};

enum class JsonNumberType
{
    NUMBER_FLOAT,
    NUMBER_INT
};

enum class JsonArrayType
{
    ARRAY_ANY,
    ARRAY_INT,
    ARRAY_FLOAT
};

struct JsonString
{
    inline const char* GetString() { return pBuffer; }
    char* pBuffer;
    int32_t length;
};

struct JsonNumber
{
    inline int32_t GetInt() { return intData; }
    inline float GetFloat() { return floatData; }

    JsonNumberType type;
    union
    {
        float floatData;
        int32_t intData;
    };
};

struct JsonObject
{
    JsonValue* GetValue(const char* pKey);
    JsonObject* GetObj(const char* pKey);
    JsonArray* GetArray(const char* pKey);
    JsonString* GetString(const char* pKey);
    JsonNumber* GetNumber(const char* pKey);
    bool GetBoolean(const char* pKey);
    JsonValue* TryGetValue(const char* pKey);
    JsonObject* TryGetObject(const char* pKey);
    JsonArray* TryGetArray(const char* pKey);
    JsonString* TryGetString(const char* pKey);
    JsonNumber* TryGetNumber(const char* pKey);
    bool TryGetBoolean(const char* pKey);

    int32_t count;
    TArray<JsonKeyValuePair> data;
};

struct JsonArray
{
    JsonValue* GetValueAt(int32_t index);
    JsonObject* GetObjectAt(int32_t index);
    JsonArray* GetArrayAt(int32_t index);
    JsonString* GetStringAt(int32_t index);
    JsonNumber* GetNumberAt(int32_t index);
    bool GetBooleanAt(int32_t index);
    int32_t GetIntAt(int32_t index);
    float GetFloatAt(int32_t index);
    TArray<JsonValue>* GetAnyArray();
    TArray<int32_t>* GetIntArray();
    TArray<float>* GetFloatArray();

    JsonArrayType type;
    int32_t count;
    union
    {
        TArray<JsonValue> anyArray;
        TArray<int32_t> intArray;
        TArray<float> floatArray;
    };

    ~JsonArray() = delete;
};

struct JsonValue
{
    inline bool IsType(JsonValueType typeToCheck) const { return typeToCheck == type; }
    inline JsonObject* GetObj() { return pObject; }
    inline JsonArray* GetArray() { return pArray; }
    inline JsonString* GetString() { return &string; }
    inline JsonNumber* GetNumber() { return &number; }
    inline bool GetBoolean() { return boolean; }

    JsonValueType type;
    union
    {
        JsonObject* pObject;
        JsonArray* pArray;
        JsonString string;
        JsonNumber number;
        bool boolean;
    };
};

struct JsonKeyValuePair
{
    inline bool IsKeyType(JsonValueType typeToCheck) const { return typeToCheck == key.type; }
    inline bool IsValueType(JsonValueType typeToCheck) const { return typeToCheck == value.type; }

    JsonValue key;
    JsonValue value;
};

namespace json
{
    bool parse(JsonValue* pOut, const void* pBuffer, size_t bufferSize, size_t lexerPreallocTokenCount = 0);
    void release(JsonValue* pJSONRoot);
}
