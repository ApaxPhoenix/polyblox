#include <cstring>

#include "graphics/library/context.hpp"
#include "graphics/library/drivers/vulkan.hpp"

namespace core::graphics::library {

    Context::Context(const void* window) noexcept
        : driver(window), fonts_(textures_) {
        for (auto&[bytes, valid] : uniforms) {
            bytes.reserve(256);
            valid = false;
        }
    }

    void Context::resize(const std::uint32_t columns, const std::uint32_t rows) noexcept {
        this->width = columns;
        this->height = rows;
    }

    void Context::push(const drivers::Key& key, const drivers::Command& command) noexcept {
        this->queue.push(key, command);
    }

    void Context::render() noexcept {
        drivers::State state{};
        state.view = view.data();
        state.projection = projection.data();
        state.region = area;
        state.active = active;
        state.flags = settings;
        state.pipeline = pipeline;
        state.uniforms = reinterpret_cast<const std::array<drivers::Uniform, drivers::MAX_UNIFORM_SLOTS>*>(&uniforms);

        driver.begin(width, height, state);
        driver.submit(queue.flush());
        driver.end();
    }

    void Context::flush() noexcept {
        queue.clear();
        for (auto&[bytes, valid] : uniforms) {
            valid = false;
        }
    }

    void Context::camera(const float* eye, const float* lens) noexcept {
        if (eye) {
            std::memcpy(this->view.data(), eye, sizeof(float) * 16);
        }
        if (lens) {
            std::memcpy(this->projection.data(), lens, sizeof(float) * 16);
        }
    }

    void Context::viewport(const drivers::Rect& bounds) noexcept {
        area = bounds;
    }

    std::uint32_t Context::target(const std::uint32_t columns, const std::uint32_t rows) noexcept {
        targets.push_back({0.0f, 0.0f, static_cast<float>(columns), static_cast<float>(rows)});

        const auto identity = static_cast<std::uint32_t>(targets.size());
        driver.surface(identity, columns, rows);
        return identity;
    }

    void Context::dispose(const std::uint32_t identity) noexcept {
        if (identity > 0 && identity <= targets.size()) {
            targets[identity - 1] = {};
            driver.unload(identity);
        }
    }

    void Context::flags(const std::uint32_t setup) noexcept {
        this->settings = setup;
    }

    void Context::program(const std::uint32_t asset) noexcept {
        this->pipeline = asset;
    }

    void Context::uniform(const std::uint32_t slot, const void* data, const std::size_t size) noexcept {
        if (!data || size == 0 || slot >= uniforms.size()) [[unlikely]] {
            return;
        }
        auto& block = uniforms[slot];
        if (block.buffer.capacity() < size) [[unlikely]] {
            block.buffer.reserve(size * 2);
        }
        block.buffer.resize(size);
        std::memcpy(block.buffer.data(), data, size);
        block.active = true;
    }

    void Context::key(const std::uint32_t code, const bool toggle) noexcept {
        if (code < keys.size()) [[likely]] {
            keys[code] = toggle;
        }
    }

    void Context::move(const float x, const float y) noexcept {
        previous = position;
        position[0] = x;
        position[1] = y;
        delta[0] = position[0] - previous[0];
        delta[1] = position[1] - previous[1];
    }

    void Context::button(const std::uint32_t code, const bool toggle) noexcept {
        if (code < buttons.size()) [[likely]] {
            buttons[code] = toggle;
        }
        if (code == 0) {
            touch = toggle;
        }
    }

    void Context::scroll(const float offset) noexcept {
        wheel += offset;
    }

}

