#pragma once

#define VK_NO_PROTOTYPES
#include <volk.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>
#include <vector>
#include <span>
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
    CuWindow* window = nullptr;
};



struct RenderPipeline {
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> sets = {};
    void clear(VkDevice p_device) {
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(p_device, pipeline, nullptr);
        }

        if (layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(p_device, layout, nullptr);
        }
        sets.clear();
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
    std::vector<VkPushConstantRange> push_constants = {};
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

enum ImageType {
    COLOR,
    DEPTH,
    STENCIL,
};

class LayoutAllocator {
public:
    const PipelineLayoutInfo generate_pipeline_info(VkDevice p_device, const std::vector<CompiledShaderInfo>& p_shader_infos);
    void clear();
private: 
    std::vector<VkDescriptorSetLayout> descriptor_layouts;
};

class CuRenderDevice {
public:
    CuRenderDevice();
    bool init(CuWindow* p_window);
    VkExtent2D get_swapchain_size() const {return swapchain.extent;}
    bool create_texture(VkFormat p_format, VkExtent3D p_extent, VkImageUsageFlags p_image_usage, VmaMemoryUsage p_memory_usage, Texture& p_out_texture);
    Buffer create_buffer(size_t p_size, VkBufferUsageFlags p_usage, VmaMemoryUsage p_memory_usage);
    void write_buffer(void* p_data, size_t p_size, Buffer& buffer);
    void clear_buffer(Buffer p_buffer);
    void clear_texture(Texture& p_texture);
    RenderPipeline create_render_pipeline(const std::vector<CompiledShaderInfo>& p_shader_infos, const Texture* p_texture = nullptr);
    void begin_recording();
    void prepare_image(Texture& p_texture, ImageType p_image_type);
    void bind_pipeline(const RenderPipeline& p_pipeline);
    void bind_push_constant(const RenderPipeline& p_pipeline, VkShaderStageFlags p_shaderStages, uint32_t p_offset, uint32_t p_size, void* p_data);
    void draw();
    void submit_image(Texture& p_from, Texture* p_to = nullptr);
    void finish_recording();
    void stop_rendering();
    void clear();

    VkDevice get_raw_device() {return device;}

    static CuRenderDevice* get_singleton();
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

    ExecutionQueuer main_deletion_queue = ExecutionQueuer(true);

    LayoutAllocator main_layout_allocator;

    int current_frame_idx = 0;
    int frame_count = 0;

    uint32_t swapchain_img_index = 0;

    static CuRenderDevice* singleton;
};

struct DescriptorWriter {
    std::deque<VkDescriptorImageInfo> image_infos;
    std::deque<VkDescriptorBufferInfo> buffer_infos;
    std::vector<VkWriteDescriptorSet> writes;

    void write_image(int p_binding,VkImageView p_image,VkSampler p_sampler , VkImageLayout p_layout, VkDescriptorType p_type);
    void write_buffer(int p_binding, Buffer& p_buffer, size_t p_offset, size_t p_stride,VkDescriptorType p_type); 

    void clear();
    void update_set(VkDescriptorSet p_set);
};