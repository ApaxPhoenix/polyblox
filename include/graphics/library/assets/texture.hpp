#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <cstdint>

namespace core::graphics::library::assets {

    class Texture final {
    public:
        struct Block {
            const void* allocation{nullptr};
            std::uint32_t width{0};
            std::uint32_t height{0};
        };

        Texture() noexcept = default;
        ~Texture() noexcept = default;

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) noexcept = default;
        Texture& operator=(Texture&&) noexcept = delete;

        std::uint32_t load(std::string_view path) noexcept;
        std::uint32_t create(const std::uint8_t* pixels, std::uint32_t width, std::uint32_t height) noexcept;
        void update(std::uint32_t id, const std::uint8_t* pixels, std::uint32_t width, std::uint32_t height) noexcept;
        [[nodiscard]] const Block* get(std::uint32_t id) const noexcept;
        void dispose(std::uint32_t id) noexcept;
        void clear() noexcept;

    private:
        std::vector<Block> storage;
        std::unordered_map<std::string, std::uint32_t> registry;
        std::vector<std::uint32_t> slots;
    };

}