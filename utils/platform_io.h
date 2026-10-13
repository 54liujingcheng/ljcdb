#pragma once

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>

#ifndef _WIN32
#include <sys/types.h>
#endif

namespace ljcdb::platform {

inline std::FILE* openFile(const std::filesystem::path& path, const char* mode) {
#ifdef _WIN32
    std::wstring wideMode;
    while (*mode != '\0') {
        wideMode.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*mode)));
        ++mode;
    }
    return ::_wfopen(path.c_str(), wideMode.c_str());
#else
    return std::fopen(path.c_str(), mode);
#endif
}

inline bool seekFile(std::FILE* file, std::uint64_t offset) {
#ifdef _WIN32
    return ::_fseeki64(file, static_cast<std::int64_t>(offset), SEEK_SET) == 0;
#else
    return ::fseeko(file, static_cast<off_t>(offset), SEEK_SET) == 0;
#endif
}

}  // namespace ljcdb::platform
