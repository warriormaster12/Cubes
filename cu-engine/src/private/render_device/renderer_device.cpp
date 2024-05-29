#include "render_device.h"
#include <VkBootstrap.h>
#include <cmath>
#include "window.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>



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

RenderPipeline CuRenderDevice::create_render_pipeline(const std::vector<CompiledShaderInfo> p_shader_infos) {
    std::unordered_map<VkShaderStageFlagBits, VkShaderModule> shader_modules = {};
    shader_modules.reserve(p_shader_infos.size());
    for (int i = 0; i < p_shader_infos.size(); ++i) {
        const CompiledShaderInfo shader_info = p_shader_infos[i];
        VkShaderModuleCreateInfo module_info = {};
        module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_info.codeSize = shader_info.code_size;
        module_info.pCode = shader_info.code;
        VkShaderModule module;
        VK_CHECK(vkCreateShaderModule(device, &module_info, nullptr, &module));
        if (shader_modules.find(VK_SHADER_STAGE_VERTEX_BIT) != nullptr) {
            ENGINE_WARN("This shader stage already exists");
            continue;
        }

        shader_modules[VK_SHADER_STAGE_VERTEX_BIT] = module;
    }
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

    transition_image(cmb, draw_texture.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
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