#include <ranges>

#if defined(_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
    #include <windows.h>
#elif defined(__ANDROID__)
    #define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
        #define VK_USE_PLATFORM_IOS_MVK
    #else
        #define VK_USE_PLATFORM_METAL_EXT
        #define VK_USE_PLATFORM_MACOS_MVK
    #endif

#else
    #define VK_USE_PLATFORM_WAYLAND_KHR
    #define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstring>

#include "graphics/library/drivers/vulkan.hpp"
#include "graphics/library/assets/shader.hpp"

namespace core::graphics::library::drivers {

    struct Context {
        VkInstance instance{VK_NULL_HANDLE};
        VkPhysicalDevice adapter{VK_NULL_HANDLE};
        VkDevice device{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};
        VkSurfaceKHR surface{VK_NULL_HANDLE};
        VkSwapchainKHR chain{VK_NULL_HANDLE};
        VkFormat format{VK_FORMAT_B8G8R8A8_UNORM};
        VkExtent2D extent{};

        static constexpr std::uint32_t count = 2;
        VkImage images[count]{};
        VkImageView views[count]{};
        VkFramebuffer targets[count]{};

        VkRenderPass pass{VK_NULL_HANDLE};
        VkCommandPool pool{VK_NULL_HANDLE};
        VkCommandBuffer list{VK_NULL_HANDLE};

        VkSemaphore available{VK_NULL_HANDLE};
        VkSemaphore finished{VK_NULL_HANDLE};
        VkFence fence{VK_NULL_HANDLE};
        std::uint32_t index{0};

        VkPipelineLayout layout{VK_NULL_HANDLE};
        std::unordered_map<std::uint32_t, VkPipeline> pipelines;

        struct Mesh {
            VkBuffer buffer{VK_NULL_HANDLE};
            VkDeviceMemory memory{VK_NULL_HANDLE};
            std::size_t bytes{};
        };
        std::unordered_map<std::uint32_t, Mesh> geometry;

        struct Texture {
            VkImage image{VK_NULL_HANDLE};
            VkDeviceMemory memory{VK_NULL_HANDLE};
            VkImageView view{VK_NULL_HANDLE};
            VkFramebuffer framebuffer{VK_NULL_HANDLE};
        };
        std::unordered_map<std::uint32_t, Texture> textures;
        std::unordered_map<std::uint32_t, Texture> surfaces;
    };

