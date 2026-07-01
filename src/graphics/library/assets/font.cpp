#include "graphics/library/assets/font.hpp"

namespace core::graphics::library::assets {

    Font::Font(Texture& registry) noexcept : textures(registry) {}

    std::uint32_t Font::load(const std::string_view path) noexcept {
        const auto atlas = textures.load(path);
        if (!slots.empty()) {
            const auto id = slots.back();
            slots.pop_back();
            storage[id - 1] = {atlas, {}};
            return id;
        }
        storage.push_back({atlas, {}});
        return static_cast<std::uint32_t>(storage.size());
    }

    const Font::Block* Font::get(const std::uint32_t id) const noexcept {
        return id > 0 && id <= storage.size() ? &storage[id - 1] : nullptr;
    }

    void Font::dispose(const std::uint32_t id) noexcept {
        if (id > 0 && id <= storage.size()) {
            if (storage[id - 1].atlas != 0 || !storage[id - 1].glyphs.empty()) {
                if (storage[id - 1].atlas != 0) {
                    textures.dispose(storage[id - 1].atlas);
                }
                storage[id - 1] = {};
                slots.push_back(id);
            }
        }
    }

    void Font::clear() noexcept {
        for (auto& [atlas, glyphs] : storage) {
            if (atlas != 0) {
                textures.dispose(atlas);
            }
        }
        storage.clear();
        slots.clear();
    }

}