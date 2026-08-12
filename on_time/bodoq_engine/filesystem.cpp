#include "filesystem.h"
#include "win32.h"
#pragma comment(lib, "Shlwapi.lib")

#include <Shlwapi.h>

static inline void normalizeSlashes(char* s)
{
    for (; *s; ++s) if (*s == '\\') *s = '/';
}

static inline bool startsWithDotSlash(const char* s)
{
    return s && s[0] == '.' && (s[1] == '\\' || s[1] == '/');
}

static inline void NormalizeSlashesInPlace(char* s)
{
    for (; *s; ++s) if (*s == '\\') *s = '/';
}

static inline void StripLeadingDotSlash(char*& p)
{
    while (p[0] == '.' && (p[1] == '\\' || p[1] == '/'))
        p += 2;
}

String fs::getRelativePath(const String& path, const String& basePath)
{
    char baseFull[MAX_PATH] = {};
    char absFull[MAX_PATH] = {};
    DWORD n1 = GetFullPathNameA(basePath.rawStr(), MAX_PATH, baseFull, nullptr);
    DWORD n2 = GetFullPathNameA(path.rawStr(), MAX_PATH, absFull, nullptr);
    if (n1 == 0 || n1 >= MAX_PATH || n2 == 0 || n2 >= MAX_PATH)
    {
        return path;
    }
    DWORD baseAttr = FILE_ATTRIBUTE_DIRECTORY;
    DWORD absAttr = FILE_ATTRIBUTE_NORMAL;

    char rel[MAX_PATH] = {};
    BOOL ok = PathRelativePathToA(rel, baseFull, baseAttr, absFull, absAttr);

    if (!ok || rel[0] == '\0')
    {
        normalizeSlashes(absFull);
        return String(absFull);
    }

    char* out = rel;
    if (startsWithDotSlash(rel))
        out += 2;

    normalizeSlashes(out);
    return String(out);
}

static bool IsLikelyFilePath(const char* path)
{
    const char* lastSlash = strrchr(path, '\\');
    if (!lastSlash) lastSlash = strrchr(path, '/');
    const char* lastDot = strrchr(path, '.');
    return lastDot && (!lastSlash || lastDot > lastSlash);
}

static void ExtractBaseDirectory(const String& projectPath, char* outDir, size_t outCap)
{
    char buf[MAX_PATH] = {};
    // copy projectPath into buf
    {
        const char* s = projectPath.rawStr();
        strncpy_s(buf, s ? s : "", _TRUNCATE);
    }

    // If it looks like a file, drop the filename
    if (IsLikelyFilePath(buf))
        PathRemoveFileSpecA(buf);

    // Canonicalize to absolute
    char full[MAX_PATH] = {};
    DWORD n = GetFullPathNameA(buf, MAX_PATH, full, nullptr);
    if (n == 0 || n >= MAX_PATH)
        strncpy_s(outDir, outCap, buf, _TRUNCATE);
    else
        strncpy_s(outDir, outCap, full, _TRUNCATE);

    // Ensure trailing slash for combining
    size_t len = strlen(outDir);
    if (len && outDir[len - 1] != '\\' && outDir[len - 1] != '/')
        strncat_s(outDir, outCap, "\\", _TRUNCATE);
}


String fs::getAbsolutePath(const String& maybeRelative, const String& projectPath)
{
    const char* relC = maybeRelative.rawStr();
    if (!relC) return String(""); // empty

    // 1) If it's already absolute, canonicalize & return.
    if (PathIsRelativeA(relC) == FALSE)
    {
        char full[MAX_PATH] = {};
        DWORD n = GetFullPathNameA(relC, MAX_PATH, full, nullptr);
        if (n > 0 && n < MAX_PATH) {
            NormalizeSlashesInPlace(full);
            return String(full);
        }
        return maybeRelative;
    }

    // 2) Derive a proper base directory from projectPath (file or dir)
    char baseDir[MAX_PATH] = {};
    ExtractBaseDirectory(projectPath, baseDir, MAX_PATH);

    // 3) Prepare the relative part (strip leading "./" or ".\")
    char relBuf[MAX_PATH] = {};
    strncpy_s(relBuf, relC, _TRUNCATE);
    char* rel = relBuf;
    StripLeadingDotSlash(rel);

    // 4) Combine base + relative
    char combined[MAX_PATH] = {};
    if (!PathCombineA(combined, baseDir, rel))
    {
        // Fallback concat
        strncpy_s(combined, baseDir, _TRUNCATE);
        strncat_s(combined, rel, _TRUNCATE);
    }

    // 5) Canonicalize to resolve ".", ".."
    char canon[MAX_PATH] = {};
    if (!PathCanonicalizeA(canon, combined))
    {
        DWORD n = GetFullPathNameA(combined, MAX_PATH, canon, nullptr);
        if (n == 0 || n >= MAX_PATH)
            strncpy_s(canon, combined, _TRUNCATE);
    }

    NormalizeSlashesInPlace(canon);
    return String(canon);
}

String fs::getRelativePath(const String& path, const WString& basePath)
{
    return getRelativePath(path, str::toString(basePath));
}

String fs::getAbsolutePath(const String& path, const WString& basePath)
{
    return getAbsolutePath(path, str::toString(basePath));
}

String fs::getCurrentDirectory()
{
    char buffer[MAX_PATH] = {};
    DWORD len = GetCurrentDirectoryA(MAX_PATH, buffer);
    if (len == 0 || len >= MAX_PATH)
        return String("");

    for (char* p = buffer; *p; ++p)
        if (*p == '\\') *p = '/';

    return String(buffer);
}

String fs::getBaseDirectory(const String& filePath)
{
    char path[MAX_PATH];
    strncpy_s(path, filePath.rawStr(), _TRUNCATE);

    if (PathRemoveFileSpecA(path))
    {
        for (char* p = path; *p; ++p)
            if (*p == '\\') *p = '/';
        return TString<>(path);
    }

    return filePath;
}
