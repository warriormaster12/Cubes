#include "render_device.h"
#include <VkBootstrap.h>
#include <cmath>
#include "window.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <spirv_reflect.h>



void Swapchain::build(VkPresentModeKHR p_present_mode /*= VK_PRESENT_MODE_FIFO_KHR*/) {
    clear();
    vkb::SwapchainBuilder swapchain_builder {physical_device, device, surface};
    vkb::Swapchain vkb_swapchain = swapchain_builder
                                    .use_default_format_selection()
                                    .set_desired_extent(window->width, window->height)
                                    .set_desired_present_mode(p_present_mode)
                                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                    .build()
                                    .value();
    swapchain = vkb_swapchain.swapchain;
    images = vkb_swapchain.get_images().value();
    views = vkb_swapchain.get_image_views().value();
    format = vkb_swapchain.image_format;
    extent = vkb_swapchain.extent;
    window->resize = false;
}

void Swapchain::clear() {

    if (views.size() > 0) {
        for (int i = 0; i < images.size(); ++i) {
            vkDestroyImageView(device, views[i], nullptr);
        }
    }

    if (swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    }
}

bool CuRenderDevice::init(CuWindow *p_window) {
    if (!p_window) {
        ENGINE_ERROR("GLFWwindow must be provided");
        return false;
    }
    window = p_window;
    VkResult volk_init = volkInitialize();
    if (volk_init != VK_SUCCESS) {
        ENGINE_ERROR("Failed to init volk");
        return false;
    }
    vkb::InstanceBuilder builder;
    vkb::Result<vkb::Instance> inst_ret = builder.set_app_name ("Vulkan app")
                        .request_validation_layers()
                        .require_api_version(1,3)
                        .use_default_debug_messenger()
                        .build();
    if (!inst_ret) {
        ENGINE_ERROR("Failed to create Vulkan instance. Error: {}", inst_ret.error().message());
        return false;
    }
    instance = inst_ret.value().instance;

    debug_messenger = inst_ret->debug_messenger;

    volkLoadInstance(instance);

    main_deletion_queue.push_function([&](){
        vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, VK_NULL_HANDLE);
        vkDestroyInstance(instance, VK_NULL_HANDLE);
    });

    glfwCreateWindowSurface(instance, window->raw_window, nullptr, &surface);
    main_deletion_queue.push_function([&](){
        vkDestroySurfaceKHR(instance, surface, VK_NULL_HANDLE);
    });
    VkPhysicalDeviceVulkan13Features feats_13 = {};
    feats_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    feats_13.dynamicRendering = true;
    feats_13.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features feats_12 = {};
    feats_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	feats_12.bufferDeviceAddress = true;
	feats_12.descriptorIndexing = true;
    vkb::PhysicalDeviceSelector selector{ inst_ret.value() };
    vkb::Result<vkb::PhysicalDevice> phys_ret = selector.set_surface(surface)
                        .set_minimum_version(1, 3) // require a vulkan 1.3 capable device
                        .set_required_features_13(feats_13)
                        .set_required_features_12(feats_12)
                        .prefer_gpu_device_type()
                        .select();
    if (!phys_ret) {
        ENGINE_ERROR("Failed to select Vulkan Physical Device. Error: {}", phys_ret.error().message());
        return false;
    }

    physical_device = phys_ret->physical_device;
    VkPhysicalDeviceProperties device_properties = {};
    vkGetPhysicalDeviceProperties(physical_device, &device_properties);
    ENGINE_INFO("Selected GPU: {}", device_properties.deviceName);
    vkb::DeviceBuilder device_builder{ phys_ret.value () };
    // automatically propagate needed data from instance & physical device
    vkb::Result<vkb::Device> dev_ret = device_builder.build ();
    if (!dev_ret) {
        ENGINE_ERROR("Failed to create Vulkan device. Error: {}", dev_ret.error().message());
        return false;
    }
    vkb::Device vkb_device = dev_ret.value ();

    // Get the VkDevice handle used in the rest of a vulkan application
    device = vkb_device.device;

    volkLoadDevice(device);

    main_deletion_queue.push_function([&](){vkDestroyDevice(device, VK_NULL_HANDLE);});

    // Get the graphics queue with a helper function
    vkb::Result<VkQueue> graphics_queue_ret = vkb_device.get_queue (vkb::QueueType::graphics);
    if (!graphics_queue_ret) {
        ENGINE_ERROR("Failed to get graphics queue. Error: {}", graphics_queue_ret.error().message()); 
        return false;
    }
    graphics_queue = graphics_queue_ret.value ();
    graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    VmaVulkanFunctions vulkan_functions = {};
    vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkan_functions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vulkan_functions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vulkan_functions.vkAllocateMemory = vkAllocateMemory;
    vulkan_functions.vkFreeMemory = vkFreeMemory;
    vulkan_functions.vkMapMemory = vkMapMemory;
    vulkan_functions.vkUnmapMemory = vkUnmapMemory;
    vulkan_functions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
    vulkan_functions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;
    vulkan_functions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vulkan_functions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    vulkan_functions.vkBindBufferMemory = vkBindBufferMemory;
    vulkan_functions.vkBindImageMemory = vkBindImageMemory;
    vulkan_functions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vulkan_functions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vulkan_functions.vkCreateBuffer = vkCreateBuffer;
    vulkan_functions.vkCreateImage = vkCreateImage;
    vulkan_functions.vkDestroyBuffer = vkDestroyBuffer;
    vulkan_functions.vkDestroyImage = vkDestroyImage;
    vulkan_functions.vkCmdCopyBuffer = vkCmdCopyBuffer;
    vulkan_functions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
    vulkan_functions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;
    vulkan_functions.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
    vulkan_functions.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
    vulkan_functions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
    vulkan_functions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
    vulkan_functions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;
    
    VmaAllocatorCreateInfo allocator_create_info = {};
    allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocator_create_info.physicalDevice = physical_device;
    allocator_create_info.device = device;
    allocator_create_info.instance = instance;
    allocator_create_info.pVulkanFunctions =  (const VmaVulkanFunctions*) &vulkan_functions;
    
    vmaCreateAllocator(&allocator_create_info, &allocator);

    main_deletion_queue.push_function([&](){
        vmaDestroyAllocator(allocator);
    });

    swapchain = Swapchain(surface, device, physical_device, window);
    swapchain.build();

    main_deletion_queue.push_function([&](){
        swapchain.clear();
    });

    {
        draw_texture.extent = {
            swapchain.extent.width,
            swapchain.extent.height,
            1
        };

        draw_texture.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        
        VkImageUsageFlags draw_image_usage_flags = {};
        draw_image_usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        draw_image_usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        draw_image_usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        draw_image_usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        VkImageCreateInfo draw_image_info = image_create_info(draw_texture.format, draw_image_usage_flags, draw_texture.extent);

        VmaAllocationCreateInfo rimg_allocinfo = {};
        rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        rimg_allocinfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        vmaCreateImage(allocator, &draw_image_info, &rimg_allocinfo, &draw_texture.image, &draw_texture.allocation, nullptr);

        VkImageViewCreateInfo rview_info = imageview_create_info(draw_texture.format, draw_texture.image, VK_IMAGE_ASPECT_COLOR_BIT);

        VK_CHECK(vkCreateImageView(device, &rview_info, nullptr, &draw_texture.view));

        main_deletion_queue.push_function([&](){
            vkDestroyImageView(device, draw_texture.view, nullptr);
		    vmaDestroyImage(allocator, draw_texture.image, draw_texture.allocation);
        });
    }

    VkCommandPoolCreateInfo command_pool_info = {};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.queueFamilyIndex = graphics_queue_family;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.pNext = nullptr;

    VkFenceCreateInfo render_fence_info = {};
    render_fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    render_fence_info.pNext = nullptr;
    render_fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo render_swapchain_semaphore_info = {};
    render_swapchain_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    render_swapchain_semaphore_info.pNext = nullptr;
    render_swapchain_semaphore_info.flags = 0;

    for(int i = 0; i < FRAME_OVERLAP; ++i){
        FrameData& current_frame = frame_data[i];
        VK_CHECK(vkCreateCommandPool(device, &command_pool_info, nullptr, &current_frame.cmp));

        VkCommandBufferAllocateInfo command_buffer_alloc_info = {};
        command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_alloc_info.commandPool = current_frame.cmp;
        command_buffer_alloc_info.pNext = nullptr;
        command_buffer_alloc_info.commandBufferCount = 1;
        command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_alloc_info, &current_frame.cmb));

        VK_CHECK(vkCreateSemaphore(device, &render_swapchain_semaphore_info, nullptr, &current_frame.swapchain_semaphore));
        VK_CHECK(vkCreateSemaphore(device, &render_swapchain_semaphore_info, nullptr, &current_frame.render_semaphore));

        VK_CHECK(vkCreateFence(device, &render_fence_info, nullptr, &current_frame.render_fence));

        current_frame.deletion_queue.push_function([&]() {
            vkDestroySemaphore(device, current_frame.swapchain_semaphore, nullptr);
            vkDestroySemaphore(device, current_frame.render_semaphore, nullptr);
            vkDestroyFence(device, current_frame.render_fence, nullptr);
            vkDestroyCommandPool(device, current_frame.cmp, nullptr);
        });
    }

    return true;
}

