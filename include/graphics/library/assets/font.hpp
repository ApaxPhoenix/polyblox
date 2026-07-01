#pragma once

#include <string_view>
#include <vector>
#include <cstdint>

#include "texture.hpp"

namespace core::graphics::library::assets {

    class Font final {
    public:
        struct Glyph {
            char32_t code;
            float uv[4];
            float size[2];
            float bearing[2];
            float advance;
        };

        struct Block {
            std::uint32_t atlas{0};
            std::vector<Glyph> glyphs;
        };

        explicit Font(Texture& registry) noexcept;
        ~Font() noexcept = default;

        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;
        Font(Font&&) noexcept = default;
        Font& operator=(Font&&) noexcept = delete;

        std::uint32_t load(std::string_view path) noexcept;
        [[nodiscard]] const Block* get(std::uint32_t id) const noexcept;
        void dispose(std::uint32_t id) noexcept;
        void clear() noexcept;

    private:
        Texture& textures;
        std::vector<Block> storage;
        std::vector<std::uint32_t> slots;
    };

}