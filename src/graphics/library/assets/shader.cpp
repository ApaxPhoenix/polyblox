#include <cstdint>

#include "graphics/library/assets/shader.hpp"

namespace core::graphics::library::assets {

    std::uint32_t Shader::load(std::string_view vertex, std::string_view path) noexcept {
        if (!slots.empty()) {
            const auto id = slots.back();
            slots.pop_back();
            storage[id - 1] = {{vertex.data(), vertex.size()}, {path.data(), path.size()}};
            return id;
        }
        storage.push_back({{vertex.data(), vertex.size()}, {path.data(), path.size()}});
        return static_cast<std::uint32_t>(storage.size());
    }

    const Shader::Block* Shader::get(const std::uint32_t id) const noexcept {
        return (id > 0 && id <= storage.size()) ? &storage[id - 1] : nullptr;
    }

    void Shader::dispose(const std::uint32_t id) noexcept {
        if (id > 0 && id <= storage.size()) {
            if (storage[id - 1].vertex.code != nullptr || storage[id - 1].pixel.code != nullptr) {
                storage[id - 1] = {};
                slots.push_back(id);
            }
        }
    }

    void Shader::clear() noexcept {
        storage.clear();
        slots.clear();
    }

}