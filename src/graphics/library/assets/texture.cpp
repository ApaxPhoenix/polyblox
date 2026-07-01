#include "graphics/library/assets/texture.hpp"

namespace core::graphics::library::assets {

    std::uint32_t Texture::load(const std::string_view path) noexcept {
        const std::string key(path);
        if (const auto item = registry.find(key); item != registry.end()) {
            return item->second;
        }
        if (!slots.empty()) {
            const auto id = slots.back();
            slots.pop_back();
            storage[id - 1] = {nullptr, 512, 512};
            registry[key] = id;
            return id;
        }
        storage.push_back({nullptr, 512, 512});
        const auto id = static_cast<std::uint32_t>(storage.size());
        registry[key] = id;
        return id;
    }

    std::uint32_t Texture::create(const std::uint8_t* pixels, const std::uint32_t width, const std::uint32_t height) noexcept {
        if (!slots.empty()) {
            const auto id = slots.back();
            slots.pop_back();
            storage[id - 1] = {pixels, width, height};
            return id;
        }
        storage.push_back({pixels, width, height});
        return static_cast<std::uint32_t>(storage.size());
    }

    void Texture::update(const std::uint32_t id, const std::uint8_t* pixels, const std::uint32_t width, const std::uint32_t height) noexcept {
        if (id > 0 && id <= storage.size()) {
            storage[id - 1] = {pixels, width, height};
        }
    }

    const Texture::Block* Texture::get(const std::uint32_t id) const noexcept {
        return (id > 0 && id <= storage.size()) ? &storage[id - 1] : nullptr;
    }

    void Texture::dispose(const std::uint32_t id) noexcept {
        if (id > 0 && id <= storage.size()) {
            if (storage[id - 1].allocation != nullptr || storage[id - 1].width != 0 || storage[id - 1].height != 0) {
                storage[id - 1] = {};
                for (auto it = registry.begin(); it != registry.end(); ++it) {
                    if (it->second == id) {
                        registry.erase(it);
                        break;
                    }
                }
                slots.push_back(id);
            }
        }
    }

    void Texture::clear() noexcept {
        storage.clear();
        registry.clear();
        slots.clear();
    }

}