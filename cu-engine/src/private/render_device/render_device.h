#pragma once

#define VK_NO_PROTOTYPES
#include <volk.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>
#include <vector>
#include "utils.h"
#include "shader_compiler.h"
#include <spirv_reflect.h>

class CuWindow;

class Swapchain {
public:
    Swapchain(){};
    Swapchain(VkSurfaceKHR p_surface, VkDevice p_device, VkPhysicalDevice p_physical_device, CuWindow* p_window) {
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

struct DescriptorAllocator {
public:
	struct PoolSizeRatio {
		VkDescriptorType type;
		float ratio;
	};

	void init(VkDevice p_device, uint32_t p_initial_sets, std::span<PoolSizeRatio> p_pool_ratios);
	void clear_pools(VkDevice p_device);
	void destroy_pools(VkDevice p_device);

    VkDescriptorSet allocate(VkDevice p_device, VkDescriptorSetLayout p_layout, uint32_t p_descriptor_set_count = 1,void* pNext = nullptr);
private:
	VkDescriptorPool get_pool(VkDevice p_device);
	VkDescriptorPool create_pool(VkDevice p_device, uint32_t p_set_count, std::span<PoolSizeRatio> p_pool_ratios);

	std::vector<PoolSizeRatio> ratios;
	std::vector<VkDescriptorPool> full_pools;
	std::vector<VkDescriptorPool> ready_pools;
	uint32_t sets_per_pool;

};

struct PipelineLayoutInfo {
    std::vector<VkDescriptorSetLayout> descriptor_layouts = {};
    std::vector<VkDescriptorSet> descriptor_sets = {};
};

struct FrameData {
    VkCommandPool cmp;
    VkCommandBuffer cmb;
    VkSemaphore swapchain_semaphore, render_semaphore;
    VkFence render_fence;
    ExecutionQueuer deletion_queue = ExecutionQueuer(true);
    DescriptorAllocator descriptor_allocator;
};
const int FRAME_OVERLAP = 2;

class CuRenderDevice {
public:
    bool init(CuWindow* p_window);
    const PipelineLayoutInfo generate_pipeline_info(const std::vector<CompiledShaderInfo>& p_shader_infos);
    void create_render_pipeline(const std::vector<CompiledShaderInfo>& p_shader_infos, const PipelineLayoutInfo& p_layout_info);
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
    CuWindow* window = nullptr;

    Swapchain swapchain;
    FrameData frame_data[FRAME_OVERLAP];

    Texture draw_texture;

    ExecutionQueuer main_deletion_queue = ExecutionQueuer(true);

    //Refactor this into a hash map later
    RenderPipeline render_pipeline;

    int current_frame_idx = 0;
    int frame_count = 0;
};