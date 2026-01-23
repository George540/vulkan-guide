// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

// --- other includes ---
#include <vk_types.h>
#include "vk_descriptors.h"

//bootstrap library
#include "VkBootstrap.h"

// Inneficient at scale, but good for this example.
struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// Reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
		{
			(*it)(); //call functors
		}

		deletors.clear();
	}
};

struct FrameData {

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;
	VkSemaphore _swapchainSemaphore, _renderSemaphore;
	VkFence _renderFence;
	DeletionQueue _deletionQueue;
};

constexpr unsigned int FRAME_OVERLAP = 2; // for double-buffering
constexpr unsigned int WAIT_FENCE_TIMEOUT = 1000000000; // for double-buffering

class VulkanEngine
{
	// Vulkan Initializers
	VkInstance _instance;// Vulkan library handle
	VkDebugUtilsMessengerEXT _debug_messenger;// Vulkan debug output handle
	VkPhysicalDevice _chosenGPU;// GPU chosen as the default device
	VkDevice _device; // Vulkan device for commands
	VkSurfaceKHR _surface;// Vulkan window surface
	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;

	// Vulkan Images
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkExtent2D _swapchainExtent;

	// Pipelines
	VkPipeline _gradientPipeline;
	VkPipelineLayout _gradientPipelineLayout;

	// Frame data
	FrameData _frames[FRAME_OVERLAP];

	// VMA
	VmaAllocator _allocator;

	// Immediate submit structures
	VkFence _immFence;
	VkCommandBuffer _immCommandBuffer;
	VkCommandPool _immCommandPool;

	//draw resources
	AllocatedImage _drawImage;
	VkExtent2D _drawExtent;

	// Descriptors
	DescriptorAllocator globalDescriptorAllocator;

	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorSetLayout _drawImageDescriptorLayout;

	// Queues
	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
	DeletionQueue _mainDeletionQueue;

	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();

public:

	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

	//initializes everything in the engine
	void init();

	void init_descriptors();

	void init_pipelines();

	void init_background_pipelines();

	void init_imgui();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();
	void draw_background(VkCommandBuffer cmd);
	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);

	//run main loop
	void run();
};
