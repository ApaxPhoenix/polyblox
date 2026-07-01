#pragma once

#include <cstdint>

#include "graphics/library/assets/shader.hpp"
#include "device.hpp"

namespace core::graphics::library::assets {
    class Shader;
}

namespace core::graphics::library::drivers {

    class Context;

    class Metal final : public Device {
    public:
        explicit Metal(const void* target) noexcept;
        ~Metal() override;

        Metal(const Metal&) = delete;
        Metal& operator=(const Metal&) = delete;
        Metal(Metal&&) noexcept = default;
        Metal& operator=(Metal&&) noexcept = delete;

        void begin(std::uint32_t width, std::uint32_t height, const State& state) noexcept override;
        void submit(const Buffer& stream) noexcept override;
        void end() noexcept override;
        void dispose() noexcept override;

        void shader(std::uint32_t handle, const assets::Shader& vertex, const assets::Shader& pixel) noexcept override;

        void mesh(std::uint32_t handle, const float* data, std::size_t size) noexcept override;
        void update(std::uint32_t handle, const float* data, std::size_t size) noexcept override;
        void free(std::uint32_t handle) noexcept override;

        void texture(std::uint32_t handle, const std::uint8_t* pixels, std::uint32_t width, std::uint32_t height) noexcept override;
        void unload(std::uint32_t handle) noexcept override;

        void surface(std::uint32_t handle, std::uint32_t width, std::uint32_t height) noexcept override;
        void bind(std::uint32_t handle) noexcept override;
        void release(std::uint32_t handle) noexcept override;
    };

}