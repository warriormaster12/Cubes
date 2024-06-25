#include "render_device.h"
#include "window.h"
#include <VkBootstrap.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

CuRenderDevice *CuRenderDevice::singleton = nullptr;

void Swapchain::build(
    VkPresentModeKHR p_present_mode /*= VK_PRESENT_MODE_FIFO_KHR*/) {
  clear();
  vkb::SwapchainBuilder swapchain_builder{physical_device, device, surface};
  vkb::Swapchain vkb_swapchain =
      swapchain_builder.use_default_format_selection()
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

void CuRenderAttachmentBuilder::add_color_attachment(
    std::array<float, 4> p_background_color /*= {0.0f, 0.0f, 0.0f, 1.0f}*/) {
  if (!(result.aspect_flags & VK_IMAGE_ASPECT_COLOR_BIT) ||
      result.aspect_flags == 0) {
    result.aspect_flags |= VK_IMAGE_ASPECT_COLOR_BIT;
  }
  result.background_color = p_background_color;
  VkRenderingAttachmentInfo color_attachment_info = {};
  color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  color_attachment_info.pNext = nullptr;

  color_attachment_info.imageView = nullptr; // we'll attach the view later
  color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  result.color_attachments.push_back(color_attachment_info);
}

void CuRenderAttachmentBuilder::add_depth_attachment() {
  // if (!(result.aspect_flags & VK_IMAGE_ASPECT_DEPTH_BIT) ||
  //     result.aspect_flags == 0) {
  //   result.aspect_flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
  // }
  VkRenderingAttachmentInfo depth_attachment_info = {};
  depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  depth_attachment_info.pNext = nullptr;

  depth_attachment_info.imageView = nullptr; // we'll attach the view later
  depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
  depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth_attachment_info.clearValue.depthStencil.depth = 1.f;

  result.depth_attachment = depth_attachment_info;
}

CuRenderAttachemnts CuRenderAttachmentBuilder::build() { return result; }

CuRenderDevice::CuRenderDevice() { singleton = this; }

CuRenderDevice *CuRenderDevice::get_singleton() { return singleton; }

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
  vkb::Result<vkb::Instance> inst_ret = builder.set_app_name("Vulkan app")
                                            .request_validation_layers()
                                            .require_api_version(1, 2)
                                            .use_default_debug_messenger()
                                            .build();
  if (!inst_ret) {
    ENGINE_ERROR("Failed to create Vulkan instance. Error: {}",
                 inst_ret.error().message());
    return false;
  }
  instance = inst_ret.value().instance;

  debug_messenger = inst_ret->debug_messenger;

  volkLoadInstance(instance);

  main_deletion_queue.push_function([&]() {
    vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, VK_NULL_HANDLE);
    vkDestroyInstance(instance, VK_NULL_HANDLE);
  });

  glfwCreateWindowSurface(instance, window->raw_window, nullptr, &surface);
  main_deletion_queue.push_function(
      [&]() { vkDestroySurfaceKHR(instance, surface, VK_NULL_HANDLE); });

  VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_extension = {};
  dynamic_rendering_extension.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
  dynamic_rendering_extension.dynamicRendering = VK_TRUE;

  VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2_extension = {};
  synchronization2_extension.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
  synchronization2_extension.pNext = nullptr;
  synchronization2_extension.synchronization2 = VK_TRUE;

  VkPhysicalDeviceVulkan12Features feats_12 = {};
  feats_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  feats_12.bufferDeviceAddress = true;
  feats_12.descriptorIndexing = true;
  vkb::PhysicalDeviceSelector selector{inst_ret.value()};
  vkb::Result<vkb::PhysicalDevice> phys_ret =
      selector.set_surface(surface)
          .set_minimum_version(1, 2) // require a vulkan 1.3 capable device
          .set_required_features_12(feats_12)
          .add_required_extensions({VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                                    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
                                    VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME})
          .prefer_gpu_device_type()
          .select();
  if (!phys_ret) {
    ENGINE_ERROR("Failed to select Vulkan Physical Device. Error: {}",
                 phys_ret.error().message());
    return false;
  }

  physical_device = phys_ret->physical_device;
  VkPhysicalDeviceProperties device_properties = {};
  vkGetPhysicalDeviceProperties(physical_device, &device_properties);
  ENGINE_INFO("Selected GPU: {}", device_properties.deviceName);
  vkb::DeviceBuilder device_builder{phys_ret.value()};
  // automatically propagate needed data from instance & physical device
  vkb::Result<vkb::Device> dev_ret =
      device_builder.add_pNext(&dynamic_rendering_extension)
          .add_pNext(&synchronization2_extension)
          .build();
  if (!dev_ret) {
    ENGINE_ERROR("Failed to create Vulkan device. Error: {}",
                 dev_ret.error().message());
    return false;
  }
  vkb::Device vkb_device = dev_ret.value();

  // Get the VkDevice handle used in the rest of a vulkan application
  device = vkb_device.device;

  volkLoadDevice(device);

  main_deletion_queue.push_function(
      [&]() { vkDestroyDevice(device, VK_NULL_HANDLE); });

  // Get the graphics queue with a helper function
  vkb::Result<VkQueue> graphics_queue_ret =
      vkb_device.get_queue(vkb::QueueType::graphics);
  if (!graphics_queue_ret) {
    ENGINE_ERROR("Failed to get graphics queue. Error: {}",
                 graphics_queue_ret.error().message());
    return false;
  }
  graphics_queue = graphics_queue_ret.value();
  graphics_queue_family =
      vkb_device.get_queue_index(vkb::QueueType::graphics).value();

  VmaVulkanFunctions vulkan_functions = {};
  vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
  vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
  vulkan_functions.vkGetPhysicalDeviceProperties =
      vkGetPhysicalDeviceProperties;
  vulkan_functions.vkGetPhysicalDeviceMemoryProperties =
      vkGetPhysicalDeviceMemoryProperties;
  vulkan_functions.vkAllocateMemory = vkAllocateMemory;
  vulkan_functions.vkFreeMemory = vkFreeMemory;
  vulkan_functions.vkMapMemory = vkMapMemory;
  vulkan_functions.vkUnmapMemory = vkUnmapMemory;
  vulkan_functions.vkGetDeviceBufferMemoryRequirements =
      vkGetDeviceBufferMemoryRequirements;
  vulkan_functions.vkGetDeviceImageMemoryRequirements =
      vkGetDeviceImageMemoryRequirements;
  vulkan_functions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
  vulkan_functions.vkInvalidateMappedMemoryRanges =
      vkInvalidateMappedMemoryRanges;
  vulkan_functions.vkBindBufferMemory = vkBindBufferMemory;
  vulkan_functions.vkBindImageMemory = vkBindImageMemory;
  vulkan_functions.vkGetBufferMemoryRequirements =
      vkGetBufferMemoryRequirements;
  vulkan_functions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
  vulkan_functions.vkCreateBuffer = vkCreateBuffer;
  vulkan_functions.vkCreateImage = vkCreateImage;
  vulkan_functions.vkDestroyBuffer = vkDestroyBuffer;
  vulkan_functions.vkDestroyImage = vkDestroyImage;
  vulkan_functions.vkCmdCopyBuffer = vkCmdCopyBuffer;
  vulkan_functions.vkGetBufferMemoryRequirements2KHR =
      vkGetBufferMemoryRequirements2KHR;
  vulkan_functions.vkGetImageMemoryRequirements2KHR =
      vkGetImageMemoryRequirements2KHR;
  vulkan_functions.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
  vulkan_functions.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
  vulkan_functions.vkGetPhysicalDeviceMemoryProperties2KHR =
      vkGetPhysicalDeviceMemoryProperties2KHR;
  vulkan_functions.vkGetDeviceBufferMemoryRequirements =
      vkGetDeviceBufferMemoryRequirements;
  vulkan_functions.vkGetDeviceImageMemoryRequirements =
      vkGetDeviceImageMemoryRequirements;

  VmaAllocatorCreateInfo allocator_create_info = {};
  allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  allocator_create_info.physicalDevice = physical_device;
  allocator_create_info.device = device;
  allocator_create_info.instance = instance;
  allocator_create_info.pVulkanFunctions =
      (const VmaVulkanFunctions *)&vulkan_functions;

  vmaCreateAllocator(&allocator_create_info, &allocator);

  main_deletion_queue.push_function([&]() { vmaDestroyAllocator(allocator); });

  swapchain = Swapchain(surface, device, physical_device, window);
  swapchain.build();

  main_deletion_queue.push_function([&]() { swapchain.clear(); });

  // per frame resource allocation
  {
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
    render_swapchain_semaphore_info.sType =
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    render_swapchain_semaphore_info.pNext = nullptr;
    render_swapchain_semaphore_info.flags = 0;

    for (int i = 0; i < FRAME_OVERLAP; ++i) {
      FrameData &current_frame = frame_data[i];
      VK_CHECK(vkCreateCommandPool(device, &command_pool_info, nullptr,
                                   &current_frame.cmp));

      VkCommandBufferAllocateInfo command_buffer_alloc_info = {};
      command_buffer_alloc_info.sType =
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      command_buffer_alloc_info.commandPool = current_frame.cmp;
      command_buffer_alloc_info.pNext = nullptr;
      command_buffer_alloc_info.commandBufferCount = 1;
      command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_alloc_info,
                                        &current_frame.cmb));

      VK_CHECK(vkCreateSemaphore(device, &render_swapchain_semaphore_info,
                                 nullptr, &current_frame.swapchain_semaphore));
      VK_CHECK(vkCreateSemaphore(device, &render_swapchain_semaphore_info,
                                 nullptr, &current_frame.render_semaphore));

      VK_CHECK(vkCreateFence(device, &render_fence_info, nullptr,
                             &current_frame.render_fence));

      std::vector<DescriptorAllocator::PoolSizeRatio> frame_sizes = {
          {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
          {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
      };

      current_frame.descriptor_allocator.init(device, 1000, frame_sizes);

      current_frame.deletion_queue.push_function([&]() {
        current_frame.descriptor_allocator.destroy_pools(device);
        vkDestroySemaphore(device, current_frame.swapchain_semaphore, nullptr);
        vkDestroySemaphore(device, current_frame.render_semaphore, nullptr);
        vkDestroyFence(device, current_frame.render_fence, nullptr);
        vkDestroyCommandPool(device, current_frame.cmp, nullptr);
      });
    }
  }

  // immediate resource allocation
  {
    VkCommandPoolCreateInfo command_pool_info = {};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.queueFamilyIndex = graphics_queue_family;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.pNext = nullptr;

    VK_CHECK(
        vkCreateCommandPool(device, &command_pool_info, nullptr, &imm_cpool));

    VkCommandBufferAllocateInfo command_buffer_alloc_info = {};
    command_buffer_alloc_info.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.commandPool = imm_cpool;
    command_buffer_alloc_info.pNext = nullptr;
    command_buffer_alloc_info.commandBufferCount = 1;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VK_CHECK(
        vkAllocateCommandBuffers(device, &command_buffer_alloc_info, &imm_cmb));

    VkFenceCreateInfo imm_fence_info = {};
    imm_fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    imm_fence_info.pNext = nullptr;
    imm_fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(device, &imm_fence_info, nullptr, &imm_fence));

    main_deletion_queue.push_function([&]() {
      vkDestroyFence(device, imm_fence, nullptr);
      vkDestroyCommandPool(device, imm_cpool, nullptr);
    });
  }

  return true;
}

