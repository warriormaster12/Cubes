#pragma once

#define VK_NO_PROTOTYPES
#include <volk.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>
#include <vector>
#include "utils.h"
#include <shader_compiler.h>

class CuWindow;

struct FrameData {
    VkCommandPool cmp;
    VkCommandBuffer cmb;
    VkSemaphore swapchain_semaphore, render_semaphore;
    VkFence render_fence;
    ExecutionQueuer deletion_queue = ExecutionQueuer(true);
};
const int FRAME_OVERLAP = 2;

class Swapchain {
public:
    Swapchain(){};
    Swapchain(VkSurfaceKHR p_surface, VkDevice p_device, VkPhysicalDevice p_physical_device, CuWindow *p_window) {
        surface = p_surface;
        device = p_device;
        physical_device = p_physical_device;
        window = p_window;
    }
    void build(VkPresentModeKHR p_present_mode = VK_PRESENT_MODE_FIFO_KHR);
    void clear();
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
    VkFormat format;
    VkExtent2D extent;
private:
    VkSurfaceKHR surface;
    VkDevice device; 
    VkPhysicalDevice physical_device;
    CuWindow *window = nullptr;
};

struct RenderPipeline {
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    void clear(VkDevice& device) {
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline, nullptr);
        }

        if (layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, layout, nullptr);
        }
    }
};

class CuRenderDevice {
public:
    bool init(CuWindow *p_window);
    void create_render_pipeline(const std::vector<CompiledShaderInfo> p_shader_infos);
    void draw();
    void clear();
private:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue graphics_queue;
    uint32_t graphics_queue_family;
    VkSurfaceKHR surface;
    VmaAllocator allocator;
    CuWindow *window = nullptr;

    Swapchain swapchain;
    FrameData frame_data[FRAME_OVERLAP];

    Texture draw_texture;

    ExecutionQueuer main_deletion_queue = ExecutionQueuer(true);

    //Refactor this into a hash map later
    RenderPipeline render_pipeline;

    int current_frame_idx = 0;
    int frame_count = 0;
};