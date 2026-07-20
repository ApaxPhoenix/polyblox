#pragma once

#include <vector>
#include <shared_mutex>
#include <atomic>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "mask.hpp"
#include "type.hpp"

namespace core::ecs {

    class Archetype {
    public:
        Mask signature;
        std::vector<Id> entities;
        std::vector<std::vector<std::uint8_t>> columns;
        std::vector<VkBuffer> buffers;
        std::vector<std::size_t> sizes;
        std::unordered_map<std::uint32_t, std::size_t> offsets;
        std::unordered_map<std::uint32_t, Archetype*> next;
        std::unordered_map<std::uint32_t, Archetype*> previous;
        std::atomic<std::uint32_t> state{0};
        mutable std::shared_mutex mutex;

        void allocate(Id id);
        std::uint32_t dispose(std::size_t row);
    };

}