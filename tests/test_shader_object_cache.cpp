#include <cstdio>
#include <cstdint>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "video_core/renderer_vulkan/shader_object_cache.h"

namespace {

using File = std::unique_ptr<std::FILE, decltype(&std::fclose)>;

File Cache(std::uint32_t magic, bool damaged = false, bool truncated = false,
           std::uint64_t size = 4) {
    File file{std::tmpfile(), &std::fclose};
    if (!file) {
        return file;
    }
    const std::uint32_t count = 1;
    std::vector<std::uint8_t> payload{10, 20, 30, 40};
    const std::uint64_t checksum = XXH3_64bits(payload.data(), payload.size());
    if (damaged) {
        payload[1] ^= 1;
    }
    std::fwrite(&magic, sizeof(magic), 1, file.get());
    std::fwrite(&count, sizeof(count), 1, file.get());
    std::fwrite(&size, sizeof(size), 1, file.get());
    if (magic == Vulkan::ShaderObjectCacheMagic) {
        std::fwrite(&checksum, sizeof(checksum), 1, file.get());
    }
    std::fwrite(payload.data(), 1, truncated ? 2 : payload.size(), file.get());
    std::rewind(file.get());
    return file;
}

}

TEST(ShaderObjectCache, ReadsIntactCheckedBinary) {
    auto file = Cache(Vulkan::ShaderObjectCacheMagic);
    ASSERT_TRUE(file);
    const auto binaries = Vulkan::ReadShaderObjectBinaries(file.get(), 1);
    ASSERT_EQ(binaries.size(), 1u);
    EXPECT_EQ(binaries[0], (std::vector<std::uint8_t>{10, 20, 30, 40}));
}

TEST(ShaderObjectCache, RejectsUncheckedLegacyBinary) {
    auto file = Cache(0x31424F53);
    ASSERT_TRUE(file);
    EXPECT_TRUE(Vulkan::ReadShaderObjectBinaries(file.get(), 1).empty());
}

TEST(ShaderObjectCache, RejectsCorruptionAndTruncation) {
    auto corrupt = Cache(Vulkan::ShaderObjectCacheMagic, true);
    auto truncated = Cache(Vulkan::ShaderObjectCacheMagic, false, true);
    ASSERT_TRUE(corrupt && truncated);
    EXPECT_TRUE(Vulkan::ReadShaderObjectBinaries(corrupt.get(), 1).empty());
    EXPECT_TRUE(Vulkan::ReadShaderObjectBinaries(truncated.get(), 1).empty());
}

TEST(ShaderObjectCache, RejectsWrongStageCountAndOversizedAllocation) {
    auto count = Cache(Vulkan::ShaderObjectCacheMagic);
    auto oversized = Cache(Vulkan::ShaderObjectCacheMagic, false, false, 64ull << 20);
    ASSERT_TRUE(count && oversized);
    EXPECT_TRUE(Vulkan::ReadShaderObjectBinaries(count.get(), 2).empty());
    EXPECT_TRUE(Vulkan::ReadShaderObjectBinaries(oversized.get(), 1).empty());
}

TEST(ShaderObjectCache, WriterRoundTripsMultipleShaderStages) {
    File file{std::tmpfile(), &std::fclose};
    ASSERT_TRUE(file);
    const std::vector<std::vector<std::uint8_t>> expected{
        {1, 2, 3, 4}, {10, 20, 30, 40, 50}};
    ASSERT_TRUE(Vulkan::WriteShaderObjectBinaries(file.get(), expected));
    std::rewind(file.get());
    EXPECT_EQ(Vulkan::ReadShaderObjectBinaries(file.get(), 2), expected);
}

TEST(ShaderObjectCache, WriterRejectsInvalidInputWithoutTouchingFile) {
    File file{std::tmpfile(), &std::fclose};
    ASSERT_TRUE(file);
    EXPECT_FALSE(Vulkan::WriteShaderObjectBinaries(nullptr, {{1}}));
    EXPECT_FALSE(Vulkan::WriteShaderObjectBinaries(file.get(), {}));
    EXPECT_FALSE(Vulkan::WriteShaderObjectBinaries(file.get(), {{}}));
    EXPECT_FALSE(Vulkan::WriteShaderObjectBinaries(file.get(),
                                                   {{1}, {2}, {3}, {4}, {5}}));
    EXPECT_EQ(std::ftell(file.get()), 0L);
}

TEST(ShaderObjectCache, WriterRejectsOversizedDriverBinary) {
    File file{std::tmpfile(), &std::fclose};
    ASSERT_TRUE(file);
    std::vector<std::vector<std::uint8_t>> oversized(1);
    oversized.front().resize(64ull << 20);
    EXPECT_FALSE(Vulkan::WriteShaderObjectBinaries(file.get(), oversized));
    EXPECT_EQ(std::ftell(file.get()), 0L);
}
