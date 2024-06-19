#pragma once

#define VK_NO_PROTOTYPES
#include <volk.h>

#include "logger.h"
#include <deque>
#include <functional>
#include <vulkan/vk_enum_string_helper.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

class ExecutionQueuer {
public:
  ExecutionQueuer(bool p_reverse) { reverse = p_reverse; }

  void push_function(std::function<void()> &&function) {
    queue.push_back(function);
  }

  void execute() {
    if (reverse) {
      for (auto it = queue.rbegin(); it != queue.rend(); it++) {
        (*it)(); // call functors
      }
    } else {
      for (auto it = queue.begin(); it != queue.end(); it++) {
        (*it)(); // call functors
      }
    }
  }

private:
  bool reverse = false;
  std::deque<std::function<void()>> queue;
};

struct Texture {
  VkImage image;
  VkImageView view;
  VmaAllocation allocation;
  VkExtent3D extent;
  VkFormat format;
};

struct Buffer {
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation allocation;
  VmaAllocationInfo info;
};

void transition_image(VkCommandBuffer command_buffer, VkImage image,
                      VkImageLayout old_layout, VkImageLayout new_layout);
void copy_image_to_image(VkCommandBuffer command_buffer, VkImage source,
                         VkImage destination, VkExtent3D src_size,
                         VkExtent3D dst_size);

VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspect_mask);

VkImageCreateInfo image_create_info(VkFormat format,
                                    VkImageUsageFlags usage_flags,
                                    VkExtent3D extent);

VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image,
                                            VkImageAspectFlags aspect_flags);

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      ENGINE_ERROR("Detected Vulkan error: {}", string_VkResult(err));         \
      abort();                                                                 \
    }                                                                          \
  } while (0)

