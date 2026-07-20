#include <shared_mutex>
#include <mutex>
#include <algorithm>
#include <numeric>
#include <cstring>

#include "core/ecs/query.hpp"

namespace core::ecs {

    Query::Query(const std::vector<std::unique_ptr<Archetype>>& registry) noexcept
        : registry(registry) {}

    Query& Query::with(const std::uint32_t component) {
        required.set(component);
        elements.push_back(component);
        return *this;
    }

    Query& Query::without(const std::uint32_t component) {
        excluded.set(component);
        return *this;
    }

    Query& Query::any(const std::vector<std::uint32_t>& components) {
        for (const std::uint32_t component : components) {
            optional.set(component);
        }
        return *this;
    }

    Query& Query::sort(const std::uint32_t component) {
        for (const auto& archetype : registry) {
            std::unique_lock lock(archetype->mutex);
            if (Mask check = archetype->signature; (check & required) == required && check.test(component)) {
                const std::size_t offset = archetype->offsets[component];
                std::size_t size = archetype->sizes[offset];
                if (size == 0) continue;

                std::vector<std::size_t> indices(archetype->entities.size());
                std::iota(indices.begin(), indices.end(), 0);

                std::uint8_t* raw = archetype->columns[offset].data();
                std::ranges::sort(indices, [raw, size](std::size_t left, std::size_t right) {
                    return std::memcmp(raw + left * size, raw + right * size, size) < 0;
                });

                std::vector<Id> arranged(archetype->entities.size());
                for (std::size_t index = 0; index < indices.size(); ++index) {
                    arranged[index] = archetype->entities[indices[index]];
                }
                archetype->entities = std::move(arranged);

                for (std::size_t track = 0; track < archetype->columns.size(); ++track) {
                    const std::size_t byte = archetype->sizes[track];
                    if (byte == 0) continue;
                    std::vector<std::uint8_t> linear(archetype->columns[track].size());
                    for (std::size_t index = 0; index < indices.size(); ++index) {
                        std::memcpy(linear.data() + index * byte,
                                    archetype->columns[track].data() + indices[index] * byte,
                                    byte);
                    }
                    archetype->columns[track] = std::move(linear);
                }
                archetype->state.fetch_add(1, std::memory_order_relaxed);
            }
        }
        return *this;
    }

    bool Query::has(const std::uint32_t component) const noexcept {
        return required.test(component);
    }

    void Query::process(const Callback& callback, const std::vector<std::uint32_t>& components) const {
        std::vector<void*> pointers;
        for (const auto& archetype : registry) {
            std::shared_lock lock(archetype->mutex);
            if (Mask check = archetype->signature; (check & required) == required && (check & excluded).none()) {
                if (optional.any() && (check & optional).none()) continue;

                const std::size_t total = archetype->entities.size();
                if (total == 0) continue;

                pointers.clear();
                for (std::uint32_t component : components) {
                    if (auto find = archetype->offsets.find(component); find != archetype->offsets.end()) {
                        pointers.push_back(archetype->columns[find->second].data());
                    } else {
                        pointers.push_back(nullptr);
                    }
                }
                callback(total, archetype->entities.data(), pointers);
            }
        }
    }

    void Query::dispatch(VkCommandBuffer command, VkPipelineLayout layout, VkPipeline pipeline, const std::vector<std::uint32_t>& components) const {
        (void)layout;
        (void)components;
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        for (const auto& archetype : registry) {
            if (Mask check = archetype->signature; (check & required) == required && (check & excluded).none()) {
                if (optional.any() && (check & optional).none()) continue;

                std::shared_lock lock(archetype->mutex);
                const auto count = static_cast<std::uint32_t>(archetype->entities.size());
                if (count == 0) continue;

                vkCmdDispatch(command, (count + 63) / 64, 1, 1);
            }
        }
    }

}