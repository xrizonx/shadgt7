// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace Vulkan {

/// Find a completed resource, beginning at the last allocation hint and wrapping once.
/// This does not query the driver; the caller refreshes its GPU timeline only on a miss.
inline std::optional<std::size_t> FindReusablePoolSlot(
    std::span<const std::uint64_t> ticks, std::size_t hint, std::uint64_t completed_tick) {
    if (ticks.empty()) {
        return std::nullopt;
    }
    if (hint >= ticks.size()) {
        hint = 0;
    }
    for (std::size_t i = hint; i < ticks.size(); ++i) {
        if (ticks[i] <= completed_tick) {
            return i;
        }
    }
    for (std::size_t i = 0; i < hint; ++i) {
        if (ticks[i] <= completed_tick) {
            return i;
        }
    }
    return std::nullopt;
}

} // namespace Vulkan
