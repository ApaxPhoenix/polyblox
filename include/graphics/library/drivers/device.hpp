#pragma once

#include <vector>
#include <array>

#include "payload.hpp"

namespace core::graphics::library::assets {
    class Shader;
}

namespace core::graphics::library::drivers {

    static constexpr std::size_t MAX_UNIFORM_SLOTS = 128;

    struct UniformBlock {
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
        // Point to the stable, flat allocation array
        const std::array<UniformBlock, MAX_UNIFORM_SLOTS>* uniforms{nullptr};
    };

    class Device {
    public:
        virtual ~Device() = default;

        virtual void begin(std::uint32_t width, std::uint32_t height, const State& state) noexcept = 0;
        virtual void submit(const Buffer& stream) noexcept = 0;
        virtual void end() noexcept = 0;
        virtual void dispose() noexcept = 0;

        virtual void shader(std::uint32_t id, const assets::Shader& vertex, const assets::Shader& pixel) noexcept = 0;

        virtual void mesh(std::uint32_t id, const float* vertices, std::size_t size) noexcept = 0;
        virtual void update(std::uint32_t id, const float* vertices, std::size_t size) noexcept = 0;
        virtual void free(std::uint32_t id) noexcept = 0;

        virtual void texture(std::uint32_t id, const std::uint8_t* pixels, std::uint32_t width, std::uint32_t height) noexcept = 0;
        virtual void unload(std::uint32_t id) noexcept = 0;

        virtual void surface(std::uint32_t id, std::uint32_t width, std::uint32_t height) noexcept = 0;
        virtual void bind(std::uint32_t id) noexcept = 0;
        virtual void release(std::uint32_t id) noexcept = 0;
    };

}
