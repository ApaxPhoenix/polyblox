#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <cstddef>

#include "assets/font.hpp"
#include "assets/mesh.hpp"
#include "assets/shader.hpp"
#include "drivers/vulkan.hpp"
#include "drivers/queue.hpp"
#include "drivers/input.hpp"

namespace core::graphics::library {

    struct Uniform {
        std::vector<std::uint8_t> buffer;
        bool active{false};
    };

    class Context final {
    public:
        explicit Context(const void* window) noexcept;
        ~Context() noexcept = default;

        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;
        Context(Context&&) noexcept = default;
        Context& operator=(Context&&) noexcept = delete;

        void resize(std::uint32_t columns, std::uint32_t rows) noexcept;
        void push(const drivers::Key& key, const drivers::Command& command) noexcept;
        void render() noexcept;
        void flush() noexcept;

        void camera(const float* eye, const float* lens) noexcept;
        void viewport(const drivers::Rect& bounds) noexcept;

        std::uint32_t target(std::uint32_t columns, std::uint32_t rows) noexcept;
        void dispose(std::uint32_t identity) noexcept;

        void flags(std::uint32_t setup) noexcept;
        void program(std::uint32_t asset) noexcept;
        void uniform(std::uint32_t slot, const void* data, std::size_t size) noexcept;

        void key(std::uint32_t code, bool toggle) noexcept;
        void move(float x, float y) noexcept;
        void button(std::uint32_t code, bool toggle) noexcept;
        void scroll(float offset) noexcept;

        [[nodiscard]] const std::array<bool, 512>& inputs() const noexcept { return keys; }
        [[nodiscard]] const std::array<bool, 16>& clicks() const noexcept { return buttons; }
        [[nodiscard]] const std::array<float, 2>& location() const noexcept { return position; }
        [[nodiscard]] const std::array<float, 2>& velocity() const noexcept { return delta; }
        [[nodiscard]] float scroll() const noexcept { return wheel; }

        [[nodiscard]] assets::Texture& textures() noexcept { return textures_; }
        [[nodiscard]] assets::Mesh& meshes() noexcept { return meshes_; }
        [[nodiscard]] assets::Font& fonts() noexcept { return fonts_; }
        [[nodiscard]] assets::Shader& shaders() noexcept { return shaders_; }

    private:
        drivers::Vulkan driver;

        assets::Texture textures_;
        assets::Mesh meshes_;
        assets::Font fonts_;
        assets::Shader shaders_;
        drivers::Queue queue;

        std::uint32_t width{0};
        std::uint32_t height{0};

        std::array<float, 16> view{};
        std::array<float, 16> projection{};
        drivers::Rect area;

        std::vector<drivers::Rect> targets;
        std::uint32_t active{0};

        std::uint32_t settings{0};
        std::uint32_t pipeline{0};
        std::array<Uniform, 128> uniforms;

        std::array<bool, 512> keys{};
        std::array<bool, 16> buttons{};
        std::array<float, 2> position{};
        std::array<float, 2> previous{};
        std::array<float, 2> delta{};
        float wheel{0.0f};
        bool touch{false};
    };

    struct Base {
        void* (*setup)(const void* window);
        void (*resize)(void* handle, std::uint32_t columns, std::uint32_t rows);
        void (*push)(void* handle, std::uint64_t key, const void* command);
        void (*render)(void* handle);
        void (*flush)(void* handle);
        void (*dispose)(void* handle);
    };

    struct State {
        void (*camera)(void* handle, const float* eye, const float* lens);
        void (*viewport)(void* handle, float x, float y, float width, float height);
        void (*flags)(void* handle, std::uint32_t setup);
        void (*program)(void* handle, std::uint32_t asset);
        void (*uniform)(void* handle, std::uint32_t slot, const void* data, std::size_t size);
    };

    struct Texture {
        std::uint32_t (*load)(void* handle, const char* path);
        std::uint32_t (*create)(void* handle, const std::uint8_t* pixels, std::uint32_t width, std::uint32_t height);
        void (*update)(void* handle, std::uint32_t identity, const std::uint8_t* pixels, std::uint32_t width, std::uint32_t height);
        void (*dispose)(void* handle, std::uint32_t identity);
    };

    struct Mesh {
        std::uint32_t (*create)(void* handle, const float* vertices, std::size_t size);
        void (*update)(void* handle, std::uint32_t identity, const float* vertices, std::size_t size);
        void (*dispose)(void* handle, std::uint32_t identity);
    };

    struct Font {
        std::uint32_t (*load)(void* handle, const char* path);
        void (*dispose)(void* handle, std::uint32_t identity);
    };

    struct Shader {
        std::uint32_t (*load)(void* handle, const char* vertex, const char* path);
        void (*dispose)(void* handle, std::uint32_t identity);
    };

    struct Target {
        std::uint32_t (*create)(void* handle, std::uint32_t width, std::uint32_t height);
        void (*dispose)(void* handle, std::uint32_t identity);
    };

    struct Bindings {
        Base base;
        State state;
        Input input;
        Texture texture;
        Mesh mesh;
        Font font;
        Shader shader;
        Target target;
    };

}

#if defined(_WIN32)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
    EXPORT const core::graphics::library::Bindings* bindings() noexcept;
}