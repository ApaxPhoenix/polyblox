#pragma once

namespace core::graphics::library::drivers {

    struct Rect {
        float x{0.0f};
        float y{0.0f};
        float width{0.0f};
        float height{0.0f};
    };

    enum class Pipeline : std::uint16_t {
        Default = 0,
        Vector = 1,
        Opaque = 2,
        Shadow = 3,
        Raycast = 4,
        Deferred = 5,
        Compute = 6,
        Post = 7
    };

    enum class Type : std::uint8_t {
        Rect,
        Circle,
        Mesh,
        Text,
        Scissor,
        Compute,
        Line,
        Volume,
        Instance
    };

    enum Flags : std::uint32_t {
        None = 0,
        Alpha = 1 << 0,
        Test = 1 << 1,
        Write = 1 << 2,
        Cull = 1 << 3,
        Wire = 1 << 4,
        Depth = 1 << 5,
        Stencil = 1 << 6,
        Blend = 1 << 7
    };

    union Key {
        std::uint64_t value;
        struct {
            std::uint64_t entity : 16;
            std::uint64_t depth : 24;
            std::uint64_t texture : 12;
            std::uint64_t pipeline : 8;
            std::uint64_t pass : 4;
        } bits;

        Key() noexcept : value(0) {}

        Key(const Key& other) noexcept : value(other.value) {}

        Key& operator=(const Key& other) noexcept {
            if (this != &other) {
                this->value = other.value;
            }
            return *this;
        }
    };

    struct Command {
        Type type;
        std::uint8_t gap[3]{};
        float box[4]{};

        Command() noexcept : type(Type::Rect), gap{0, 0, 0}, box{0.0f, 0.0f, 0.0f, 0.0f} {}

        Command(const Command& other) noexcept : type(other.type) {
            this->gap[0] = other.gap[0]; this->gap[1] = other.gap[1]; this->gap[2] = other.gap[2];
            this->box[0] = other.box[0]; this->box[1] = other.box[1]; this->box[2] = other.box[2]; this->box[3] = other.box[3];
        }

        Command& operator=(const Command& other) noexcept {
            if (this != &other) {
                this->type = other.type;
                this->gap[0] = other.gap[0]; this->gap[1] = other.gap[1]; this->gap[2] = other.gap[2];
                this->box[0] = other.box[0]; this->box[1] = other.box[1]; this->box[2] = other.box[2]; this->box[3] = other.box[3];
            }
            return *this;
        }
    };

    struct Buffer {
        const Key* keys{nullptr};
        const Command* commands{nullptr};
        std::size_t count{0};
    };

}