Buffer CuRenderDevice::create_buffer(size_t p_size, VkBufferUsageFlags p_usage,
                                     VmaMemoryUsage p_memory_usage) {
  VkBufferCreateInfo buffer_info = {};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = p_size;
  buffer_info.usage = p_usage;

  VmaAllocationCreateInfo vma_alloc_info = {};
  vma_alloc_info.usage = p_memory_usage;
  vma_alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

  Buffer out_buffer;

  VK_CHECK(vmaCreateBuffer(allocator, &buffer_info, &vma_alloc_info,
                           &out_buffer.buffer, &out_buffer.allocation,
                           &out_buffer.info));

  return out_buffer;
}

void CuRenderDevice::write_buffer(void *p_data, size_t p_size, Buffer &buffer) {
  if (buffer.buffer == VK_NULL_HANDLE) {
    ENGINE_WARN("Can't write data to a buffer that isn't created");
    return;
  }
  char *buffer_data;
  vmaMapMemory(allocator, buffer.allocation, (void **)&buffer_data);
  memcpy(buffer_data, p_data, p_size);
  vmaUnmapMemory(allocator, buffer.allocation);
}

void CuRenderDevice::copy_buffer(const Buffer &p_source,
                                 const Buffer &p_destination,
                                 VkDeviceSize p_size,
                                 VkDeviceSize p_source_offset,
                                 VkDeviceSize p_destination_offset) {
  VkBufferCopy buffer_copy = {};
  buffer_copy.size = p_size;
  buffer_copy.srcOffset = p_source_offset;
  buffer_copy.dstOffset = p_destination_offset;

  vkCmdCopyBuffer(imm_cmb, p_source.buffer, p_destination.buffer, 1,
                  &buffer_copy);
}

