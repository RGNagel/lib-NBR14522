#pragma once

#include <cstdint>
#include <string>
#include <vector>

inline std::vector<std::uint8_t> hexstr2bytes(const std::string& hexbytes) {
    std::vector<std::uint8_t> retval;

    // should not be odd length
    if (hexbytes.size() & 1)
        return retval;

    for (size_t i = 0; i < hexbytes.size(); i += 2) {
        std::string hexbyte = hexbytes.substr(i, 2);
        auto byte = strtol(hexbyte.c_str(), nullptr, 16);
        retval.push_back(static_cast<std::uint8_t>(byte));
    }

    return retval;
}