#pragma once

#include "string.h"

namespace fs
{
	String getRelativePath(const String& path, const String& basePath);
	String getAbsolutePath(const String& path, const String& basePath);
	String getRelativePath(const String& path, const WString& basePath);
	String getAbsolutePath(const String& path, const WString& basePath);
	String getCurrentDirectory();
	String getBaseDirectory(const String& path);
}