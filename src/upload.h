#pragma once
#include <windows.h>
#include <vector>
#include <string>

std::wstring UploadToNekoo(const std::vector<BYTE>& imageData, const std::wstring& apiKey = L"");
