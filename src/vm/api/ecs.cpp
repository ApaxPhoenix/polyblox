#include <sol/sol.hpp>
#include <lua.hpp>
#include <unordered_map>
#include <functional>
#include <utility>
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <ranges>

#include "vm/api/ecs.hpp"
#include "core/ecs/world.hpp"
#include "core/ecs/query.hpp"
#include "core/primitives/vector2.hpp"
#include "core/primitives/vector3.hpp"
#include "core/primitives/vector4.hpp"
#include "core/primitives/quaternion.hpp"
#include "core/primitives/matrix3.hpp"
#include "core/primitives/matrix4.hpp"
#include "core/primitives/transform.hpp"

using namespace core::ecs;
using namespace core::ecs::primitives;

namespace {
    using Reader = sol::object(*)(const void* pointer, lua_State* state);
    using Writer = void(*)(void* pointer, const sol::object& value);

    struct Metadata {
        std::uint32_t id = 0;
        std::size_t size = 0;
        bool object = false;
        Reader reader = nullptr;
        Writer writer = nullptr;
    };

    class Registry {
    public:
        struct Evaluator {
            using Match = std::function<bool(const sol::object&)>;
            Match match;
            Metadata meta;
        };

        template <typename Type>
        static void add() {
            Evaluator item;
            item.match = [](const sol::object& value) {
                return value.is<Type>();
            };
            item.meta.size = sizeof(Type);
            item.meta.object = false;
            item.meta.reader = [](const void* pointer, lua_State* state) {
                return sol::make_object(state, *static_cast<const Type*>(pointer));
            };
            item.meta.writer = [](void* pointer, const sol::object& value) {
                *static_cast<Type*>(pointer) = value.as<Type>();
            };
            list().push_back(std::move(item));
        }

        static std::vector<Evaluator>& list() {
            static std::vector<Evaluator> data;
            return data;
        }

        static bool find(const sol::object& value, Metadata& meta) {
            for (const auto& item : list()) {
                if (item.match(value)) {
                    meta = item.meta;
                    return true;
                }
            }
            return false;
        }
    };

    struct Entity {
        World* world = nullptr;
        Id identity = Null;

        [[nodiscard]] Id id() const { return identity; }
    };

    struct Column {
        void* block = nullptr;
        std::size_t size = 0;
        Reader reader = nullptr;
        Writer writer = nullptr;
        const Id* entities = nullptr;
        World* world = nullptr;

        [[nodiscard]] sol::object get(const sol::this_state state, const std::size_t index) const {
            const void* pointer = static_cast<const char*>(block) + (index - 1) * size;
            return reader(pointer, state);
        }

        void set(const std::size_t index, const sol::object& value) const {
            void* pointer = static_cast<char*>(block) + (index - 1) * size;
            writer(pointer, value);
        }
    };

    struct Search {
        Query base;

        explicit Search(Query query) : base(std::move(query)) {}

        Search& with(const std::uint32_t type) {
            base.with(type);
            return *this;
        }

        Search& without(const std::uint32_t type) {
            base.without(type);
            return *this;
        }

        Search& any(const sol::table& table) {
            std::vector<std::uint32_t> types;
            for (const auto& value : table | std::views::values) {
                types.push_back(value.as<std::uint32_t>());
            }
            base.any(types);
            return *this;
        }

        [[nodiscard]] bool has(const std::uint32_t type) const {
            return base.has(type);
        }
    };

    void free(lua_State* state, const World* world, Id identity, std::uint32_t type, const Metadata& meta) {
        if (meta.object) {
            if (void* pointer = const_cast<World*>(world)->get(identity, type)) {
                int* cell = static_cast<int*>(pointer);
                if (*cell > 0) {
                    luaL_unref(state, LUA_REGISTRYINDEX, *cell);
                }
            }
        }
    }
}

