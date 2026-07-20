#include <cstring>
#include <mutex>
#include <shared_mutex>

#include "core/ecs/world.hpp"

namespace core::ecs {

    std::uint32_t World::component(const std::string& name, const std::size_t size) {
        if (const auto find = names.find(name); find != names.end()) {
            return find->second;
        }
        const auto id = static_cast<std::uint32_t>(sizes.size());
        sizes.push_back(size);
        names[name] = id;

        if (name == "parent") {
            head = id;
        } else if (name == "children") {
            tail = id;
        }
        return id;
    }

    Id World::spawn() {
        Id id;
        if (!free.empty()) {
            id = free.back();
            free.pop_back();
        } else {
            id = next++;
        }
        if (id >= slots.size()) {
            slots.resize(id + 1);
        }
        const Mask blank;
        Archetype* archetype = view(blank);
        archetype->allocate(id);
        slots[id] = {archetype, archetype->entities.size() - 1};
        return id;
    }

    Id World::clone(const Id id) {
        const Id duplicate = spawn();
        auto [archetype, source_row] = slots[id];
        std::shared_lock lock_shared(archetype->mutex);

        const Mask mask = archetype->signature;
        move(duplicate, mask);

        auto [goal, dest_row] = slots[duplicate];
        std::unique_lock lock_unique(goal->mutex);

        for (auto const& [component, track] : archetype->offsets) {
            if (const std::size_t size = sizes[component]; size > 0) {
                const std::size_t offset = goal->offsets[component];
                std::memcpy(goal->columns[offset].data() + dest_row * size,
                            archetype->columns[track].data() + source_row * size,
                            size);
            }
        }
        return duplicate;
    }

    void World::dispose(Id id) {
        if (lock) {
            steps.emplace_back([this, id]() { dispose(id); });
            return;
        }
        auto [archetype, row] = slots[id];
        if (!archetype) return;

        if (tail != 0xFFFFFFFF && has(id, tail)) {
            if (const auto* list = static_cast<std::vector<Id>*>(get(id, tail))) {
                for (const Id nested : *list) {
                    dispose(nested);
                }
            }
        }

        if (const std::uint32_t moved = archetype->dispose(row); moved != id) {
            slots[moved].row = row;
        }
        slots[id] = {nullptr, 0};
        free.push_back(id);
    }

    void World::clear() {
        kinds.clear();
        slots.clear();
        free.clear();
        cache.clear();
        events.clear();
        steps.clear();
        next = 0;
        lock = false;
    }

    void World::attach(const Id entity, const Id parent) {
        if (head == 0xFFFFFFFF) component("parent", sizeof(Id));
        if (tail == 0xFFFFFFFF) component("children", sizeof(std::vector<Id>));

        set(entity, head, &parent);

        if (!has(parent, tail)) {
            std::vector<Id> list;
            list.push_back(entity);
            set(parent, tail, &list);
        } else {
            auto* list = static_cast<std::vector<Id>*>(get(parent, tail));
            list->push_back(entity);
        }
    }

    Id World::parent(const Id id) const {
        if (head == 0xFFFFFFFF || !has(id, head)) return Null;
        return *static_cast<const Id*>(this->get(id, head));
    }

    const std::vector<Id>& World::children(const Id id) const {
        static constexpr std::vector<Id> empty;
        if (tail == 0xFFFFFFFF || !has(id, tail)) return empty;
        return *static_cast<const std::vector<Id>*>(this->get(id, tail));
    }

    void* World::set(Id id, std::uint32_t type, const void* data) {
        if (lock) {
            steps.emplace_back([this, id, type, data]() { set(id, type, data); });
            return nullptr;
        }
        auto [archetype, row] = slots[id];
        Mask mask = archetype->signature;
        mask.set(type);

        move(id, mask);

        void* pointer = get(id, type);
        if (data && pointer) {
            std::memcpy(pointer, data, sizes[type]);
        }

        if (const auto find = events.find(type); find != events.end()) {
            find->second(id);
        }
        return pointer;
    }

    void* World::get(const Id id, const std::uint32_t type) const {
        auto [archetype, row] = slots[id];
        if (!archetype) return nullptr;

        const auto find = archetype->offsets.find(type);
        if (find == archetype->offsets.end()) return nullptr;

        const std::size_t size = sizes[type];
        if (size == 0) return nullptr;
        return archetype->columns[find->second].data() + row * size;
    }

    bool World::has(const Id id, const std::uint32_t type) const {
        auto [archetype, row] = slots[id];
        if (!archetype) return false;
        return archetype->signature.test(type);
    }

    void World::remove(Id id, std::uint32_t type) {
        if (lock) {
            steps.emplace_back([this, id, type]() { remove(id, type); });
            return;
        }
        auto [archetype, row] = slots[id];
        Mask mask = archetype->signature;
        mask.reset(type);
        move(id, mask);
    }

    void World::global(const std::uint32_t type, const void* data, const std::size_t size) {
        auto& resource = cache[type];
        resource.resize(size);
        if (data && size > 0) {
            std::memcpy(resource.data(), data, size);
        }
    }

    void* World::global(const std::uint32_t type) {
        const auto find = cache.find(type);
        if (find == cache.end()) return nullptr;
        return find->second.data();
    }

    void World::wait(const std::uint32_t type, const Task& action) {
        events[type] = action;
    }

    void World::batch(const std::function<void()>& flow) {
        lock = true;
        flow();
        lock = false;
        for (const auto& task : steps) {
            task();
        }
        steps.clear();
    }

    Query World::query() const {
        return Query(kinds);
    }

    void World::loop(const Query& query, const Query::Callback& action) {
        query.process(action, query.results());
    }

    Archetype* World::view(const Mask& mask) {
        for (const auto& archetype : kinds) {
            if (archetype->signature == mask) return archetype.get();
        }
        auto archetype = std::make_unique<Archetype>();
        archetype->signature = mask;
        for (std::uint32_t tag = 0; tag < Capacity; ++tag) {
            if (mask.test(tag)) {
                archetype->offsets[tag] = archetype->columns.size();
                archetype->columns.emplace_back();
                archetype->sizes.push_back(sizes[tag]);
            }
        }

        kinds.push_back(std::move(archetype));
        return kinds.back().get();
    }

    void World::move(const Id id, const Mask& mask) {
        auto [type, row] = slots[id];
        Archetype* old = type;
        if (old->signature == mask) return;

        Archetype* goal = view(mask);
        goal->allocate(id);

        auto [dest_archetype, dest_row] = Slot{goal, goal->entities.size() - 1};

        for (auto const& [component, track] : old->offsets) {
            if (mask.test(component)) {
                if (const std::size_t size = sizes[component]; size > 0) {
                    const std::size_t offset = goal->offsets[component];
                    std::memcpy(goal->columns[offset].data() + dest_row * size,
                                old->columns[track].data() + row * size,
                                size);
                }
            }
        }

        if (const std::uint32_t moved = old->dispose(row); moved != id) {
            slots[moved].row = row;
        }
        slots[id] = {dest_archetype, dest_row};
    }

}