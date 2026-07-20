#pragma once

#include <bitset>
#include <cstddef>

namespace core::ecs {

    constexpr std::size_t Capacity = 256;
    using Mask = std::bitset<Capacity>;

}