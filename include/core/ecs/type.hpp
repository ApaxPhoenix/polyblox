#pragma once

#include <cstddef>

namespace core::ecs {

    class Archetype;

    using Id = std::uint32_t;
    constexpr Id Null = 0xFFFFFFFF;

    struct Slot {
        Archetype* archetype = nullptr;
        std::size_t row = 0;
    };

}