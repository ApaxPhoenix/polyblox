#pragma once

#include <cstdint>

#include "device.hpp"

namespace core::graphics::library::drivers {

    struct Context;

    class Vulkan final : public Device {
    public:
        explicit Vulkan(const void* target) noexcept;
        ~Vulkan() override;

        Vulkan(const Vulkan&) = delete;
        Vulkan& operator=(const Vulkan&) = delete;
        Vulkan(Vulkan&&) noexcept = default;
        Vulkan& operator=(Vulkan&&) noexcept = delete;

        void begin(std::uint32_t width, std::uint32_t height, const State& state) noexcept override;
        void submit(const Buffer& stream) noexcept override;
        void end() noexcept override;
        void dispose() noexcept override;

        void shader(std::uint32_t id, const assets::Shader& vertex, const assets::Shader& pixel) noexcept override;

        void mesh(std::uint32_t id, const float* vertices, std::size_t size) noexcept override;
        void update(std::uint32_t id, const float* vertices, std::size_t size) noexcept override;
        void free(std::uint32_t id) noexcept override;

        void texture(std::uint32_t id, const std::uint8_t* pixels, std::uint32_t width, std::uint32_t height) noexcept override;
        void unload(std::uint32_t id) noexcept override;

        void surface(std::uint32_t id, std::uint32_t width, std::uint32_t height) noexcept override;
        void bind(std::uint32_t id) noexcept override;
        void release(std::uint32_t id) noexcept override;
    };

}