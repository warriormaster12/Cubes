#pragma once

#define VK_NO_PROTOTYPES
#include <volk.h>

#include <functional>
#include <deque>
#include "logger.h"
#include <vulkan/vk_enum_string_helper.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

class ExecutionQueuer {
public:
    ExecutionQueuer(bool p_reverse) {
        reverse = p_reverse;
    }

    void push_function(std::function<void()>&& function) {
        queue.push_back(function);
    }

    void execute() {
        if (reverse) {
            for (auto it = queue.rbegin(); it != queue.rend(); it++) {
                (*it)(); //call functors
            }
        } else {
            for (auto it = queue.begin(); it != queue.end(); it++) {
                (*it)(); //call functors
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

void transition_image(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);
void copy_image_to_image(VkCommandBuffer command_buffer, VkImage source, VkImage destination, VkExtent2D src_size, VkExtent2D dst_size);


VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspect_mask);

VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usage_flags, VkExtent3D extent);

VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspect_flags);


#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            ENGINE_ERROR("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)
