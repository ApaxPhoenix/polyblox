#include "graphics/library/context.hpp"
#include "graphics/library/drivers/vulkan.hpp"
#include "graphics/library/drivers/directx.hpp"
#include <cstring>

namespace core::graphics::library {

    Context::Context(const Api api, const void* target) noexcept : fonts_(textures_) {
        if (api == Api::Vulkan) {
            device = std::make_unique<drivers::Vulkan>(target);
        } else if (api == Api::DirectX) {
            device = std::make_unique<drivers::DirectX>(target);
        }
        for (auto& block : uniforms) {
            block.buffer.reserve(256);
            block.active = false;
        }
    }

    void Context::resize(std::uint32_t width, std::uint32_t height) noexcept {
        this->width = width;
        this->height = height;
    }

    void Context::push(const drivers::Key& key, const drivers::Command& command) noexcept {
        queue.push(key, command);
    }

    void Context::render() noexcept {
        if (device) {
            drivers::State state_struct{};
            state_struct.view = view.data();
            state_struct.projection = projection.data();
            state_struct.region = area;
            state_struct.active = active;
            state_struct.flags = settings;
            state_struct.pipeline = pipeline;
            device->begin(width, height, state_struct);
            device->submit(queue.flush());
            device->end();
        }
    }

    void Context::flush() noexcept {
        queue.clear();
        for (auto& block : uniforms) {
            block.active = false;
        }
    }

    void Context::camera(const float* view, const float* projection) noexcept {
        if (view) {
            std::memcpy(this->view.data(), view, sizeof(float) * 16);
        }
        if (projection) {
            std::memcpy(this->projection.data(), projection, sizeof(float) * 16);
        }
    }

    void Context::viewport(const drivers::Rect& bounds) noexcept {
        area = bounds;
    }

    std::uint32_t Context::target(std::uint32_t width, std::uint32_t height) noexcept {
    targets.push_back({0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)});

