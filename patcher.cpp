#include <iostream>
#include <fstream>
#include <Windows.h>
#include <WinBase.h>
#include <cstdio>
#include <ShlObj.h>
#include <propkey.h>
#include <atlbase.h>
#include <io.h>
#include <fcntl.h>
#include <string>
#include <system_error>
#include <vector>

bool MatchSignature(const uint8_t* data, const uint8_t* sig, const char* mask)
{
    for (; *mask; ++mask, ++data, ++sig)
    {
        if (*mask == 'x' && *data != *sig) return false;
    }
    return *mask == NULL;
}

uint8_t* FindSignature(uint8_t* data, uint64_t start, uint64_t size, const uint8_t* sig, const char* mask)
{
    for (uint64_t i = 0; i < size; i++)
    {
        if (MatchSignature(data + start + i, sig, mask))
        {
            return data + start + i;
        }
    }

    return nullptr;
}

bool FileExists(const std::wstring& name)
{
    std::ifstream f(name);
    return f.good();
}

std::wstring GetShellPropStringFromPath(LPCWSTR pPath, PROPERTYKEY const& key)
{
    CComPtr<IShellItem2> pItem;
    HRESULT hr = SHCreateItemFromParsingName(pPath, nullptr, IID_PPV_ARGS(&pItem));
    if (FAILED(hr)) throw std::system_error(hr, std::system_category(), "SHCreateItemFromParsingName() failed");

    CComHeapPtr<WCHAR> pValue;
    hr = pItem->GetString(key, &pValue);
    if (FAILED(hr)) throw std::system_error(hr, std::system_category(), "IShellItem2::GetString() failed");

    return std::wstring(pValue);
}

int wmain(int argc, wchar_t* argv[])
{
    auto imagePath = std::wstring(argv[0]);
    std::wstring basename = imagePath.substr(imagePath.find_last_of(L"/\\") + 1);
    if (_wcsicmp(basename.c_str(), L"KFEditor.exe") == 0)
    {
        size_t len = 0;

        for (auto i = 1; i < argc; i++)
        {
            len += wcslen(argv[i]);
        }

        const bool shouldPatch = argc > 1 && _wcsicmp(L"mergepackages", argv[1]) == 0;
        const wchar_t* makeCommandlet = L"make";

        wchar_t* all_args;
        {
            auto size = (len + argc) * sizeof(wchar_t);
            wchar_t* _all_args = all_args = (wchar_t*)malloc(size);
            memset(_all_args, 0, size);

            for (auto i = 0; i < argc; i++)
            {
                if (shouldPatch && i == 1)
                {
                    memcpy(_all_args, makeCommandlet, wcslen(makeCommandlet) * sizeof(wchar_t));
                    _all_args += wcslen(makeCommandlet);
                }
                else
                {
                    memcpy(_all_args, argv[i], wcslen(argv[i]) * sizeof(wchar_t));
                    _all_args += wcslen(argv[i]);
                }
                *_all_args = L' ';
                _all_args++;
            }
            *(_all_args) = L'\0';
        }

        STARTUPINFO si{ sizeof(si) };
        PROCESS_INFORMATION pi{};
        if (CreateProcess(shouldPatch ? L"KFEditor_mergepackages.exe" : L"KFEditor_original.exe", all_args, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        {
            WaitForSingleObject(pi.hProcess, INFINITE);

            DWORD exitCode;
            if (!GetExitCodeProcess(pi.hProcess, &exitCode)) return -1;

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            return exitCode;
        }

        return -1;
    }

    if (!FileExists(L"KFEditor.exe")) return -2;

    CoInitialize(nullptr);
    _setmode(_fileno(stdout), _O_U16TEXT);

    try
    {
        wchar_t* fileExt;
        wchar_t filePath[256];
        if (!GetFullPathName(L"KFEditor.exe", 256, filePath, &fileExt)) return -1;
        std::wstring fileDesc = GetShellPropStringFromPath(filePath, PKEY_FileDescription);
        if (wcscmp(L"Killing Floor 2", fileDesc.c_str()) != 0) return -3;
    }
    catch (std::system_error const& e)
    {
        return -4;
    }

    std::vector<char> buffer;
    std::streamsize size;
    {
        std::ifstream file(L"KFEditor.exe", std::ios::binary | std::ios::ate);
        size = file.tellg();
        file.seekg(0, std::ios::beg);

        buffer = std::vector<char>(size);
        if (!file.read(buffer.data(), size))
        {
            file.close();
            return -1;
        }
        file.close();
    }

    // Patch the class reference string
    {
        const wchar_t commandletName[24] = L"UnrealEd.MakeCommandlet";
        const wchar_t commandletReplacement[24] = L"UnrealEd.Merge\0\0\0\0\0\0\0\0\0";
        const char commandletMask[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

        auto data = (uint8_t*)buffer.data();

        uint64_t startOffset = 0;
        uint8_t* result;
        while ((result = FindSignature(data, startOffset, size - startOffset, (const uint8_t*)&commandletName, commandletMask)) != nullptr)
        {
            startOffset = (result - data) + sizeof(commandletName);
            memcpy(result, commandletReplacement, sizeof(commandletReplacement));
        }
    }

    // Patch the class name
    {
        const wchar_t commandletName[25] = L"UMergePackagesCommandlet";
        const wchar_t commandletReplacement[25] = L"UMerge\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
        const char commandletMask[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

        auto data = (uint8_t*)buffer.data();

        uint64_t startOffset = 0;
        uint8_t* result;
        while ((result = FindSignature(data, startOffset, size - startOffset, (const uint8_t*)&commandletName, commandletMask)) != nullptr)
        {
            startOffset = (result - data) + sizeof(commandletName);
            memcpy(result, commandletReplacement, sizeof(commandletReplacement));
        }
    }

    std::ofstream fout("KFEditor_mergepackages.exe", std::ios::out | std::ios::binary);
    fout.write(buffer.data(), buffer.size());
    fout.close();

    if (!CopyFile(L"KFEditor.exe", L"KFEditor_original.exe", false)) return -5;
    if (!CopyFile(argv[0], L"KFEditor.exe", false)) return -5;

    return 0;
}
