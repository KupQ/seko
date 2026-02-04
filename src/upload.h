#pragma once
#include <windows.h>
#include <vector>
#include <string>


std::wstring UploadToNekoo(const std::vector<BYTE>& imageData, const std::wstring& apiKey = L"");
std::wstring UploadFileToNekoo(const std::wstring& filePath, const std::wstring& apiKey = L"");