static uint32_t get_format_size(VkFormat p_format) {
  uint32_t result = 0;
  switch (p_format) {
  case VK_FORMAT_UNDEFINED:
    result = 0;
    break;
  case VK_FORMAT_R4G4_UNORM_PACK8:
    result = 1;
    break;
  case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
    result = 2;
    break;
  case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
    result = 2;
    break;
  case VK_FORMAT_R5G6B5_UNORM_PACK16:
    result = 2;
    break;
  case VK_FORMAT_B5G6R5_UNORM_PACK16:
    result = 2;
    break;
  case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
    result = 2;
    break;
  case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
    result = 2;
    break;
  case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
    result = 2;
    break;
  case VK_FORMAT_R8_UNORM:
    result = 1;
    break;
  case VK_FORMAT_R8_SNORM:
    result = 1;
    break;
  case VK_FORMAT_R8_USCALED:
    result = 1;
    break;
  case VK_FORMAT_R8_SSCALED:
    result = 1;
    break;
  case VK_FORMAT_R8_UINT:
    result = 1;
    break;
  case VK_FORMAT_R8_SINT:
    result = 1;
    break;
  case VK_FORMAT_R8_SRGB:
    result = 1;
    break;
  case VK_FORMAT_R8G8_UNORM:
    result = 2;
    break;
  case VK_FORMAT_R8G8_SNORM:
    result = 2;
    break;
  case VK_FORMAT_R8G8_USCALED:
    result = 2;
    break;
  case VK_FORMAT_R8G8_SSCALED:
    result = 2;
    break;
  case VK_FORMAT_R8G8_UINT:
    result = 2;
    break;
  case VK_FORMAT_R8G8_SINT:
    result = 2;
    break;
  case VK_FORMAT_R8G8_SRGB:
    result = 2;
    break;
  case VK_FORMAT_R8G8B8_UNORM:
    result = 3;
    break;
  case VK_FORMAT_R8G8B8_SNORM:
    result = 3;
    break;
  case VK_FORMAT_R8G8B8_USCALED:
    result = 3;
    break;
  case VK_FORMAT_R8G8B8_SSCALED:
    result = 3;
    break;
  case VK_FORMAT_R8G8B8_UINT:
    result = 3;
    break;
  case VK_FORMAT_R8G8B8_SINT:
    result = 3;
    break;
  case VK_FORMAT_R8G8B8_SRGB:
    result = 3;
    break;
  case VK_FORMAT_B8G8R8_UNORM:
    result = 3;
    break;
  case VK_FORMAT_B8G8R8_SNORM:
    result = 3;
    break;
  case VK_FORMAT_B8G8R8_USCALED:
    result = 3;
    break;
  case VK_FORMAT_B8G8R8_SSCALED:
    result = 3;
    break;
  case VK_FORMAT_B8G8R8_UINT:
    result = 3;
    break;
  case VK_FORMAT_B8G8R8_SINT:
    result = 3;
    break;
  case VK_FORMAT_B8G8R8_SRGB:
    result = 3;
    break;
  case VK_FORMAT_R8G8B8A8_UNORM:
    result = 4;
    break;
  case VK_FORMAT_R8G8B8A8_SNORM:
    result = 4;
    break;
  case VK_FORMAT_R8G8B8A8_USCALED:
    result = 4;
    break;
  case VK_FORMAT_R8G8B8A8_SSCALED:
    result = 4;
    break;
  case VK_FORMAT_R8G8B8A8_UINT:
    result = 4;
    break;
  case VK_FORMAT_R8G8B8A8_SINT:
    result = 4;
    break;
  case VK_FORMAT_R8G8B8A8_SRGB:
    result = 4;
    break;
  case VK_FORMAT_B8G8R8A8_UNORM:
    result = 4;
    break;
  case VK_FORMAT_B8G8R8A8_SNORM:
    result = 4;
    break;
  case VK_FORMAT_B8G8R8A8_USCALED:
    result = 4;
    break;
  case VK_FORMAT_B8G8R8A8_SSCALED:
    result = 4;
    break;
  case VK_FORMAT_B8G8R8A8_UINT:
    result = 4;
    break;
  case VK_FORMAT_B8G8R8A8_SINT:
    result = 4;
    break;
  case VK_FORMAT_B8G8R8A8_SRGB:
    result = 4;
    break;
  case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A8B8G8R8_UINT_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A8B8G8R8_SINT_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A2R10G10B10_UINT_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A2R10G10B10_SINT_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A2B10G10R10_UINT_PACK32:
    result = 4;
    break;
  case VK_FORMAT_A2B10G10R10_SINT_PACK32:
    result = 4;
    break;
  case VK_FORMAT_R16_UNORM:
    result = 2;
    break;
  case VK_FORMAT_R16_SNORM:
    result = 2;
    break;
  case VK_FORMAT_R16_USCALED:
    result = 2;
    break;
  case VK_FORMAT_R16_SSCALED:
    result = 2;
    break;
  case VK_FORMAT_R16_UINT:
    result = 2;
    break;
  case VK_FORMAT_R16_SINT:
    result = 2;
    break;
  case VK_FORMAT_R16_SFLOAT:
    result = 2;
    break;
  case VK_FORMAT_R16G16_UNORM:
    result = 4;
    break;
  case VK_FORMAT_R16G16_SNORM:
    result = 4;
    break;
  case VK_FORMAT_R16G16_USCALED:
    result = 4;
    break;
  case VK_FORMAT_R16G16_SSCALED:
    result = 4;
    break;
  case VK_FORMAT_R16G16_UINT:
    result = 4;
    break;
  case VK_FORMAT_R16G16_SINT:
    result = 4;
    break;
  case VK_FORMAT_R16G16_SFLOAT:
    result = 4;
    break;
  case VK_FORMAT_R16G16B16_UNORM:
    result = 6;
    break;
  case VK_FORMAT_R16G16B16_SNORM:
    result = 6;
    break;
  case VK_FORMAT_R16G16B16_USCALED:
    result = 6;
    break;
  case VK_FORMAT_R16G16B16_SSCALED:
    result = 6;
    break;
  case VK_FORMAT_R16G16B16_UINT:
    result = 6;
    break;
  case VK_FORMAT_R16G16B16_SINT:
    result = 6;
    break;
  case VK_FORMAT_R16G16B16_SFLOAT:
    result = 6;
    break;
  case VK_FORMAT_R16G16B16A16_UNORM:
    result = 8;
    break;
  case VK_FORMAT_R16G16B16A16_SNORM:
    result = 8;
    break;
  case VK_FORMAT_R16G16B16A16_USCALED:
    result = 8;
    break;
  case VK_FORMAT_R16G16B16A16_SSCALED:
    result = 8;
    break;
  case VK_FORMAT_R16G16B16A16_UINT:
    result = 8;
    break;
  case VK_FORMAT_R16G16B16A16_SINT:
    result = 8;
    break;
  case VK_FORMAT_R16G16B16A16_SFLOAT:
    result = 8;
    break;
  case VK_FORMAT_R32_UINT:
    result = 4;
    break;
  case VK_FORMAT_R32_SINT:
    result = 4;
    break;
  case VK_FORMAT_R32_SFLOAT:
    result = 4;
    break;
  case VK_FORMAT_R32G32_UINT:
    result = 8;
    break;
  case VK_FORMAT_R32G32_SINT:
    result = 8;
    break;
  case VK_FORMAT_R32G32_SFLOAT:
    result = 8;
    break;
  case VK_FORMAT_R32G32B32_UINT:
    result = 12;
    break;
  case VK_FORMAT_R32G32B32_SINT:
    result = 12;
    break;
  case VK_FORMAT_R32G32B32_SFLOAT:
    result = 12;
    break;
  case VK_FORMAT_R32G32B32A32_UINT:
    result = 16;
    break;
  case VK_FORMAT_R32G32B32A32_SINT:
    result = 16;
    break;
  case VK_FORMAT_R32G32B32A32_SFLOAT:
    result = 16;
    break;
  case VK_FORMAT_R64_UINT:
    result = 8;
    break;
  case VK_FORMAT_R64_SINT:
    result = 8;
    break;
  case VK_FORMAT_R64_SFLOAT:
    result = 8;
    break;
  case VK_FORMAT_R64G64_UINT:
    result = 16;
    break;
  case VK_FORMAT_R64G64_SINT:
    result = 16;
    break;
  case VK_FORMAT_R64G64_SFLOAT:
    result = 16;
    break;
  case VK_FORMAT_R64G64B64_UINT:
    result = 24;
    break;
  case VK_FORMAT_R64G64B64_SINT:
    result = 24;
    break;
  case VK_FORMAT_R64G64B64_SFLOAT:
    result = 24;
    break;
  case VK_FORMAT_R64G64B64A64_UINT:
    result = 32;
    break;
  case VK_FORMAT_R64G64B64A64_SINT:
    result = 32;
    break;
  case VK_FORMAT_R64G64B64A64_SFLOAT:
    result = 32;
    break;
  case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
    result = 4;
    break;
  case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    result = 4;
    break;

  default:
    break;
  }
  return result;
}