    Vulkan::Vulkan(const void* target) noexcept : context(new Context()) {
        VkApplicationInfo app = {};
        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app.pApplicationName = "nullptr";
        app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app.pEngineName = "nullptr";
        app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app.apiVersion = VK_API_VERSION_1_0;

        std::vector<const char*> layers;
        #if defined(_DEBUG)
        layers.push_back("VK_LAYER_KHRONOS_validation");
        #endif

        std::vector<const char*> extensions = { VK_KHR_SURFACE_EXTENSION_NAME };
        #if defined(_WIN32)
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        #elif defined(__APPLE__)
        #include <TargetConditionals.h>
        extensions.push_back("VK_EXT_metal_surface");
        // Required portability extensions for modern MoltenVK on macOS/iOS
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        #elif defined(__ANDROID__)
        extensions.push_back("VK_KHR_android_surface");
        #else
        extensions.push_back("VK_KHR_xcb_surface");
        #endif

        VkInstanceCreateInfo setup = {};
        setup.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        setup.pApplicationInfo = &app;
        setup.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
        setup.ppEnabledLayerNames = layers.data();
        setup.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
        setup.ppEnabledExtensionNames = extensions.data();

        #if defined(__APPLE__)
        setup.flags |= VK_INSTANCE_CREATE_ENUMERATION_PORTABILITY_BIT_KHR;
        #endif

        if (vkCreateInstance(&setup, nullptr, &context->instance) != VK_SUCCESS) return;

        std::uint32_t count = 0;
        vkEnumeratePhysicalDevices(context->instance, &count, nullptr);
        if (count == 0) return;
        std::vector<VkPhysicalDevice> adapters(count);
        vkEnumeratePhysicalDevices(context->instance, &count, adapters.data());
        context->adapter = adapters[0];

        float priority = 1.0f;
        VkDeviceQueueCreateInfo queue = {};
        queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue.queueFamilyIndex = 0;
        queue.queueCount = 1;
        queue.pQueuePriorities = &priority;

        std::vector<const char*> devices = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        #if defined(__APPLE__)
        devices.push_back("VK_KHR_portability_subset");
        #endif

        VkDeviceCreateInfo device = {};
        device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device.queueCreateInfoCount = 1;
        device.pQueueCreateInfos = &queue;
        device.enabledExtensionCount = static_cast<std::uint32_t>(devices.size());
        device.ppEnabledExtensionNames = devices.data();

        if (vkCreateDevice(context->adapter, &device, nullptr, &context->device) != VK_SUCCESS) return;
        vkGetDeviceQueue(context->device, 0, 0, &context->queue);

        if (target) {
            #if defined(_WIN32)
            VkWin32SurfaceCreateInfoKHR surface = {};
            surface.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            surface.hwnd = static_cast<HWND>(const_cast<void*>(target));
            surface.hinstance = GetModuleHandle(nullptr);
            vkCreateWin32SurfaceKHR(context->instance, &surface, nullptr, &context->surface);
            #elif defined(__APPLE__)
            // Missing window surface creation block for macOS/iOS target handles
            #if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
            VkIOSSurfaceCreateInfoMVK surface = {};
            surface.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
            surface.pView = target;
            vkCreateIOSSurfaceMVK(context->instance, &surface, nullptr, &context->surface);
            #else
            VkMacOSSurfaceCreateInfoMVK surface = {};
            surface.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
            surface.pView = target;
            vkCreateMacOSSurfaceMVK(context->instance, &surface, nullptr, &context->surface);
            #endif
            #elif defined(__ANDROID__)
            VkAndroidSurfaceCreateInfoKHR surface = {};
            surface.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
            surface.window = (ANativeWindow*)target;
            vkCreateAndroidSurfaceKHR(context->instance, &surface, nullptr, &context->surface);
            #endif

            VkSwapchainCreateInfoKHR chain = {};
            chain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            chain.surface = context->surface;
            chain.minImageCount = Context::count;
            chain.imageFormat = context->format;
            chain.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            chain.imageExtent = { 800, 600 };
            chain.imageArrayLayers = 1;
            chain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            chain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            chain.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            chain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            chain.presentMode = VK_PRESENT_MODE_FIFO_KHR;
            chain.clipped = VK_TRUE;

            if (vkCreateSwapchainKHR(context->device, &chain, nullptr, &context->chain) == VK_SUCCESS) {
                std::uint32_t images_count = 0;
                vkGetSwapchainImagesKHR(context->device, context->chain, &images_count, nullptr);
                vkGetSwapchainImagesKHR(context->device, context->chain, &images_count, context->images);
                context->extent = chain.imageExtent;
            }
        }

        VkAttachmentDescription attachment = {};
        attachment.format = context->format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference reference = {};
        reference.attachment = 0;
        reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &reference;

        VkRenderPassCreateInfo pass = {};
        pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        pass.attachmentCount = 1;
        pass.pAttachments = &attachment;
        pass.subpassCount = 1;
        pass.pSubpasses = &subpass;

        vkCreateRenderPass(context->device, &pass, nullptr, &context->pass);

        if (target) {
            for (std::uint32_t i = 0; i < Context::count; i++) {
                VkImageViewCreateInfo view = {};
                view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                view.image = context->images[i];
                view.viewType = VK_IMAGE_VIEW_TYPE_2D;
                view.format = context->format;
                view.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
                view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

                if (vkCreateImageView(context->device, &view, nullptr, &context->views[i]) == VK_SUCCESS) {
                    VkFramebufferCreateInfo framebuffer = {};
                    framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    framebuffer.renderPass = context->pass;
                    framebuffer.attachmentCount = 1;
                    framebuffer.pAttachments = &context->views[i];
                    framebuffer.width = context->extent.width;
                    framebuffer.height = context->extent.height;
                    framebuffer.layers = 1;

                    vkCreateFramebuffer(context->device, &framebuffer, nullptr, &context->targets[i]);
                }
            }
        }

        VkCommandPoolCreateInfo pool = {};
        pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool.queueFamilyIndex = 0;
        pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(context->device, &pool, nullptr, &context->pool) == VK_SUCCESS) {
            VkCommandBufferAllocateInfo buffer = {};
            buffer.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            buffer.commandPool = context->pool;
            buffer.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            buffer.commandBufferCount = 1;

            vkAllocateCommandBuffers(context->device, &buffer, &context->list);
        }