void ecs(lua_State* state) {
    sol::state_view lua(state);
    sol::table hidden = lua.create_table();

    Registry::add<Vector2>();
    Registry::add<Vector3>();
    Registry::add<Vector4>();
    Registry::add<Quaternion>();
    Registry::add<Matrix3>();
    Registry::add<Matrix4>();
    Registry::add<Transform>();

    auto store = std::make_shared<std::unordered_map<std::uint32_t, Metadata>>();

    hidden.new_usertype<Column>("column",
        sol::no_constructor,
        sol::meta_function::index, [](const sol::this_state state, const Column& self, const sol::object& key) -> sol::object {
            if (key.is<std::size_t>()) {
                return self.get(state, key.as<std::size_t>());
            }
            if (key.is<std::string>()) {
                const std::string name = key.as<std::string>();
                if (name == "get") return sol::make_object(state, &Column::get);
                if (name == "set") return sol::make_object(state, &Column::set);
                if (name == "index") {
                    return sol::make_object(state, [world = self.world, entities = self.entities](std::size_t index) {
                        return Entity{ world, entities[index - 1] };
                    });
                }
            }
            return sol::lua_nil;
        },
        sol::meta_function::new_index, [](const Column& self, std::size_t index, const sol::object& value) {
            self.set(index, value);
        }
    );

    hidden.new_usertype<Search>("query",
        sol::no_constructor,
        "with", &Search::with,
        "without", &Search::without,
        "any", &Search::any,
        "has", &Search::has
    );

    hidden.new_usertype<Entity>("Entity",
        sol::no_constructor,
        "id", &Entity::id,
        "set", [store](const sol::this_state state, Entity& self, std::uint32_t type, const sol::object& value) -> Entity& {
            if (!self.world) return self;
            if (auto match = store->find(type); match != store->end()) {
                const auto& meta = match->second;
                if (meta.size == 0) {
                    self.world->set(self.identity, type, nullptr);
                } else {
                    free(state, self.world, self.identity, type, meta);
                    std::vector<char> memory(meta.size, 0);
                    meta.writer(memory.data(), value);
                    self.world->set(self.identity, type, memory.data());
                }
            }
            return self;
        },
        "tag", [](Entity& self, std::uint32_t type) -> Entity& {
            if (self.world) {
                self.world->set(self.identity, type, nullptr);
            }
            return self;
        },
        "attach", [](Entity& self, const Entity& parent) -> Entity& {
            if (self.world) {
                self.world->attach(self.identity, parent.identity);
            }
            return self;
        },
        "parent", [](const sol::this_state state, const Entity& self) -> sol::object {
            if (!self.world) return sol::lua_nil;
            Id parent = self.world->parent(self.identity);
            if (parent == Null) return sol::lua_nil;
            return sol::make_object(state, Entity{ self.world, parent });
        }
    );

    sol::usertype<World> syntax = lua.new_usertype<World>("World",
        sol::no_constructor,
        "component", [store](const sol::this_state state, World& instance, const std::string& name, const sol::optional<sol::object>& value) {
            (void)state;
            Metadata meta;
            if (value && value->valid() && !value->is<sol::lua_nil_t>()) {
                const auto& input = *value;
                if (!Registry::find(input, meta)) {
                    meta.size = sizeof(int);
                    meta.object = true;
                    meta.reader = [](const void* pointer, lua_State* state) -> sol::object {
                        lua_rawgeti(state, LUA_REGISTRYINDEX, *static_cast<const int*>(pointer));
                        sol::object object(state, -1);
                        lua_pop(state, 1);
                        return object;
                    };
                    meta.writer = [](void* pointer, const sol::object& value) {
                        int* cell = static_cast<int*>(pointer);
                        lua_State* state = value.lua_state();
                        if (*cell > 0) {
                            luaL_unref(state, LUA_REGISTRYINDEX, *cell);
                            *cell = 0;
                        }
                        if (value.valid() && !value.is<sol::lua_nil_t>()) {
                            sol::main_object registry(state, value);
                            *cell = registry.registry_index();
                            registry.abandon();
                        }
                    };
                }
            } else {
                meta.size = 0;
                meta.object = false;
                meta.reader = [](const void*, lua_State* state) -> sol::object { return {state, sol::lua_nil}; };
                meta.writer = [](void*, const sol::object&) {};
            }
            const std::uint32_t type = instance.component(name, meta.size);
            meta.id = type;
            (*store)[type] = meta;
            return type;
        },
        "spawn", [](World& instance) {
            return Entity{ &instance, instance.spawn() };
        },
        "clone", [](World& instance, const Entity& entity) {
            return Entity{ &instance, instance.clone(entity.identity) };
        },
        "dispose", [store](const sol::this_state state, World& instance, const sol::object& entity) {
            Id identity = entity.is<Entity>() ? entity.as<Entity>().identity : entity.as<Id>();
            for (const auto& meta : *store | std::views::values) {
                free(state, &instance, identity, meta.id, meta);
            }
            instance.dispose(identity);
        },
        "clear", [](World& instance) {
            instance.clear();
        },
        "has", [](const World& instance, const sol::object& entity, const std::uint32_t type) {
            const Id identity = entity.is<Entity>() ? entity.as<Entity>().identity : entity.as<Id>();
            return instance.has(identity, type);
        },
        "remove", [store](const sol::this_state state, World& instance, const sol::object& entity, const std::uint32_t type) {
            const Id identity = entity.is<Entity>() ? entity.as<Entity>().identity : entity.as<Id>();
            if (const auto match = store->find(type); match != store->end()) {
                free(state, &instance, identity, type, match->second);
            }
            instance.remove(identity, type);
        },
        "query", [](const World& instance) { return Search{ instance.query() }; },
        "batch", [](World& instance, sol::function action) {
            instance.batch([action = std::move(action)]() { static_cast<void>(action()); });
        },
        "set", [store](const sol::this_state state, World& instance, const sol::object& entity, std::uint32_t type, const sol::object& value) {
            const Id identity = entity.is<Entity>() ? entity.as<Entity>().identity : entity.as<Id>();
            const auto match = store->find(type);
            if (match == store->end()) return;
            const auto& meta = match->second;
            if (meta.size == 0) {
                instance.set(identity, type, nullptr);
                return;
            }
            free(state, &instance, identity, type, meta);
            std::vector<char> memory(meta.size, 0);
            meta.writer(memory.data(), value);
            instance.set(identity, type, memory.data());
        },
        "get", [store](const sol::this_state state, const World& instance, const sol::object& entity, std::uint32_t type) -> sol::object {
            Id identity = entity.is<Entity>() ? entity.as<Entity>().identity : entity.as<Id>();
            void* pointer = instance.get(identity, type);
            if (!pointer) return sol::lua_nil;
            auto match = store->find(type);
            if (match == store->end()) return sol::lua_nil;
            return match->second.reader(pointer, state);
        },
        "global", [store](const sol::this_state state, World& instance, std::uint32_t type) -> sol::object {
            void* pointer = instance.global(type);
            if (!pointer) return sol::lua_nil;
            auto match = store->find(type);
            if (match == store->end()) return sol::lua_nil;
            return match->second.reader(pointer, state);
        },
        "wait", [](World& instance, std::uint32_t type, sol::function action) {
            instance.wait(type, [action = std::move(action)](Id identity) { static_cast<void>(action(identity)); });
        },
        "loop", [store](const sol::this_state state, const World& instance, const Search& search, sol::function action) {
            if (!action.valid()) return;
            World::loop(search.base, [state, action = std::move(action), store, &instance, order = search.base.results()](std::size_t count, const Id* entities, const std::vector<void*>& blocks) {
                std::vector<sol::object> args;
                args.reserve(blocks.size() + 1);
                args.push_back(sol::make_object(state, count));
                for (std::size_t index = 0; index < blocks.size(); ++index) {
                    if (index < order.size()) {
                        std::uint32_t type = order[index];
                        if (auto match = store->find(type); match != store->end()) {
                            const auto& meta = match->second;
                            auto* world = const_cast<World*>(&instance);
                            args.push_back(sol::make_object(state, Column{ blocks[index], meta.size, meta.reader, meta.writer, entities, world }));
                            continue;
                        }
                    }
                    args.emplace_back(sol::lua_nil);
                }
                static_cast<void>(action.call(sol::as_args(args)));
            });
        }
    );

    syntax["new"] = [] {
        return std::make_unique<World>();
    };
}