    const auto id = static_cast<std::uint32_t>(targets.size());
    if (device) {
        device->surface(id, width, height);
    }
    return id;
}
    void Context::bind(const std::uint32_t id) noexcept {
        active = id;
        if (device) {
            if (id == 0) {
                device->release(id);
            } else {
                device->bind(id);
            }
        }
    }

    void Context::dispose(const std::uint32_t id) noexcept {
        if (id > 0 && id <= targets.size()) {
            targets[id - 1] = {};
            if (device) {
                device->unload(id);
            }
        }
    }

    void Context::flags(const std::uint32_t setup) noexcept {
        settings = setup;
    }

    void Context::program(const std::uint32_t asset) noexcept {
        pipeline = asset;
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

    static void* base_setup(std::uint32_t api, const void* window) {
        return new Context(static_cast<Api>(api), window);
    }

    static void base_resize(void* handle, const std::uint32_t width, const std::uint32_t height) {
        if (handle) {
            static_cast<Context*>(handle)->resize(width, height);
        }
    }

    static void base_push(void* handle, const std::uint64_t key, const void* command) {
        if (handle && command) {
            drivers::Key k;
            k.value = key;
            static_cast<Context*>(handle)->push(k, *static_cast<const drivers::Command*>(command));
        }
    }

    static void base_render(void* handle) {
        if (handle) {
            static_cast<Context*>(handle)->render();
        }
    }

    static void base_flush(void* handle) {
        if (handle) {
            static_cast<Context*>(handle)->flush();
        }
    }

    static void base_dispose(void* handle) {
        if (handle) {
            delete static_cast<Context*>(handle);
        }
    }

    static void state_camera(void* handle, const float* view, const float* projection) {
        if (handle) {
            static_cast<Context*>(handle)->camera(view, projection);
        }
    }

    static void state_viewport(void* handle, const float x, const float y, const float width, const float height) {
        if (handle) {
            const drivers::Rect rect{x, y, width, height};
            static_cast<Context*>(handle)->viewport(rect);
        }
    }

    static void state_flags(void* handle, const std::uint32_t setup) {
        if (handle) {
            static_cast<Context*>(handle)->flags(setup);
        }
    }

    static void state_program(void* handle, const std::uint32_t asset) {
        if (handle) {
            static_cast<Context*>(handle)->program(asset);
        }
    }

    static void state_uniform(void* handle, const std::uint32_t slot, const void* data, const std::size_t size) {
        if (handle) {
            static_cast<Context*>(handle)->uniform(slot, data, size);
        }
    }

    static std::uint32_t texture_load(void* handle, const char* path) {
        return (handle && path) ? static_cast<Context*>(handle)->textures().load(path) : 0;
    }

    static std::uint32_t texture_create(void* handle, const std::uint8_t* pixels, const std::uint32_t width, const std::uint32_t height) {
        return handle ? static_cast<Context*>(handle)->textures().create(pixels, width, height) : 0;
    }

    static void texture_update(void* handle, const std::uint32_t id, const std::uint8_t* pixels, const std::uint32_t width, const std::uint32_t height) {
        if (handle) {
            static_cast<Context*>(handle)->textures().update(id, pixels, width, height);
        }
    }

    static void texture_dispose(void* handle, const std::uint32_t id) {
        if (handle) {
            static_cast<Context*>(handle)->textures().dispose(id);
        }
    }

    static std::uint32_t mesh_create(void* handle, const float* vertices, const std::size_t size) {
        return handle ? static_cast<Context*>(handle)->meshes().create(vertices, size) : 0;
    }

    static void mesh_update(void* handle, const std::uint32_t id, const float* vertices, const std::size_t size) {
        if (handle) {
            static_cast<Context*>(handle)->meshes().update(id, vertices, size);
        }
    }

    static void mesh_dispose(void* handle, const std::uint32_t id) {
        if (handle) {
            static_cast<Context*>(handle)->meshes().dispose(id);
        }
    }

    static std::uint32_t font_load(void* handle, const char* path) {
        return handle && path ? static_cast<Context*>(handle)->fonts().load(path) : 0;
    }

    static void font_dispose(void* handle, const std::uint32_t id) {
        if (handle) {
            static_cast<Context*>(handle)->fonts().dispose(id);
        }
    }

    static std::uint32_t shader_load(void* handle, const char* vertex, const char* path) {
        return handle && vertex && path ? static_cast<Context*>(handle)->shaders().load(vertex, path) : 0;
    }

    static void shader_dispose(void* handle, const std::uint32_t id) {
        if (handle) {
            static_cast<Context*>(handle)->shaders().dispose(id);
        }
    }

    static std::uint32_t target_create(void* handle, const std::uint32_t width, const std::uint32_t height) {
        return handle ? static_cast<Context*>(handle)->target(width, height) : 0;
    }

    static void target_bind(void* handle, const std::uint32_t id) {
        if (handle) {
            static_cast<Context*>(handle)->bind(id);
        }
    }

    static void target_dispose(void* handle, const std::uint32_t id) {
        if (handle) {
            static_cast<Context*>(handle)->dispose(id);
        }
    }

}

extern "C" {
    EXPORT const core::graphics::library::Bindings* bindings() noexcept {
        static constexpr core::graphics::library::Bindings instance{
            { core::graphics::library::base_setup, core::graphics::library::base_resize, core::graphics::library::base_push, core::graphics::library::base_render, core::graphics::library::base_flush, core::graphics::library::base_dispose },
            { core::graphics::library::state_camera, core::graphics::library::state_viewport, core::graphics::library::state_flags, core::graphics::library::state_program, core::graphics::library::state_uniform },
            { core::graphics::library::texture_load, core::graphics::library::texture_create, core::graphics::library::texture_update, core::graphics::library::texture_dispose },
            { core::graphics::library::mesh_create, core::graphics::library::mesh_update, core::graphics::library::mesh_dispose },
            { core::graphics::library::font_load, core::graphics::library::font_dispose },
            { core::graphics::library::shader_load, core::graphics::library::shader_dispose },
            { core::graphics::library::target_create, core::graphics::library::target_bind, core::graphics::library::target_dispose }
        };
        return &instance;
    }
}