        VkSemaphoreCreateInfo semaphore = {};
        semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence = {};
        fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        vkCreateSemaphore(context->device, &semaphore, nullptr, &context->available);
        vkCreateSemaphore(context->device, &semaphore, nullptr, &context->finished);
        vkCreateFence(context->device, &fence, nullptr, &context->fence);

        VkPipelineLayoutCreateInfo layout = {};
        layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vkCreatePipelineLayout(context->device, &layout, nullptr, &context->layout);
    }

    Vulkan::~Vulkan() {
        this->teardown();
    }

    void Vulkan::begin(const std::uint32_t width, const std::uint32_t height, const State& state) const noexcept {
        vkWaitForFences(context->device, 1, &context->fence, VK_TRUE, UINT64_MAX);
        vkResetFences(context->device, 1, &context->fence);

        if (state.active == 0 && context->chain) {
            vkAcquireNextImageKHR(context->device, context->chain, UINT64_MAX, context->available, VK_NULL_HANDLE, &context->index);
        }

        vkResetCommandBuffer(context->list, 0);

        VkCommandBufferBeginInfo begin = {};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(context->list, &begin);

        VkRenderPassBeginInfo pass = {};
        pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        pass.renderPass = context->pass;

        if (state.active > 0) {
            if (const auto lookup = context->surfaces.find(state.active); lookup != context->surfaces.end()) {
                pass.framebuffer = lookup->second.framebuffer;
                pass.renderArea.extent = { width, height };
            }
        } else if (context->chain) {
            pass.framebuffer = context->targets[context->index];
            pass.renderArea.extent = context->extent;
        }

        constexpr VkClearValue clear = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        pass.clearValueCount = 1;
        pass.pClearValues = &clear;

        vkCmdBeginRenderPass(context->list, &pass, VK_SUBPASS_CONTENTS_INLINE);

        if (const auto lookup = context->pipelines.find(state.pipeline); lookup != context->pipelines.end()) {
            vkCmdBindPipeline(context->list, VK_PIPELINE_BIND_POINT_GRAPHICS, lookup->second);
        }
    }

    void Vulkan::submit(const Buffer& stream) const noexcept {
        if (!stream.commands || stream.count == 0) return;
        const auto* array = static_cast<const Command*>(stream.commands);

        for (std::size_t step = 0; step < stream.count; ++step) {
            if (const auto& item = array[step]; item.type == Type::Mesh) {
                const std::uint32_t identity = item.gap[0];
                if (const auto lookup = context->geometry.find(identity); lookup != context->geometry.end()) {
                    VkDeviceSize offset = 0;
                    vkCmdBindVertexBuffers(context->list, 0, 1, &lookup->second.buffer, &offset);
                    vkCmdDraw(context->list, static_cast<std::uint32_t>(lookup->second.bytes / (sizeof(float) * 6)), 1, 0, 0);
                }
            }
        }
    }

    void Vulkan::end() const noexcept {
        vkCmdEndRenderPass(context->list);
        vkEndCommandBuffer(context->list);

        VkSubmitInfo submission = {};
        submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        constexpr VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        if (context->chain) {
            submission.waitSemaphoreCount = 1;
            submission.pWaitSemaphores = &context->available;
            submission.pWaitDstStageMask = &flags;
            submission.signalSemaphoreCount = 1;
            submission.pSignalSemaphores = &context->finished;
        }

        submission.commandBufferCount = 1;
        submission.pCommandBuffers = &context->list;

        vkQueueSubmit(context->queue, 1, &submission, context->fence);

        if (context->chain) {
            VkPresentInfoKHR present = {};
            present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present.waitSemaphoreCount = 1;
            present.pWaitSemaphores = &context->finished;
            present.swapchainCount = 1;
            present.pSwapchains = &context->chain;
            present.pImageIndices = &context->index;

            vkQueuePresentKHR(context->queue, &present);
        }
    }

    void Vulkan::dispose() noexcept {
        this->teardown();
    }

    void Vulkan::shader(std::uint32_t identity, const assets::Shader& vertex, const assets::Shader& pixel) const noexcept {
        const auto* source = vertex.get(identity);
        if (!source || !context->pass) return;

        VkShaderModule vertex_module = VK_NULL_HANDLE;
        VkShaderModule pixel_module = VK_NULL_HANDLE;

        VkShaderModuleCreateInfo vs = {};
        vs.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vs.codeSize = source->vertex.size;
        vs.pCode = static_cast<const std::uint32_t*>(source->vertex.code);
        vkCreateShaderModule(context->device, &vs, nullptr, &vertex_module);

        VkShaderModuleCreateInfo ps = {};
        ps.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ps.codeSize = source->pixel.size;
        ps.pCode = static_cast<const std::uint32_t*>(source->pixel.code);
        vkCreateShaderModule(context->device, &ps, nullptr, &pixel_module);

        VkPipelineShaderStageCreateInfo stages[2] = {};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertex_module;
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = pixel_module;
        stages[1].pName = "main";

        VkVertexInputBindingDescription binding = { 0, sizeof(float) * 6, VK_VERTEX_INPUT_RATE_VERTEX };
        VkVertexInputAttributeDescription attributes[2] = {
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
            { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3 }
        };

        VkPipelineVertexInputStateCreateInfo vertex_input = {};
        vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input.vertexBindingDescriptionCount = 1;
        vertex_input.pVertexBindingDescriptions = &binding;
        vertex_input.vertexAttributeDescriptionCount = 2;
        vertex_input.pVertexAttributeDescriptions = attributes;

        VkPipelineInputAssemblyStateCreateInfo assembly = {};
        assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport = { 0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f };
        VkRect2D scissor = { {0, 0}, {800, 600} };
        VkPipelineViewportStateCreateInfo viewport_state = {};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.pViewports = &viewport;
        viewport_state.scissorCount = 1;
        viewport_state.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample = {};
        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState blend = {};
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blend_state = {};
        blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blend_state.attachmentCount = 1;
        blend_state.pAttachments = &blend;

        VkGraphicsPipelineCreateInfo blueprint = {};
        blueprint.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        blueprint.stageCount = 2;
        blueprint.pStages = stages;
        blueprint.pVertexInputState = &vertex_input;
        blueprint.pInputAssemblyState = &assembly;
        blueprint.pViewportState = &viewport_state;
        blueprint.pRasterizationState = &rasterizer;
        blueprint.pMultisampleState = &multisample;
        blueprint.pColorBlendState = &blend_state;
        blueprint.layout = context->layout;
        blueprint.renderPass = context->pass;

        VkPipeline pipeline = VK_NULL_HANDLE;
        if (vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1, &blueprint, nullptr, &pipeline) == VK_SUCCESS) {
            context->pipelines[identity] = pipeline;
        }

        vkDestroyShaderModule(context->device, pixel_module, nullptr);
        vkDestroyShaderModule(context->device, vertex_module, nullptr);
    }

    void Vulkan::mesh(const std::uint32_t identity, const float* vertices, const std::size_t size) const noexcept {
        if (!vertices || size == 0) return;

        Context::Mesh record = {};
        record.bytes = size;

        VkBufferCreateInfo buffer = {};
        buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer.size = size;
        buffer.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(context->device, &buffer, nullptr, &record.buffer) == VK_SUCCESS) {
            VkMemoryRequirements requirements;
            vkGetBufferMemoryRequirements(context->device, record.buffer, &requirements);

            VkMemoryAllocateInfo allocation = {};
            allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocation.allocationSize = requirements.size;

            VkPhysicalDeviceMemoryProperties properties;
            vkGetPhysicalDeviceMemoryProperties(context->adapter, &properties);
            for (std::uint32_t i = 0; i < properties.memoryTypeCount; i++) {
                if (requirements.memoryTypeBits & 1 << i && properties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                    allocation.memoryTypeIndex = i;
                    break;
                }
            }

            if (vkAllocateMemory(context->device, &allocation, nullptr, &record.memory) == VK_SUCCESS) {
                vkBindBufferMemory(context->device, record.buffer, record.memory, 0);

                void* data = nullptr;
                vkMapMemory(context->device, record.memory, 0, size, 0, &data);
                std::memcpy(data, vertices, size);
                vkUnmapMemory(context->device, record.memory);

                context->geometry[identity] = record;
            }
        }
    }

    void Vulkan::update(const std::uint32_t identity, const float* vertices, const std::size_t size) const noexcept {
        if (const auto lookup = context->geometry.find(identity); lookup != context->geometry.end()) {
            void* data = nullptr;
            vkMapMemory(context->device, lookup->second.memory, 0, std::min(size, lookup->second.bytes), 0, &data);
            std::memcpy(data, vertices, std::min(size, lookup->second.bytes));
            vkUnmapMemory(context->device, lookup->second.memory);
        }
    }

    void Vulkan::free(const std::uint32_t identity) const noexcept {
        if (const auto lookup = context->geometry.find(identity); lookup != context->geometry.end()) {
            vkDestroyBuffer(context->device, lookup->second.buffer, nullptr);
            vkFreeMemory(context->device, lookup->second.memory, nullptr);
            context->geometry.erase(lookup);
        }
    }

    void Vulkan::texture(const std::uint32_t identity, const std::uint8_t* pixels, const std::uint32_t width, const std::uint32_t height) const noexcept {
        if (!pixels || width == 0 || height == 0) return;

        const VkDeviceSize bytes = static_cast<VkDeviceSize>(width) * height * 4;

        VkBuffer staging_buffer = VK_NULL_HANDLE;
        VkDeviceMemory staging_memory = VK_NULL_HANDLE;

        VkBufferCreateInfo staging = {};
        staging.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        staging.size = bytes;
        staging.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staging.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(context->device, &staging, nullptr, &staging_buffer) != VK_SUCCESS) return;

        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(context->device, staging_buffer, &requirements);

        VkMemoryAllocateInfo staging_allocation = {};
        staging_allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        staging_allocation.allocationSize = requirements.size;

        VkPhysicalDeviceMemoryProperties properties;
        vkGetPhysicalDeviceMemoryProperties(context->adapter, &properties);
        
        for (std::uint32_t i = 0; i < properties.memoryTypeCount; i++) {
            if (requirements.memoryTypeBits & 1 << i && 
                properties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                staging_allocation.memoryTypeIndex = i;
                break;
            }
        }

        if (vkAllocateMemory(context->device, &staging_allocation, nullptr, &staging_memory) != VK_SUCCESS) {
            vkDestroyBuffer(context->device, staging_buffer, nullptr);
            return;
        }
        vkBindBufferMemory(context->device, staging_buffer, staging_memory, 0);

        void* data = nullptr;
        vkMapMemory(context->device, staging_memory, 0, bytes, 0, &data);
        std::memcpy(data, pixels, bytes);
        vkUnmapMemory(context->device, staging_memory);

        Context::Texture record = {};
        
        VkImageCreateInfo image = {};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = VK_FORMAT_R8G8B8A8_UNORM;
        image.extent = { width, height, 1 };
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(context->device, &image, nullptr, &record.image) != VK_SUCCESS) {
            vkDestroyBuffer(context->device, staging_buffer, nullptr);
            vkFreeMemory(context->device, staging_memory, nullptr);
            return;
        }

        vkGetImageMemoryRequirements(context->device, record.image, &requirements);

        VkMemoryAllocateInfo image_allocation = {};
        image_allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        image_allocation.allocationSize = requirements.size;

        for (std::uint32_t i = 0; i < properties.memoryTypeCount; i++) {
            if (requirements.memoryTypeBits & 1 << i &&
                properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                image_allocation.memoryTypeIndex = i;
                break;
            }
        }

        if (vkAllocateMemory(context->device, &image_allocation, nullptr, &record.memory) != VK_SUCCESS) {
            vkDestroyImage(context->device, record.image, nullptr);
            vkDestroyBuffer(context->device, staging_buffer, nullptr);
            vkFreeMemory(context->device, staging_memory, nullptr);
            return;
        }
        vkBindImageMemory(context->device, record.image, record.memory, 0);

        VkCommandBufferAllocateInfo buffer_allocation = {};
        buffer_allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        buffer_allocation.commandPool = context->pool;
        buffer_allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        buffer_allocation.commandBufferCount = 1;

        VkCommandBuffer list = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(context->device, &buffer_allocation, &list);

        VkCommandBufferBeginInfo begin = {};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(list, &begin);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = record.image;
        barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(list, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region = {};
        region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(list, staging_buffer, record.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(list, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        vkEndCommandBuffer(list);

        VkSubmitInfo submission = {};
        submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submission.commandBufferCount = 1;
        submission.pCommandBuffers = &list;

        vkQueueSubmit(context->queue, 1, &submission, VK_NULL_HANDLE);
        vkQueueWaitIdle(context->queue);

        vkFreeCommandBuffers(context->device, context->pool, 1, &list);
        vkDestroyBuffer(context->device, staging_buffer, nullptr);
        vkFreeMemory(context->device, staging_memory, nullptr);

        VkImageViewCreateInfo view = {};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image = record.image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = VK_FORMAT_R8G8B8A8_UNORM;
        view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        if (vkCreateImageView(context->device, &view, nullptr, &record.view) == VK_SUCCESS) {
            context->textures[identity] = record;
        }
    }

    void Vulkan::unload(const std::uint32_t identity) const noexcept {
        if (const auto lookup = context->textures.find(identity); lookup != context->textures.end()) {
            vkDestroyImageView(context->device, lookup->second.view, nullptr);
            vkDestroyImage(context->device, lookup->second.image, nullptr);
            vkFreeMemory(context->device, lookup->second.memory, nullptr);
            context->textures.erase(lookup);
        }
    }

    void Vulkan::surface(const std::uint32_t identity, const std::uint32_t width, const std::uint32_t height) const noexcept {
        Context::Texture record = {};

        VkImageCreateInfo image = {};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = context->format;
        image.extent = { width, height, 1 };
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(context->device, &image, nullptr, &record.image) == VK_SUCCESS) {
            VkMemoryRequirements requirements;
            vkGetImageMemoryRequirements(context->device, record.image, &requirements);

            VkMemoryAllocateInfo allocation = {};
            allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocation.allocationSize = requirements.size;

            VkPhysicalDeviceMemoryProperties properties;
            vkGetPhysicalDeviceMemoryProperties(context->adapter, &properties);
            for (std::uint32_t i = 0; i < properties.memoryTypeCount; i++) {
                if (requirements.memoryTypeBits & 1 << i && properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                    allocation.memoryTypeIndex = i;
                    break;
                }
            }

            if (vkAllocateMemory(context->device, &allocation, nullptr, &record.memory) == VK_SUCCESS) {
                vkBindImageMemory(context->device, record.image, record.memory, 0);

                VkImageViewCreateInfo view = {};
                view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                view.image = record.image;
                view.viewType = VK_IMAGE_VIEW_TYPE_2D;
                view.format = context->format;
                view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

                if (vkCreateImageView(context->device, &view, nullptr, &record.view) == VK_SUCCESS) {
                    VkFramebufferCreateInfo framebuffer = {};
                    framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    framebuffer.renderPass = context->pass;
                    framebuffer.attachmentCount = 1;
                    framebuffer.pAttachments = &record.view;
                    framebuffer.width = width;
                    framebuffer.height = height;
                    framebuffer.layers = 1;

                    if (vkCreateFramebuffer(context->device, &framebuffer, nullptr, &record.framebuffer) == VK_SUCCESS) {
                        context->surfaces[identity] = record;
                    }
                }
            }
        }
    }

    void Vulkan::release(const std::uint32_t identity) const noexcept {
        if (const auto lookup = context->surfaces.find(identity); lookup != context->surfaces.end()) {
            vkDestroyFramebuffer(context->device, lookup->second.framebuffer, nullptr);
            vkDestroyImageView(context->device, lookup->second.view, nullptr);
            vkDestroyImage(context->device, lookup->second.image, nullptr);
            vkFreeMemory(context->device, lookup->second.memory, nullptr);
            context->surfaces.erase(lookup);
        }
    }

    void Vulkan::teardown() noexcept {
        if (context) {
            if (context->device) {
                vkDeviceWaitIdle(context->device);

                for (const auto &pipeline: context->pipelines | std::views::values) {
                    vkDestroyPipeline(context->device, pipeline, nullptr);
                }
                vkDestroyPipelineLayout(context->device, context->layout, nullptr);

                for (const auto &geometry: context->geometry | std::views::values) {
                    vkDestroyBuffer(context->device, geometry.buffer, nullptr);
                    vkFreeMemory(context->device, geometry.memory, nullptr);
                }

                for (auto &[image, memory, view, framebuffer]: context->textures | std::views::values) {
                    vkDestroyImageView(context->device, view, nullptr);
                    vkDestroyImage(context->device, image, nullptr);
                    vkFreeMemory(context->device, memory, nullptr);
                }

                for (auto &[image, memory, view, framebuffer]: context->surfaces | std::views::values) {
                    vkDestroyFramebuffer(context->device, framebuffer, nullptr);
                    vkDestroyImageView(context->device, view, nullptr);
                    vkDestroyImage(context->device, image, nullptr);
                    vkFreeMemory(context->device, memory, nullptr);
                }

                vkDestroySemaphore(context->device, context->finished, nullptr);
                vkDestroySemaphore(context->device, context->available, nullptr);
                vkDestroyFence(context->device, context->fence, nullptr);

                vkDestroyCommandPool(context->device, context->pool, nullptr);

                if (context->chain) {
                    for (std::uint32_t i = 0; i < Context::count; i++) {
                        vkDestroyFramebuffer(context->device, context->targets[i], nullptr);
                        vkDestroyImageView(context->device, context->views[i], nullptr);
                    }
                    vkDestroySwapchainKHR(context->device, context->chain, nullptr);
                }

                vkDestroyRenderPass(context->device, context->pass, nullptr);
                vkDestroyDevice(context->device, nullptr);
            }

            if (context->instance) {
                if (context->surface) {
                    vkDestroySurfaceKHR(context->instance, context->surface, nullptr);
                }
                vkDestroyInstance(context->instance, nullptr);
            }

            delete context;
            context = nullptr;
        }
    }

}