void CuRenderDevice::clear_buffer(Buffer p_buffer) {
  if (p_buffer.buffer == VK_NULL_HANDLE)
    return;
  vmaDestroyBuffer(allocator, p_buffer.buffer, p_buffer.allocation);
}

bool CuRenderDevice::create_texture(VkFormat p_format, VkExtent3D p_extent,
                                    VkImageUsageFlags p_image_usage,
                                    VmaMemoryUsage p_memory_usage,
                                    Texture &p_out_texture) {
  p_out_texture.format = p_format;
  p_out_texture.extent = p_extent;
  VkImageCreateInfo image_info = image_create_info(
      p_out_texture.format, p_image_usage, p_out_texture.extent);

  VmaAllocationCreateInfo img_alloc_info = {};
  img_alloc_info.usage = p_memory_usage;
  img_alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  VK_CHECK(vmaCreateImage(allocator, &image_info, &img_alloc_info,
                          &p_out_texture.image, &p_out_texture.allocation,
                          nullptr));

  if ((p_image_usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
    p_out_texture.aspect_flags |= VK_IMAGE_ASPECT_COLOR_BIT;
  }

  if ((p_image_usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
    // For now we only support depth aspect.
    p_out_texture.aspect_flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
  }
  VkImageViewCreateInfo rview_info = imageview_create_info(
      p_out_texture.format, p_out_texture.image, p_out_texture.aspect_flags);

  VK_CHECK(
      vkCreateImageView(device, &rview_info, nullptr, &p_out_texture.view));

  return true;
}

void CuRenderDevice::clear_texture(Texture &p_texture) {
  if (p_texture.view == VK_NULL_HANDLE)
    return;
  vkDestroyImageView(device, p_texture.view, nullptr);
  if (p_texture.image == VK_NULL_HANDLE)
    return;
  vmaDestroyImage(allocator, p_texture.image, p_texture.allocation);
}

void DescriptorAllocator::init(VkDevice p_device, uint32_t p_max_sets,
                               std::span<PoolSizeRatio> p_pool_ratios) {
  ratios.clear();

  for (PoolSizeRatio r : p_pool_ratios) {
    ratios.push_back(r);
  }

  VkDescriptorPool new_pool = create_pool(p_device, p_max_sets, p_pool_ratios);

  sets_per_pool = p_max_sets * 1.5; // grow it next allocation

  ready_pools.push_back(new_pool);
}

void DescriptorAllocator::clear_pools(VkDevice p_device) {
  for (VkDescriptorPool p : ready_pools) {
    vkResetDescriptorPool(p_device, p, 0);
  }
  for (VkDescriptorPool p : full_pools) {
    vkResetDescriptorPool(p_device, p, 0);
    ready_pools.push_back(p);
  }
  full_pools.clear();
}

void DescriptorAllocator::destroy_pools(VkDevice p_device) {
  for (auto p : ready_pools) {
    vkDestroyDescriptorPool(p_device, p, nullptr);
  }
  ready_pools.clear();
  for (auto p : full_pools) {
    vkDestroyDescriptorPool(p_device, p, nullptr);
  }
  full_pools.clear();
}

VkDescriptorPool DescriptorAllocator::get_pool(VkDevice p_device) {
  VkDescriptorPool new_pool;
  if (ready_pools.size() != 0) {
    new_pool = ready_pools.back();
    ready_pools.pop_back();
  } else {
    // need to create a new pool
    new_pool = create_pool(p_device, sets_per_pool, ratios);

    sets_per_pool = sets_per_pool * 1.5;
    if (sets_per_pool > 4092) {
      sets_per_pool = 4092;
    }
  }

  return new_pool;
}

VkDescriptorPool
DescriptorAllocator::create_pool(VkDevice p_device, uint32_t p_set_count,
                                 std::span<PoolSizeRatio> p_pool_ratios) {
  std::vector<VkDescriptorPoolSize> poolSizes;
  for (PoolSizeRatio ratio : p_pool_ratios) {
    poolSizes.push_back(VkDescriptorPoolSize{
        .type = ratio.type,
        .descriptorCount = uint32_t(ratio.ratio * p_set_count)});
  }

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = 0;
  pool_info.maxSets = p_set_count;
  pool_info.poolSizeCount = (uint32_t)poolSizes.size();
  pool_info.pPoolSizes = poolSizes.data();

  VkDescriptorPool new_pool;
  vkCreateDescriptorPool(p_device, &pool_info, nullptr, &new_pool);
  return new_pool;
}

VkDescriptorSet
DescriptorAllocator::allocate(VkDevice p_device, VkDescriptorSetLayout p_layout,
                              uint32_t p_descriptor_set_count /*= 1*/,
                              void *pNext /*= nullptr*/) {
  // get or create a pool to allocate from
  VkDescriptorPool pool_to_use = get_pool(p_device);

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.pNext = pNext;
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool_to_use;
  allocInfo.descriptorSetCount = p_descriptor_set_count;
  allocInfo.pSetLayouts = &p_layout;

  VkDescriptorSet ds;
  VkResult result = vkAllocateDescriptorSets(p_device, &allocInfo, &ds);

  // allocation failed. Try again
  if (result == VK_ERROR_OUT_OF_POOL_MEMORY ||
      result == VK_ERROR_FRAGMENTED_POOL) {

    full_pools.push_back(pool_to_use);

    pool_to_use = get_pool(p_device);
    allocInfo.descriptorPool = pool_to_use;

    VK_CHECK(vkAllocateDescriptorSets(p_device, &allocInfo, &ds));
  }

  ready_pools.push_back(pool_to_use);
  return ds;
}

bool get_shader_stage(const CompiledShaderInfo p_shader_info,
                      VkShaderStageFlagBits &p_out_shader_stage) {
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
  size_t operator()(const VkDescriptorSetLayoutBinding &binding) const {
    size_t hash = std::hash<uint32_t>{}(binding.binding) ^
                  std::hash<uint32_t>{}(binding.descriptorType) ^
                  std::hash<uint32_t>{}(binding.descriptorCount);
    return hash;
  }
};

// Equality function for VkDescriptorSetLayoutBinding
struct BindingEqual {
  bool operator()(const VkDescriptorSetLayoutBinding &lhs,
                  const VkDescriptorSetLayoutBinding &rhs) const {
    return lhs.binding == rhs.binding &&
           lhs.descriptorType == rhs.descriptorType &&
           lhs.descriptorCount == rhs.descriptorCount;
  }
};

const PipelineLayoutInfo LayoutAllocator::generate_pipeline_info(
    VkDevice p_device, const std::vector<CompiledShaderInfo> &p_shader_infos) {
  std::vector<VkDescriptorSetLayout> out_layouts = {};
  std::vector<VkPushConstantRange> out_push_constant = {};
  std::vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions =
      {};
  std::vector<VkVertexInputBindingDescription> vertex_binding_descriptions = {};

  std::unordered_map<uint32_t,
                     std::unordered_map<size_t, VkDescriptorSetLayoutBinding>>
      unique_bindings;
  std::unordered_map<size_t, VkPushConstantRange> unique_push_constants;
  std::unordered_map<VkShaderStageFlagBits, SpvReflectShaderModule>
      reflect_modules = {};

  for (int i = 0; i < p_shader_infos.size(); ++i) {
    const CompiledShaderInfo shader_info = p_shader_infos[i];

    VkShaderStageFlagBits shader_stage;
    if (!get_shader_stage(shader_info, shader_stage)) {
      continue;
    }
    if (reflect_modules.find(shader_stage) != reflect_modules.end()) {
      ENGINE_WARN("This reflect shader stage already exists");
      continue;
    }

    SpvReflectShaderModule module;
    if (spvReflectCreateShaderModule(
            shader_info.buffer.size() * sizeof(uint32_t),
            shader_info.buffer.data(), &module) != SPV_REFLECT_RESULT_SUCCESS) {
      ENGINE_ERROR("Failed to create reflect module");
    }

    reflect_modules[shader_stage] = module;
  }

  // Reflect Vertex layout
  for (std::pair<VkShaderStageFlagBits, SpvReflectShaderModule> entry :
       reflect_modules) {

    SpvReflectShaderModule &module = entry.second;
    if (module.shader_stage ==
        static_cast<VkShaderStageFlags>(VK_SHADER_STAGE_VERTEX_BIT)) {
      // Enumerate and extract shader's input variables
      uint32_t varCount = 0;
      SpvReflectResult result =
          spvReflectEnumerateInputVariables(&module, &varCount, nullptr);
      assert(result == SPV_REFLECT_RESULT_SUCCESS);
      std::vector<SpvReflectInterfaceVariable *> inputVars(varCount);
      result = spvReflectEnumerateInputVariables(&module, &varCount,
                                                 inputVars.data());
      assert(result == SPV_REFLECT_RESULT_SUCCESS);

      VkVertexInputBindingDescription binding_description = {};
      binding_description.binding = 0;
      binding_description.stride = 0;
      binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

      for (int i = 0; i < inputVars.size(); i++) {
        const SpvReflectInterfaceVariable &refl_var = *(inputVars[i]);
        if (refl_var.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
          continue;
        }

        VkVertexInputAttributeDescription attribute = {};
        attribute.binding = binding_description.binding;
        attribute.location = refl_var.location;
        attribute.format = static_cast<VkFormat>(inputVars[i]->format);
        attribute.offset = 0;
        vertex_attribute_descriptions.push_back(attribute);
      }

      if (vertex_attribute_descriptions.size() == 0) {
        break;
      }

      std::sort(vertex_attribute_descriptions.begin(),
                vertex_attribute_descriptions.end(),
                [](const VkVertexInputAttributeDescription &a,
                   const VkVertexInputAttributeDescription &b) {
                  return a.location < b.location;
                });

      for (VkVertexInputAttributeDescription &attribute :
           vertex_attribute_descriptions) {
        attribute.offset = binding_description.stride;
        binding_description.stride += get_format_size(attribute.format);
      }

      vertex_binding_descriptions.push_back(binding_description);
    }
  }
  // Reflect push constants
  for (std::pair<VkShaderStageFlagBits, SpvReflectShaderModule> entry :
       reflect_modules) {
    SpvReflectShaderModule &module = entry.second;

    uint32_t count = 0;
    SpvReflectResult result =
        spvReflectEnumeratePushConstantBlocks(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectBlockVariable *> push_constants(count);
    result = spvReflectEnumeratePushConstantBlocks(&module, &count,
                                                   push_constants.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    for (int i = 0; i < push_constants.size(); ++i) {
      const SpvReflectBlockVariable &constant = *(push_constants[i]);

      VkPushConstantRange push_constant_range = {};
      push_constant_range.stageFlags =
          static_cast<VkShaderStageFlagBits>(module.shader_stage);
      push_constant_range.offset = constant.offset;
      push_constant_range.size = constant.size;

      size_t hash = std::hash<uint32_t>{}(push_constant_range.offset) ^
                    std::hash<uint32_t>{}(push_constant_range.size);

      if (unique_push_constants.find(hash) != unique_push_constants.end()) {
        unique_push_constants[hash].stageFlags |=
            push_constant_range.stageFlags;
      } else {
        unique_push_constants[hash] = push_constant_range;
      }
    }
  }
  // Reflect descriptor sets
  for (std::pair<VkShaderStageFlagBits, SpvReflectShaderModule> entry :
       reflect_modules) {
    SpvReflectShaderModule &module = entry.second;

    uint32_t count = 0;
    SpvReflectResult result =
        spvReflectEnumerateDescriptorSets(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectDescriptorSet *> sets(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    for (int set_i = 0; set_i < sets.size(); ++set_i) {
      const SpvReflectDescriptorSet &refl_set = *(sets[set_i]);
      std::unordered_map<size_t, VkDescriptorSetLayoutBinding> bindings;
      for (int binding_i = 0; binding_i < refl_set.binding_count; ++binding_i) {
        const SpvReflectDescriptorBinding &refl_binding =
            *(refl_set.bindings[binding_i]);
        VkDescriptorSetLayoutBinding layout_binding = {};
        layout_binding.binding = refl_binding.binding;
        layout_binding.descriptorType =
            static_cast<VkDescriptorType>(refl_binding.descriptor_type);
        layout_binding.descriptorCount = 1;
        layout_binding.stageFlags =
            static_cast<VkShaderStageFlagBits>(module.shader_stage);
        for (int dim_i = 0; dim_i < refl_binding.array.dims_count; ++dim_i) {
          layout_binding.descriptorCount *= refl_binding.array.dims[dim_i];
        }
        size_t binding_hash = BindingHash{}(layout_binding);
        if (bindings.find(binding_hash) != bindings.end()) {
          bindings[binding_hash].stageFlags |= layout_binding.stageFlags;
        } else {
          bindings[binding_hash] = layout_binding;
        }
      }

      // Merge stages if the layout already exists
      if (unique_bindings.find(refl_set.set) != unique_bindings.end()) {
        for (auto &[hash, binding] : bindings) {
          unique_bindings[refl_set.set][hash].stageFlags |= binding.stageFlags;
        }
      } else {
        // Store unique bindings
        unique_bindings[refl_set.set] = bindings;
      }
    }
  }

  // Create Vulkan descriptor set layouts from the unique layout infos
  for (const auto &set_entry : unique_bindings) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (const auto &binding_entry : set_entry.second) {
      bindings.push_back(binding_entry.second);
    }

    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = bindings.size();
    info.pBindings = bindings.data();

    VkDescriptorSetLayout layout;
    VK_CHECK(vkCreateDescriptorSetLayout(p_device, &info, nullptr, &layout));

    out_layouts.push_back(layout);
    descriptor_layouts.push_back(layout);
  }

  for (const auto &entry : unique_push_constants) {
    out_push_constant.push_back(entry.second);
  }

  PipelineLayoutInfo info = {};
  info.descriptor_layouts = out_layouts;
  info.push_constants = out_push_constant;
  info.vertex_attributes = vertex_attribute_descriptions;
  info.vertex_bindings = vertex_binding_descriptions;

  return info;
}

void LayoutAllocator::clear() {
  CuRenderDevice *device = CuRenderDevice::get_singleton();
  if (!device) {
    ENGINE_ERROR("Can't find device in LayoutAllocator");
    return;
  }

  for (int i = 0; i < descriptor_layouts.size(); ++i) {
    vkDestroyDescriptorSetLayout(device->get_raw_device(),
                                 descriptor_layouts[i], nullptr);
  }
}

void CuRenderDevice::immediate_submit(std::function<void()> &&p_function) {
  VK_CHECK(vkResetFences(device, 1, &imm_fence));
  VK_CHECK(vkResetCommandBuffer(imm_cmb, 0));

  VkCommandBufferBeginInfo imm_cmb_begin_info = {};
  imm_cmb_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  imm_cmb_begin_info.pNext = nullptr;
  imm_cmb_begin_info.pInheritanceInfo = nullptr;
  imm_cmb_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VK_CHECK(vkBeginCommandBuffer(imm_cmb, &imm_cmb_begin_info));

  p_function();

  VK_CHECK(vkEndCommandBuffer(imm_cmb));

  VkCommandBufferSubmitInfo cmb_submit_info = {};
  cmb_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  cmb_submit_info.pNext = nullptr;
  cmb_submit_info.commandBuffer = imm_cmb;
  cmb_submit_info.deviceMask = 0;

  VkSubmitInfo2KHR submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR;
  submit_info.pNext = nullptr;
  submit_info.waitSemaphoreInfoCount = 0;
  submit_info.pWaitSemaphoreInfos = nullptr;
  submit_info.signalSemaphoreInfoCount = 0;
  submit_info.pSignalSemaphoreInfos = nullptr;
  submit_info.commandBufferInfoCount = 1;
  submit_info.pCommandBufferInfos = &cmb_submit_info;

  VK_CHECK(vkQueueSubmit2KHR(graphics_queue, 1, &submit_info, imm_fence));

  VK_CHECK(vkWaitForFences(device, 1, &imm_fence, true, 9999999999));
}

RenderPipeline CuRenderDevice::create_render_pipeline(
    const std::vector<CompiledShaderInfo> &p_shader_infos,
    const VkBool32 p_depth_write_test, const VkCompareOp p_depth_compare_op,
    const std::vector<Texture> p_color_textures /*= {}*/,
    const VkFormat p_depth_format /*= VK_FORMAT_UNDEFINED*/) {
  std::unordered_map<VkShaderStageFlagBits, VkShaderModule> shader_modules = {};
  std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {};

  shader_stages.resize(p_shader_infos.size());

  RenderPipeline out_pipeline = {};

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
    if (shader_modules.find(shader_stage) != shader_modules.end()) {
      ENGINE_WARN("This shader stage already exists");
      continue;
    }
    VkPipelineShaderStageCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.stage = shader_stage;
    stage_info.module = module;
    stage_info.pName = "main";
    shader_modules[shader_stage] = module;
    shader_stages[i] = stage_info;
  }

  PipelineLayoutInfo pipeline_layout_info =
      main_layout_allocator.generate_pipeline_info(device, p_shader_infos);

  VkPipelineLayoutCreateInfo layout_info = {};
  layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layout_info.flags = 0;
  layout_info.pPushConstantRanges = pipeline_layout_info.push_constants.data();
  layout_info.pushConstantRangeCount =
      pipeline_layout_info.push_constants.size();
  layout_info.pSetLayouts = pipeline_layout_info.descriptor_layouts.data();
  layout_info.setLayoutCount = pipeline_layout_info.descriptor_layouts.size();

  VK_CHECK(vkCreatePipelineLayout(device, &layout_info, nullptr,
                                  &out_pipeline.layout));

  VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
  input_assembly_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_info.primitiveRestartEnable = VK_FALSE;

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.pVertexBindingDescriptions =
      pipeline_layout_info.vertex_bindings.data();
  vertex_input_info.vertexBindingDescriptionCount =
      pipeline_layout_info.vertex_bindings.size();
  vertex_input_info.pVertexAttributeDescriptions =
      pipeline_layout_info.vertex_attributes.data();
  vertex_input_info.vertexAttributeDescriptionCount =
      pipeline_layout_info.vertex_attributes.size();

  std::array<VkDynamicState, 2> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
                                                  VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
  dynamic_state_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state_info.flags = 0;
  dynamic_state_info.pDynamicStates = dynamic_states.data();
  dynamic_state_info.dynamicStateCount = dynamic_states.size();

  std::vector<VkFormat> color_formats = {};
  color_formats.resize(p_color_textures.size() > 0 ? p_color_textures.size()
                                                   : 0);

  if (p_color_textures.size() > 0) {
    for (int i = 0; i < p_color_textures.size(); ++i) {
      color_formats[i] = p_color_textures[i].format;
    }
  } else {
    color_formats[0] = swapchain.format;
  }

  VkPipelineRenderingCreateInfoKHR rendering_info = {};
  rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
  rendering_info.colorAttachmentCount = color_formats.size();
  rendering_info.pColorAttachmentFormats = color_formats.data();
  rendering_info.depthAttachmentFormat = p_depth_format;
  rendering_info.viewMask = 0;

  VkPipelineViewportStateCreateInfo viewport_state_info = {};
  viewport_state_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state_info.pNext = nullptr;
  viewport_state_info.viewportCount = 1;
  viewport_state_info.scissorCount = 1;

  VkPipelineColorBlendAttachmentState color_blend_attachment{};
  color_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;

  VkPipelineRasterizationStateCreateInfo rasterizer_info{};
  rasterizer_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer_info.depthClampEnable = VK_FALSE;
  rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
  rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer_info.lineWidth = 1.0f;
  rasterizer_info.cullMode = VK_CULL_MODE_NONE;
  rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer_info.depthBiasEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo color_blend_info = {};
  color_blend_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend_info.pNext = nullptr;
  color_blend_info.logicOpEnable = VK_FALSE;
  color_blend_info.logicOp = VK_LOGIC_OP_COPY;
  color_blend_info.attachmentCount = 1;
  color_blend_info.pAttachments = &color_blend_attachment;

  VkPipelineMultisampleStateCreateInfo multisampling_info{};
  multisampling_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling_info.sampleShadingEnable = VK_FALSE;
  multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
  depth_stencil_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_info.depthCompareOp = p_depth_compare_op;
  depth_stencil_info.depthTestEnable = p_depth_write_test;
  depth_stencil_info.depthWriteEnable = VK_TRUE;
  depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
  depth_stencil_info.minDepthBounds = 0.0f;
  depth_stencil_info.maxDepthBounds = 1.0f;

  // No stencil support atm
  depth_stencil_info.stencilTestEnable = VK_FALSE;
  depth_stencil_info.front = {}; // Optional
  depth_stencil_info.back = {};  // Optional

  VkGraphicsPipelineCreateInfo pipeline_info = {};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.pNext = &rendering_info;
  pipeline_info.layout = out_pipeline.layout;
  pipeline_info.pInputAssemblyState = &input_assembly_info;
  pipeline_info.pDynamicState = &dynamic_state_info;
  pipeline_info.pColorBlendState = &color_blend_info;
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pRasterizationState = &rasterizer_info;
  pipeline_info.pDepthStencilState = &depth_stencil_info;
  pipeline_info.pStages = shader_stages.data();
  pipeline_info.stageCount = shader_modules.size();
  pipeline_info.pMultisampleState = &multisampling_info;
  pipeline_info.renderPass = VK_NULL_HANDLE;
  pipeline_info.pViewportState = &viewport_state_info;
  pipeline_info.subpass = 0;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_info.basePipelineIndex = 0;

  VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info,
                                     nullptr, &out_pipeline.pipeline));

  for (std::pair<const VkShaderStageFlagBits, VkShaderModule> &element :
       shader_modules) {
    vkDestroyShaderModule(device, element.second, nullptr);
  }

  std::vector<VkDescriptorSet> descriptor_sets = {};
  int index = 0;
  out_pipeline.sets.resize(FRAME_OVERLAP *
                           pipeline_layout_info.descriptor_layouts.size());
  for (int i = 0; i < FRAME_OVERLAP; ++i) {
    FrameData &current_frame = frame_data[i];
    for (int j = 0; j < pipeline_layout_info.descriptor_layouts.size(); ++j) {
      VkDescriptorSet set = current_frame.descriptor_allocator.allocate(
          device, pipeline_layout_info.descriptor_layouts[j]);
      out_pipeline.sets[index] = set;
      ++index;
    }
  }

  return out_pipeline;
}

void CuRenderDevice::prepare_image(
    CuRenderAttachemnts &p_render_attachments, const Texture *p_color_texture,
    const Texture *p_depth_texture /*= nullptr*/) {
  if (p_render_attachments.color_attachments.size() == 0) {
    ENGINE_ERROR("No render attachments provided for this pass");
    return;
  }

  if (p_color_texture == nullptr) {
    ENGINE_ERROR("Color texture cannot be nullptr");
    return;
  }

  FrameData &current_frame = frame_data[current_frame_idx];
  VkCommandBuffer cmb = current_frame.cmb;
  transition_image(cmb, p_color_texture->image, VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_GENERAL);

  VkImageSubresourceRange subresource_range =
      image_subresource_range(p_color_texture->aspect_flags);

  if (p_render_attachments.color_attachments.size() > 0) {
    VkClearColorValue clearValue = {{p_render_attachments.background_color[0],
                                     p_render_attachments.background_color[1],
                                     p_render_attachments.background_color[2],
                                     p_render_attachments.background_color[3]}};
    vkCmdClearColorImage(cmb, p_color_texture->image, VK_IMAGE_LAYOUT_GENERAL,
                         &clearValue, 1, &subresource_range);
  }

  transition_image(cmb, p_color_texture->image, VK_IMAGE_LAYOUT_GENERAL,
                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  for (int i = 0; i < p_render_attachments.color_attachments.size(); ++i) {
    p_render_attachments.color_attachments[i].imageView = p_color_texture->view;
  }

  if (p_depth_texture) {
    p_render_attachments.depth_attachment.imageView = p_depth_texture->view;

    transition_image(cmb, p_depth_texture->image, VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
  }

  VkRenderingInfo rendering_info = {};
  rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  rendering_info.pNext = nullptr;
  rendering_info.renderArea =
      VkRect2D{VkOffset2D{0, 0}, VkExtent2D{p_color_texture->extent.width,
                                            p_color_texture->extent.height}};
  rendering_info.pColorAttachments =
      p_render_attachments.color_attachments.data();
  rendering_info.colorAttachmentCount =
      p_render_attachments.color_attachments.size();
  rendering_info.pDepthAttachment =
      p_depth_texture ? &p_render_attachments.depth_attachment : nullptr;
  rendering_info.pStencilAttachment = nullptr;
  rendering_info.layerCount = 1;

  vkCmdBeginRenderingKHR(cmb, &rendering_info);

  // set dynamic viewport and scissor
  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = p_color_texture->extent.width;
  viewport.height = p_color_texture->extent.height;
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;

  vkCmdSetViewport(cmb, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = p_color_texture->extent.width;
  scissor.extent.height = p_color_texture->extent.height;

  vkCmdSetScissor(cmb, 0, 1, &scissor);
}

void CuRenderDevice::begin_recording() {
  if (window->resize) {
    vkDeviceWaitIdle(device);
    swapchain.build();
  }
  FrameData &current_frame = frame_data[current_frame_idx];
  VK_CHECK(vkWaitForFences(device, 1, &current_frame.render_fence, true,
                           1000000000));

  VkResult next_img_result = vkAcquireNextImageKHR(
      device, swapchain.swapchain, 1000000000,
      current_frame.swapchain_semaphore, nullptr, &swapchain_img_index);

  if (next_img_result == VK_ERROR_OUT_OF_DATE_KHR) {
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
}

void CuRenderDevice::bind_pipeline(const RenderPipeline &p_pipeline) {
  VkCommandBuffer cmb = frame_data[current_frame_idx].cmb;
  if (p_pipeline.pipeline == VK_NULL_HANDLE) {
    ENGINE_WARN("Render pipeline not allocated. Fix it.");
    return;
  }
  vkCmdBindPipeline(cmb, VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline.pipeline);
}

void CuRenderDevice::bind_descriptor(const RenderPipeline &p_pipeline,
                                     const uint32_t p_index) {
  VkCommandBuffer cmb = frame_data[current_frame_idx].cmb;
  if (p_pipeline.pipeline == VK_NULL_HANDLE) {
    return;
  }
  int set_count = p_pipeline.sets.size() / FRAME_OVERLAP;
  if (p_index >= set_count) {
    ENGINE_WARN("Cannot bind set: {}. Not that many sets have been allocated",
                p_index);
    return;
  }
  VkDescriptorSet set =
      set_count > 1
          ? p_pipeline.sets[FRAME_OVERLAP * current_frame_idx + p_index]
          : p_pipeline.sets[current_frame_idx];
  vkCmdBindDescriptorSets(cmb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          p_pipeline.layout, p_index, 1, &set, 0, 0);
}

void CuRenderDevice::bind_push_constant(const RenderPipeline &p_pipeline,
                                        VkShaderStageFlags p_shaderStages,
                                        uint32_t p_offset, uint32_t p_size,
                                        void *p_data) {
  VkCommandBuffer cmb = frame_data[current_frame_idx].cmb;
  vkCmdPushConstants(cmb, p_pipeline.layout, p_shaderStages, p_offset, p_size,
                     p_data);
}

void CuRenderDevice::bind_vertex_buffer(uint32_t p_first_binding,
                                        uint32_t p_binding_count,
                                        std::vector<Buffer> p_buffers,
                                        std::vector<VkDeviceSize> p_offsets) {
  VkCommandBuffer cmb = frame_data[current_frame_idx].cmb;
  std::vector<VkBuffer> raw_buffers(p_buffers.size());
  for (int i = 0; i < p_buffers.size(); ++i) {
    raw_buffers[i] = p_buffers[i].buffer;
  }
  vkCmdBindVertexBuffers(cmb, p_first_binding, p_binding_count,
                         raw_buffers.data(), p_offsets.data());
}

void CuRenderDevice::bind_index_buffer(const Buffer &p_buffer,
                                       VkDeviceSize p_offset,
                                       bool p_u32 /*= false*/) {
  VkCommandBuffer cmb = frame_data[current_frame_idx].cmb;
  vkCmdBindIndexBuffer(cmb, p_buffer.buffer, p_offset,
                       p_u32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
}

void CuRenderDevice::draw(uint32_t p_vertex_count, uint32_t p_instance_count,
                          uint32_t p_first_vertex, uint32_t p_first_instance) {
  VkCommandBuffer cmb = frame_data[current_frame_idx].cmb;
  vkCmdDraw(cmb, p_vertex_count, p_instance_count, p_first_vertex,
            p_first_instance);
}

void CuRenderDevice::draw_indexed(uint32_t p_index_count,
                                  uint32_t p_instance_count,
                                  uint32_t p_first_index,
                                  int32_t p_vertex_offset,
                                  uint32_t p_first_instance) {
  VkCommandBuffer cmb = frame_data[current_frame_idx].cmb;
  vkCmdDrawIndexed(cmb, p_index_count, p_instance_count, p_first_index,
                   p_vertex_offset, p_first_instance);
}

void CuRenderDevice::submit_image(Texture &p_from,
                                  Texture *p_to /*= nullptr*/) {
  FrameData &current_frame = frame_data[current_frame_idx];

  VkCommandBuffer cmb = current_frame.cmb;

  vkCmdEndRenderingKHR(cmb);

  transition_image(cmb, p_from.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  transition_image(cmb, swapchain.images[swapchain_img_index],
                   VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkExtent3D destination_extent;
  if (p_to) {
    destination_extent = p_to->extent;
  } else {
    destination_extent = {swapchain.extent.width, swapchain.extent.height, 1};
  }

  copy_image_to_image(cmb, p_from.image,
                      p_to ? p_to->image
                           : swapchain.images[swapchain_img_index],
                      p_from.extent, destination_extent);

  transition_image(cmb, swapchain.images[swapchain_img_index],
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void CuRenderDevice::finish_recording() {
  FrameData &current_frame = frame_data[current_frame_idx];

  VkCommandBuffer cmb = current_frame.cmb;

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
  signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR;
  signal_info.deviceIndex = 0;
  signal_info.value = 1;

  VkSubmitInfo2KHR submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR;
  submit_info.pNext = nullptr;
  submit_info.waitSemaphoreInfoCount = 1;
  submit_info.pWaitSemaphoreInfos = &wait_info;
  submit_info.signalSemaphoreInfoCount = 1;
  submit_info.pSignalSemaphoreInfos = &signal_info;
  submit_info.commandBufferInfoCount = 1;
  submit_info.pCommandBufferInfos = &cmb_submit_info;

  VK_CHECK(vkQueueSubmit2KHR(graphics_queue, 1, &submit_info,
                             current_frame.render_fence));

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
  } else if (result != VK_SUCCESS) {
    ENGINE_ERROR("Failed to present graphics queue");
    return;
  }

  current_frame_idx = (current_frame_idx + 1) % FRAME_OVERLAP;
  frame_count++;
}

void CuRenderDevice::stop_rendering() { vkDeviceWaitIdle(device); }

void CuRenderDevice::clear() {
  for (int i = 0; i < FRAME_OVERLAP; ++i) {
    FrameData &current_frame = frame_data[i];
    current_frame.deletion_queue.execute();
    current_frame.descriptor_allocator.destroy_pools(device);
  }
  main_layout_allocator.clear();
  main_deletion_queue.execute();
}

void DescriptorWriter::write_buffer(int p_binding, Buffer &p_buffer,
                                    size_t p_offset, size_t p_stride,
                                    VkDescriptorType p_type) {
  VkDescriptorBufferInfo &info =
      buffer_infos.emplace_back(VkDescriptorBufferInfo{
          .buffer = p_buffer.buffer, .offset = p_offset, .range = p_stride});

  VkWriteDescriptorSet write = {.sType =
                                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

  write.dstBinding = p_binding;
  write.dstSet = VK_NULL_HANDLE; // left empty for now until we need to write it
  write.descriptorCount = 1;
  write.descriptorType = p_type;
  write.pBufferInfo = &info;

  writes.push_back(write);
}

void DescriptorWriter::update_set(VkDescriptorSet p_set) {
  CuRenderDevice *render_device = CuRenderDevice::get_singleton();
  if (p_set == VK_NULL_HANDLE) {
    ENGINE_WARN("VK_NULL_HANDLE set discovered. Can't write to it");
  }
  if (!render_device) {
    ENGINE_ERROR("Failed to update set");
    return;
  }
  for (VkWriteDescriptorSet &write : writes) {
    write.dstSet = p_set;
  }

  vkUpdateDescriptorSets(render_device->get_raw_device(),
                         (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void DescriptorWriter::clear() {
  image_infos.clear();
  writes.clear();
  buffer_infos.clear();
}