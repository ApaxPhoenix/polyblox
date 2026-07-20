#pragma once

#include <cstdint>

namespace core::graphics::library {

    struct Input {
        void (*key)(void* handle, std::uint32_t code, bool pressed);
        void (*move)(void* handle, float x, float y);
        void (*button)(void* handle, std::uint32_t code, bool pressed);
        void (*scroll)(void* handle, float offset);
    };

}