extern "C" {
    EXPORT const core::graphics::library::Bindings* bindings() noexcept {
        using namespace core::graphics::library;

        static constexpr Bindings instance{
            {
                [](const void* window) -> void* { return new Context(window); },
                [](void* handle, const std::uint32_t columns, const std::uint32_t rows) { if (handle) static_cast<Context*>(handle)->resize(columns, rows); },
                [](void* handle, const std::uint64_t key, const void* command) { if (handle && command) static_cast<Context*>(handle)->push(*reinterpret_cast<const drivers::Key*>(&key), *static_cast<const drivers::Command*>(command)); },
                [](void* handle) { if (handle) static_cast<Context*>(handle)->render(); },
                [](void* handle) { if (handle) static_cast<Context*>(handle)->flush(); },
                [](void* handle) { if (handle) delete static_cast<Context*>(handle); }
            },
            {
                [](void* handle, const float* eye, const float* lens) { if (handle) static_cast<Context*>(handle)->camera(eye, lens); },
                [](void* handle, const float x, const float y, const float width, const float height) { if (handle) static_cast<Context*>(handle)->viewport(drivers::Rect{x, y, width, height}); },
                [](void* handle, const std::uint32_t setup) { if (handle) static_cast<Context*>(handle)->flags(setup); },
                [](void* handle, const std::uint32_t asset) { if (handle) static_cast<Context*>(handle)->program(asset); },
                [](void* handle, const std::uint32_t slot, const void* data, const std::size_t size) { if (handle) static_cast<Context*>(handle)->uniform(slot, data, size); }
            },
            {
                [](void* handle, const std::uint32_t code, const bool toggled) { if (handle) static_cast<Context*>(handle)->key(code, toggled); },
                [](void* handle, const float x, const float y) { if (handle) static_cast<Context*>(handle)->move(x, y); },
                [](void* handle, const std::uint32_t code, const bool toggled) { if (handle) static_cast<Context*>(handle)->button(code, toggled); },
                [](void* handle, const float offset) { if (handle) static_cast<Context*>(handle)->scroll(offset); }
            },
            {
                [](void* handle, const char* path) -> std::uint32_t { return handle && path ? static_cast<Context*>(handle)->textures().load(path) : 0; },
                [](void* handle, const std::uint8_t* pixels, const std::uint32_t width, const std::uint32_t height) -> std::uint32_t { return handle ? static_cast<Context*>(handle)->textures().create(pixels, width, height) : 0; },
                [](void* handle, const std::uint32_t identity, const std::uint8_t* pixels, const std::uint32_t width, const std::uint32_t height) { if (handle) static_cast<Context*>(handle)->textures().update(identity, pixels, width, height); },
                [](void* handle, const std::uint32_t identity) { if (handle) static_cast<Context*>(handle)->textures().dispose(identity); }
            },
            {
                [](void* handle, const float* vertices, const std::size_t size) -> std::uint32_t { return handle ? static_cast<Context*>(handle)->meshes().create(vertices, size) : 0; },
                [](void* handle, const std::uint32_t identity, const float* vertices, const std::size_t size) { if (handle) static_cast<Context*>(handle)->meshes().update(identity, vertices, size); },
                [](void* handle, const std::uint32_t identity) { if (handle) static_cast<Context*>(handle)->meshes().dispose(identity); }
            },
            {
                [](void* handle, const char* path) -> std::uint32_t { return handle && path ? static_cast<Context*>(handle)->fonts().load(path) : 0; },
                [](void* handle, const std::uint32_t identity) { if (handle) static_cast<Context*>(handle)->fonts().dispose(identity); }
            },
            {
                [](void* handle, const char* vertex, const char* path) -> std::uint32_t { return handle && vertex && path ? static_cast<Context*>(handle)->shaders().load(vertex, path) : 0; },
                [](void* handle, const std::uint32_t identity) { if (handle) static_cast<Context*>(handle)->shaders().dispose(identity); }
            },
            {
                [](void* handle, const std::uint32_t width, const std::uint32_t height) -> std::uint32_t { return handle ? static_cast<Context*>(handle)->target(width, height) : 0; },
                [](void* handle, const std::uint32_t identity) { if (handle) static_cast<Context*>(handle)->dispose(identity); }
            }
        };
        return &instance;
    }
}