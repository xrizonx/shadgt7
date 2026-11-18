#pragma once

#include <cstdio>
#include <cstdint>
#include <vector>
#include <xxhash.h>

namespace Vulkan {

inline constexpr std::uint32_t ShaderObjectCacheMagic = 0x32424F53;

inline std::vector<std::vector<std::uint8_t>> ReadShaderObjectBinaries(
    std::FILE* file, std::uint32_t expected_count) {
    std::uint32_t magic = 0, count = 0;
    if (!file || expected_count == 0 || expected_count > 4 ||
        std::fread(&magic, sizeof(magic), 1, file) != 1 ||
        std::fread(&count, sizeof(count), 1, file) != 1 ||
        magic != ShaderObjectCacheMagic || count != expected_count) {
        return {};
    }
    std::vector<std::vector<std::uint8_t>> binaries;
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint64_t size = 0, checksum = 0;
        if (std::fread(&size, sizeof(size), 1, file) != 1 || size == 0 ||
            size >= (64ull << 20) ||
            std::fread(&checksum, sizeof(checksum), 1, file) != 1) {
            return {};
        }
        auto& binary = binaries.emplace_back(static_cast<std::size_t>(size));
        if (std::fread(binary.data(), 1, binary.size(), file) != binary.size() ||
            XXH3_64bits(binary.data(), binary.size()) != checksum) {
            return {};
        }
    }
    return binaries;
}

/// PERF-GT7-108: the binaries in SO-013's format (ReadShaderObjectBinaries reads them back).
inline bool WriteShaderObjectBinaries(std::FILE* file,
                                      const std::vector<std::vector<std::uint8_t>>& binaries) {
    // Keep the writer's constraints identical to ReadShaderObjectBinaries. Validate everything
    // before the first byte so invalid driver data never creates an unreadable cache file.
    if (!file || binaries.empty() || binaries.size() > 4) {
        return false;
    }
    for (const auto& binary : binaries) {
        if (binary.empty() || binary.size() >= (64ull << 20)) {
            return false;
        }
    }

    const auto count = static_cast<std::uint32_t>(binaries.size());
    if (std::fwrite(&ShaderObjectCacheMagic, sizeof(ShaderObjectCacheMagic), 1, file) != 1 ||
        std::fwrite(&count, sizeof(count), 1, file) != 1) {
        return false;
    }
    for (const auto& binary : binaries) {
        const std::uint64_t size = binary.size();
        const std::uint64_t checksum = XXH3_64bits(binary.data(), binary.size());
        if (std::fwrite(&size, sizeof(size), 1, file) != 1 ||
            std::fwrite(&checksum, sizeof(checksum), 1, file) != 1 ||
            std::fwrite(binary.data(), 1, binary.size(), file) != binary.size()) {
            return false;
        }
    }
    return true;
}


}
