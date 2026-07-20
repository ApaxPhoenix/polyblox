#include <sol/sol.hpp>
#include <array>
#include <string>

#include "vm/api/input.hpp"
#include "graphics/library/context.hpp"
#include "core/primitives/vector2.hpp"

using namespace core::graphics::library;
using namespace core::ecs::primitives;

namespace {
    struct Cache {
        std::array<bool, 512> keys{};
        std::array<bool, 16> buttons{};
        float wheel{0.0f};
    };

    Cache history;
}

void input(lua_State* state, const Context* context) {
    sol::state_view view(state);
    sol::table table = view.create_table();

    sol::table keyboard = view.create_table();
    sol::table mouse = view.create_table();
    sol::table gesture = view.create_table();

    for (int code = 65; code <= 90; ++code) {
        std::string name(1, static_cast<char>(code));
        keyboard[name] = code;
    }
    for (int code = 48; code <= 57; ++code) {
        std::string name(1, static_cast<char>(code));
        keyboard[name] = code;
    }

    keyboard["Space"] = 32;
    keyboard["Left"] = 263;
    keyboard["Right"] = 264;
    keyboard["Up"] = 265;
    keyboard["Down"] = 266;

#if defined(_WIN32)
    keyboard["Escape"] = 256;
    keyboard["Enter"] = 257;
    keyboard["Tab"] = 258;
    keyboard["Shift"] = 259;
    keyboard["Control"] = 260;
    keyboard["Alt"] = 261;
    keyboard["Windows"] = 262;
#elif defined(__APPLE__)
    keyboard["Escape"] = 256;
    keyboard["Return"] = 257;
    keyboard["Tab"] = 258;
    keyboard["Shift"] = 259;
    keyboard["Control"] = 260;
    keyboard["Option"] = 261;
    keyboard["Command"] = 262;
#else
    keyboard["Escape"] = 256;
    keyboard["Enter"] = 257;
    keyboard["Tab"] = 258;
    keyboard["Shift"] = 259;
    keyboard["Control"] = 260;
    keyboard["Alt"] = 261;
    keyboard["Super"] = 262;
#endif

    mouse["Left"] = 512;
    mouse["Right"] = 513;
    mouse["Middle"] = 514;
    mouse["M1"] = 515;
    mouse["M2"] = 516;

    mouse["position"] = [context]() {
        return Vector2(context->location()[0], context->location()[1]);
    };

    mouse["delta"] = [context]() {
        return Vector2(context->velocity()[0], context->velocity()[1]);
    };

    mouse["scroll"] = [context]() {
        return context->scroll();
    };

    mouse["scrolling"] = [context]() {
        return context->scroll() != history.wheel;
    };

    gesture["Touch"] = 512;

    table["Keyboard"] = keyboard;
    table["Mouse"] = mouse;
    table["Gesture"] = gesture;

    table["down"] = [context](const std::uint32_t code) {
        if (code < 512) return context->inputs()[code];
        if (code < 528) return context->clicks()[code - 512];
        return false;
    };

    table["release"] = [context](const std::uint32_t code) {
        if (code < 512) return !context->inputs()[code] && history.keys[code];
        if (code < 528) return !context->clicks()[code - 512] && history.buttons[code - 512];
        return false;
    };

    view["Input"] = table;
}