bool get_shader_stage(const CompiledShaderInfo p_shader_info, VkShaderStageFlagBits& p_out_shader_stage) {
    switch (p_shader_info.stage) {
        case VERTEX:
            p_out_shader_stage = VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case FRAGMENT:
            p_out_shader_stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
        default:
            ENGINE_ERROR("Not a supported shader stage");
            return false;
    }
    return true;
}

struct BindingHash {
    size_t operator()(const VkDescriptorSetLayoutBinding& binding) const {
        size_t hash = std::hash<uint32_t>{}(binding.binding) ^ 
                      std::hash<uint32_t>{}(binding.descriptorType) ^ 
                      std::hash<uint32_t>{}(binding.descriptorCount);
        return hash;
    }
};

// Equality function for VkDescriptorSetLayoutBinding
struct BindingEqual {
    bool operator()(const VkDescriptorSetLayoutBinding& lhs, const VkDescriptorSetLayoutBinding& rhs) const {
        return lhs.binding == rhs.binding &&
               lhs.descriptorType == rhs.descriptorType &&
               lhs.descriptorCount == rhs.descriptorCount;
    }
};


std::vector<VkDescriptorSetLayout> generate_descriptor_layouts(VkDevice device, std::vector<SpvReflectShaderModule> p_modules) {
    std::vector<VkDescriptorSetLayout> out = {};
    std::unordered_map<size_t, std::vector<VkDescriptorSetLayoutBinding>> unique_bindings;
    std::unordered_map<size_t, VkDescriptorSetLayoutCreateInfo> unique_layouts;

    for (int i = 0; i < p_modules.size(); ++i) {
        SpvReflectShaderModule& module = p_modules[i];
        
        uint32_t count = 0;
        SpvReflectResult result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        
        std::vector<SpvReflectDescriptorSet*> sets(count);
        result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        
        for (int set_i = 0; set_i < sets.size(); ++set_i) {
            const SpvReflectDescriptorSet& refl_set = *(sets[set_i]);
            std::vector<VkDescriptorSetLayoutBinding> bindings(refl_set.binding_count);
            for (int binding_i = 0; binding_i < refl_set.binding_count; ++binding_i) {
                const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[binding_i]);
                VkDescriptorSetLayoutBinding layout_binding = {};
                layout_binding.binding = refl_binding.binding;
                layout_binding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
                layout_binding.descriptorCount = 1;
                layout_binding.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);
                for (int dim_i = 0; dim_i < refl_binding.array.dims_count; ++dim_i) {
                    layout_binding.descriptorCount *= refl_binding.array.dims[dim_i];
                }
                bindings[binding_i] = layout_binding;
            }

            // Create a hash for the bindings
            size_t hash = 0;
            for (const auto& binding : bindings) {
                hash ^= BindingHash{}(binding);
            }

            // Merge stages if the layout already exists
            if (unique_bindings.find(hash) != unique_bindings.end()) {
                for (int j = 0; j < bindings.size(); ++j) {
                    unique_bindings[hash][j].stageFlags |= bindings[j].stageFlags;
                }
            } else {
                // Store unique bindings
                unique_bindings[hash] = bindings;
            }
        }
    }

    // Create Vulkan descriptor set layouts from the unique layout infos
    for (const auto& entry : unique_bindings) {
        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = entry.second.size();
        info.pBindings = entry.second.data();

        VkDescriptorSetLayout layout;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &layout));
        out.push_back(layout);
    }

    return out;
}

