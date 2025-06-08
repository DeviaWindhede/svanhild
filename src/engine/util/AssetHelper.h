#pragma once

inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
{
    if (path == nullptr)
    {
        throw std::exception();
    }

    DWORD size = GetModuleFileName(nullptr, path, pathSize);
    if (size == 0 || size == pathSize)
    {
        // Method failed or path was truncated.
        throw std::exception();
    }

    if (WCHAR* lastSlash = wcsrchr(path, L'\\'))
    {
        *(lastSlash + 1) = L'\0';
    }
}
