#include <fstream>
#include <string>
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

int wmain(int argc, wchar_t* argv[])
{
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

    return 0;
}
