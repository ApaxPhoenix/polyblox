#include <algorithm>
#include <mutex>

#include "core/ecs/archetype.hpp"

namespace core::ecs {

    void Archetype::allocate(const Id id) {
        std::unique_lock guard(this->mutex);

        entities.push_back(id);
        for (std::size_t index = 0; index < columns.size(); ++index) {
            columns[index].resize(entities.size() * sizes[index]);
        }

        state.fetch_add(1, std::memory_order_relaxed);
    }

    std::uint32_t Archetype::dispose(const std::size_t row) {
        std::unique_lock guard(this->mutex);
        const Id last = entities.back();

        for (std::size_t index = 0; index < columns.size(); ++index) {
            if (const std::size_t size = sizes[index]; size > 0) {
                std::uint8_t* source = columns[index].data();
                std::copy(source + (entities.size() - 1) * size,
                          source + entities.size() * size,
                          source + row * size);
                columns[index].resize((entities.size() - 1) * size);
            }
        }

        entities[row] = last;
        entities.pop_back();

        state.fetch_add(1, std::memory_order_relaxed);
        return last;
    }

}