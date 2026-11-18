// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "video_core/renderer_vulkan/resource_pool_selection.h"

using Vulkan::FindReusablePoolSlot;

TEST(ResourcePoolSelection, EmptyPoolNeedsAnAllocation) {
    const std::vector<std::uint64_t> ticks;
    EXPECT_FALSE(FindReusablePoolSlot(ticks, 0, 10).has_value());
}

TEST(ResourcePoolSelection, WrapsBeforeRefreshingTheSemaphore) {
    // The hinted tail is busy, but a resource before the hint is already complete.
    // The pool must reuse it without an unnecessary Vulkan semaphore query.
    const std::array<std::uint64_t, 4> ticks{2, 9, 10, 11};
    EXPECT_EQ(FindReusablePoolSlot(ticks, 2, 5), 0u);
}

TEST(ResourcePoolSelection, UsesNewGpuProgressAfterRefresh) {
    const std::array<std::uint64_t, 3> ticks{8, 7, 9};
    EXPECT_FALSE(FindReusablePoolSlot(ticks, 2, 6).has_value());
    EXPECT_EQ(FindReusablePoolSlot(ticks, 2, 8), 0u);
}

TEST(ResourcePoolSelection, HonorsHintAndIncompleteTicks) {
    const std::array<std::uint64_t, 4> ticks{1, 3, 3, 4};
    EXPECT_EQ(FindReusablePoolSlot(ticks, 1, 3), 1u);
    EXPECT_EQ(FindReusablePoolSlot(ticks, 3, 3), 0u);
    EXPECT_FALSE(FindReusablePoolSlot(ticks, 0, 0).has_value());
    EXPECT_EQ(FindReusablePoolSlot(ticks, 999, 3), 0u);
}