void CuRenderDevice::create_render_pipeline(const std::vector<CompiledShaderInfo> p_shader_infos) {
    std::unordered_map<VkShaderStageFlagBits, VkShaderModule> shader_modules = {};
    std::vector<SpvReflectShaderModule> spv_reflect_modules = {};
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {};
    
    shader_stages.resize(p_shader_infos.size());
    spv_reflect_modules.resize(p_shader_infos.size());

    for (int i = 0; i < p_shader_infos.size(); ++i) {
        const CompiledShaderInfo shader_info = p_shader_infos[i];
        VkShaderModuleCreateInfo module_info = {};
        module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_info.pNext = nullptr;
        module_info.flags = 0;
        module_info.codeSize = shader_info.buffer.size() * sizeof(uint32_t);
        module_info.pCode = shader_info.buffer.data();
        VkShaderModule module;
        VK_CHECK(vkCreateShaderModule(device, &module_info, nullptr, &module));
        VkShaderStageFlagBits shader_stage;
        if (!get_shader_stage(shader_info, shader_stage)) {
            continue;
        }
        if (shader_modules.find(shader_stage) != nullptr) {
            ENGINE_WARN("This shader stage already exists");
            continue;
        }
        VkPipelineShaderStageCreateInfo stage_info  {};
        stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage_info.stage = shader_stage;
        stage_info.module = module;
        stage_info.pName = "main";
        shader_modules[shader_stage] = module;
        shader_stages[i] = stage_info;

        if (spvReflectCreateShaderModule(shader_info.buffer.size() * sizeof(uint32_t), shader_info.buffer.data(), &spv_reflect_modules[i]) != SPV_REFLECT_RESULT_SUCCESS) {
            ENGINE_ERROR("Failed to create reflect module for");
        }
    }

    const std::vector<VkDescriptorSetLayout> descriptor_layouts = generate_descriptor_layouts(device, spv_reflect_modules);

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.flags = 0;
    layout_info.pPushConstantRanges = nullptr;
    layout_info.pushConstantRangeCount = 0;
    layout_info.pSetLayouts = descriptor_layouts.data();
    layout_info.setLayoutCount = descriptor_layouts.size();

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(device, &layout_info, nullptr, &layout));

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.vertexAttributeDescriptionCount = 0;

    std::array<VkDynamicState, 2> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.flags = 0;
    dynamic_state_info.pDynamicStates = dynamic_states.data();
    dynamic_state_info.dynamicStateCount = dynamic_states.size();

    VkPipelineRenderingCreateInfoKHR rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachmentFormats = &draw_texture.format;
    rendering_info.viewMask = 0;

    VkPipelineViewportStateCreateInfo viewport_state_info = {};
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.pNext = nullptr;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.scissorCount =1;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineRasterizationStateCreateInfo rasterizer_info{};
    rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_info.depthClampEnable = VK_FALSE;
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_info.lineWidth = 1.0f;
    rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_info.depthBiasEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_info = {};
    color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_info.pNext = nullptr;
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attachment;

    VkPipelineMultisampleStateCreateInfo multisampling_info{};
    multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_info.sampleShadingEnable = VK_FALSE;
    multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.pNext = &rendering_info;
    pipeline_info.layout  = layout;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pDynamicState = &dynamic_state_info;
    pipeline_info.pColorBlendState = &color_blend_info;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pRasterizationState = &rasterizer_info;
    pipeline_info.pStages = shader_stages.data();
    pipeline_info.stageCount = shader_modules.size();
    pipeline_info.pMultisampleState = &multisampling_info;
    pipeline_info.renderPass = VK_NULL_HANDLE;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = 0;

    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline));

    for (std::pair<const VkShaderStageFlagBits, VkShaderModule>& element : shader_modules) {
        vkDestroyShaderModule(device, element.second, nullptr);
    }

    render_pipeline.layout = layout;
    render_pipeline.pipeline = pipeline;

    main_deletion_queue.push_function([&](){render_pipeline.clear(device);});
}

