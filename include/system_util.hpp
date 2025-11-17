//
// Created by margo on 15/11/2025.
//

#pragma once
#include <string>

namespace system_util
{
    static std::string get_system_platform( )
    {
#ifdef _WIN32
        return "windows";
#elif defined(_WIN64)
        return "windows";
#elif defined(__unix__) || defined(__unix)
        return "unix";
#else
        return "unknown";
#endif
    }
}