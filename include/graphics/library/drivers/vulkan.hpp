#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include "payload.hpp"

namespace core::graphics::library::assets {
    class Shader;
}

namespace core::graphics::library::drivers {

    struct Context;

    static constexpr std::size_t MAX_UNIFORM_SLOTS = 128;

    struct Uniform {
        std::vector<std::uint8_t> buffer;
        bool active{false};
    };

    struct State {
        const float* view{nullptr};
        const float* projection{nullptr};
        Rect region{};
        std::uint32_t active{0};
        std::uint32_t flags{0};
        std::uint32_t pipeline{0};
        const std::array<Uniform, MAX_UNIFORM_SLOTS>* uniforms{nullptr};
    };

    class Vulkan final {
    public:
        explicit Vulkan(const void* target) noexcept;
        ~Vulkan();

        Vulkan(const Vulkan&) = delete;
        Vulkan& operator=(const Vulkan&) = delete;
        Vulkan(Vulkan&&) noexcept = default;
        Vulkan& operator=(Vulkan&&) noexcept = delete;

        void begin(std::uint32_t width, std::uint32_t height, const State& state) const noexcept;
        void submit(const Buffer& stream) const noexcept;
        void end() const noexcept;
        void dispose() noexcept;

        void shader(std::uint32_t identity, const assets::Shader& vertex, const assets::Shader& pixel) const noexcept;

        void mesh(std::uint32_t identity, const float* vertices, std::size_t size) const noexcept;
        void update(std::uint32_t identity, const float* vertices, std::size_t size) const noexcept;
        void free(std::uint32_t identity) const noexcept;

        void texture(std::uint32_t identity, const std::uint8_t* pixels, std::uint32_t width, std::uint32_t height) const noexcept;
        void unload(std::uint32_t identity) const noexcept;

        void surface(std::uint32_t identity, std::uint32_t width, std::uint32_t height) const noexcept;
        void release(std::uint32_t identity) const noexcept;

    private:
        void teardown() noexcept;

        Context* context{nullptr};
    };

}