#pragma once

#include <vector>
#include <string_view>
#include <cstddef>

namespace core::graphics::library::assets {

    class Shader final {
    public:
        struct Program {
            const void* code{nullptr};
            std::size_t size{0};
        };

        struct Block {
            Program vertex;
            Program pixel;
        };

        Shader() noexcept = default;
        ~Shader() noexcept = default;

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) noexcept = default;
        Shader& operator=(Shader&&) noexcept = delete;

        std::uint32_t load(std::string_view vertex, std::string_view path) noexcept;
        [[nodiscard]] const Block* get(std::uint32_t id) const noexcept;
        void dispose(std::uint32_t id) noexcept;
        void clear() noexcept;

    private:
        std::vector<Block> storage;
        std::vector<std::uint32_t> slots;
    };

}