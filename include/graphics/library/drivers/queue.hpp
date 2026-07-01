#pragma once

#include <vector>

#include "payload.hpp"

namespace core::graphics::library::drivers {

    class Queue final {
    public:
        Queue() noexcept = default;
        ~Queue() noexcept = default;

        Queue(const Queue&) = delete;
        Queue& operator=(const Queue&) = delete;
        Queue(Queue&&) noexcept = default;
        Queue& operator=(Queue&&) noexcept = delete;

        void push(const Key& key, const Command& command) noexcept;
        Buffer flush() noexcept;
        void clear() noexcept;
        void sort() noexcept;

    private:
        std::vector<Key> keys;
        std::vector<Command> commands;

        std::vector<Key> scratch;
        std::vector<Command> sorted;

        Buffer buffer{};
    };

}