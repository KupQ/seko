#pragma once
#include <windows.h>
#include <string>

void ShowToastNotification(const std::wstring& url);
void ShowUploadingToast();
void UpdateToastWithUrl(const std::wstring& url);
void CloseToast();
