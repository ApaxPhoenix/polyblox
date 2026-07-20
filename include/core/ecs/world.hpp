#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

#include "archetype.hpp"
#include "query.hpp"

namespace core::ecs {

    class World {
    public:
        World() = default;
        ~World() = default;

        World(const World&) = delete;
        World& operator=(const World&) = delete;
        World(World&&) noexcept = default;
        World& operator=(World&&) noexcept = default;

        std::uint32_t component(const std::string& name, std::size_t size = 0);
        Id spawn();
        Id clone(Id id);
        void dispose(Id id);
        void clear();

        void attach(Id entity, Id parent);
        [[nodiscard]] Id parent(Id id) const;
        [[nodiscard]] const std::vector<Id>& children(Id id) const;

        void* set(Id id, std::uint32_t type, const void* data = nullptr);
        [[nodiscard]] void* get(Id id, std::uint32_t type) const;
        [[nodiscard]] bool has(Id id, std::uint32_t type) const;
        void remove(Id id, std::uint32_t type);

        void global(std::uint32_t type, const void* data, std::size_t size);
        [[nodiscard]] void* global(std::uint32_t type);

        using Task = std::function<void(Id id)>;
        void wait(std::uint32_t type, const Task& action);

        void batch(const std::function<void()>& flow);
        [[nodiscard]] Query query() const;

        static void loop(const Query& query, const Query::Callback& action);

    private:
        Archetype* view(const Mask& mask);
        void move(Id id, const Mask& mask);

        std::uint32_t head = 0xFFFFFFFF;
        std::uint32_t tail = 0xFFFFFFFF;

        std::vector<std::unique_ptr<Archetype>> kinds;
        std::vector<Slot> slots;
        std::vector<Id> free;
        std::vector<std::size_t> sizes;
        std::unordered_map<std::string, std::uint32_t> names;
        std::unordered_map<std::uint32_t, std::vector<std::uint8_t>> cache;
        std::unordered_map<std::uint32_t, Task> events;
        std::vector<std::function<void()>> steps;
        bool lock = false;
        Id next = 0;
    };

}