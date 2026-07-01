#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

namespace core::graphics::library::assets {

    class Mesh final {
    public:
        struct Block {
            const float* allocation{nullptr};
            std::size_t size{0};
        };

        Mesh() noexcept = default;
        ~Mesh() noexcept = default;

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&&) noexcept = default;
        Mesh& operator=(Mesh&&) noexcept = delete;

        std::uint32_t create(const float* vertices, std::size_t size) noexcept;
        void update(std::uint32_t id, const float* vertices, std::size_t size) noexcept;
        [[nodiscard]] const Block* get(std::uint32_t id) const noexcept;
        void dispose(std::uint32_t id) noexcept;
        void clear() noexcept;

    private:
        std::vector<Block> storage;
        std::vector<std::uint32_t> slots;
    };

}