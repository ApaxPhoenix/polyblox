#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <vulkan/vulkan.h>
#include "archetype.hpp"

namespace core::ecs {

    class Query {
    public:
        using Callback = std::function<void(std::size_t size, const Id* entities, const std::vector<void*>& columns)>;

        explicit Query(const std::vector<std::unique_ptr<Archetype>>& registry) noexcept;

        Query& with(std::uint32_t component);
        Query& without(std::uint32_t component);
        Query& any(const std::vector<std::uint32_t>& components);
        Query& sort(std::uint32_t component);

        [[nodiscard]] bool has(std::uint32_t component) const noexcept;

        void process(const Callback& callback, const std::vector<std::uint32_t>& components) const;
        void dispatch(VkCommandBuffer command, VkPipelineLayout layout, VkPipeline pipeline, const std::vector<std::uint32_t>& components) const;

        [[nodiscard]] const std::vector<std::uint32_t>& results() const noexcept { return elements; }

    private:
        const std::vector<std::unique_ptr<Archetype>>& registry;
        Mask required;
        Mask excluded;
        Mask optional;
        std::vector<std::uint32_t> elements;
    };

}