void CuRenderDevice::draw() {
    if (window->resize) {
        vkDeviceWaitIdle(device);
		swapchain.build();
    }
    FrameData& current_frame = frame_data[current_frame_idx];
    VK_CHECK(vkWaitForFences(device, 1, &current_frame.render_fence, true, 1000000000));

    uint32_t swapchain_img_index;
    VkResult next_img_result = vkAcquireNextImageKHR(device, swapchain.swapchain, 1000000000, current_frame.swapchain_semaphore, nullptr, &swapchain_img_index);

    if(next_img_result == VK_ERROR_OUT_OF_DATE_KHR) {
        return;
	}

    VK_CHECK(vkResetFences(device, 1, &current_frame.render_fence));

    VkCommandBuffer cmb = current_frame.cmb;

    VK_CHECK(vkResetCommandBuffer(cmb, 0));

    VkCommandBufferBeginInfo main_cmb_begin_info = {};
    main_cmb_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    main_cmb_begin_info.pNext = nullptr;
    main_cmb_begin_info.pInheritanceInfo = nullptr;
    main_cmb_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(cmb, &main_cmb_begin_info));

    transition_image(cmb, draw_texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    VkImageSubresourceRange clear_range = image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    VkClearColorValue clear_value;
    float flash = std::abs(std::sin(frame_count / 120.f));
	clear_value = { { 0.0f, 0.0f, flash, 1.0f } };
    
    vkCmdClearColorImage(cmb, draw_texture.image, VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);

    transition_image(cmb, draw_texture.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    
    VkRenderingAttachmentInfo color_attachment_info {};
    color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment_info.pNext = nullptr;

    color_attachment_info.imageView = draw_texture.view;
    color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.pNext = nullptr;
    rendering_info.renderArea = VkRect2D{
        VkOffset2D{0,0}, 
        VkExtent2D {draw_texture.extent.width, 
        draw_texture.extent.height}
    };
    rendering_info.pColorAttachments = &color_attachment_info;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pDepthAttachment = nullptr;
    rendering_info.pStencilAttachment = nullptr;
    rendering_info.layerCount = 1;

    vkCmdBeginRendering(cmb, &rendering_info);

    vkCmdBindPipeline(cmb, VK_PIPELINE_BIND_POINT_GRAPHICS, render_pipeline.pipeline);
    //set dynamic viewport and scissor
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = draw_texture.extent.width;
	viewport.height = draw_texture.extent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(cmb, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = draw_texture.extent.width;
	scissor.extent.height = draw_texture.extent.height;

	vkCmdSetScissor(cmb, 0, 1, &scissor);

	//launch a draw command to draw 3 vertices
	vkCmdDraw(cmb, 3, 1, 0, 0);

    vkCmdEndRendering(cmb);

    transition_image(cmb, draw_texture.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	transition_image(cmb, swapchain.images[swapchain_img_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);


	copy_image_to_image(cmb, draw_texture.image, swapchain.images[swapchain_img_index], {draw_texture.extent.width, draw_texture.extent.height}, swapchain.extent);

    transition_image(cmb, swapchain.images[swapchain_img_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    
    VK_CHECK(vkEndCommandBuffer(cmb));

    VkCommandBufferSubmitInfo cmb_submit_info = {};
    cmb_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmb_submit_info.pNext = nullptr;
    cmb_submit_info.commandBuffer = cmb;
    cmb_submit_info.deviceMask = 0;

    VkSemaphoreSubmitInfo wait_info = {};
    wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    wait_info.pNext = nullptr;
    wait_info.semaphore = current_frame.swapchain_semaphore;
    wait_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
    wait_info.deviceIndex = 0;
    wait_info.value = 1;

    VkSemaphoreSubmitInfo signal_info = {};
    signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signal_info.pNext = nullptr;
    signal_info.semaphore = current_frame.render_semaphore;
    signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    signal_info.deviceIndex = 0;
    signal_info.value = 1;

    VkSubmitInfo2 submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreInfoCount = 1;
    submit_info.pWaitSemaphoreInfos = &wait_info;
    submit_info.signalSemaphoreInfoCount = 1;
    submit_info.pSignalSemaphoreInfos = &signal_info;
    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = &cmb_submit_info;

    VK_CHECK(vkQueueSubmit2(graphics_queue, 1, &submit_info, current_frame.render_fence));

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.pSwapchains = &swapchain.swapchain;
    present_info.swapchainCount = 1;
    present_info.pWaitSemaphores = &current_frame.render_semaphore;
    present_info.waitSemaphoreCount = 1;
    present_info.pImageIndices = &swapchain_img_index;

    VkResult result = vkQueuePresentKHR(graphics_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || window->resize) {
        return;
	}
    else if (result != VK_SUCCESS) {
        ENGINE_ERROR("Failed to present graphics queue");
        return;
    }

    current_frame_idx = (current_frame_idx + 1) % FRAME_OVERLAP;
    frame_count++;
}

void CuRenderDevice::clear() {
    vkDeviceWaitIdle(device);
    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        FrameData& current_frame = frame_data[i];
        current_frame.deletion_queue.execute();
        
    }

    main_deletion_queue.execute();
}