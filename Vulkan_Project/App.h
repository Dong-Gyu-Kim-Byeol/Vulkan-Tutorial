#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

//#define STB_IMAGE_IMPLEMENTATION	// App.cpp
#include <stb_image.h>


#include <iostream>
#include <optional>
#include <vector>
#include <set>
#include <array>
#include <unordered_map>
#include <thread>
#include <mutex>

#include <cassert>

#include "GameData.h"
#include "Error.h"
#include "ReadFile.h"


struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;
};

struct MSAA
{
	VkSampleCountFlagBits Samples;
	VkImage ColorImage;
	VkDeviceMemory ColorImageMemory;
	VkImageView ColorImageView;
};
struct Swapchain
{
	VkSwapchainKHR Swapchain;
	VkFormat ImageFormat;
	VkExtent2D Extent;

	std::vector<VkImage> Images;
	std::vector<VkImageView> ImageViews;
	VkImage DepthImage;
	VkDeviceMemory DepthImageMemory;
	VkImageView DepthImageView;

	MSAA MSAA;

	std::vector<VkFramebuffer> Framebuffers;
};

struct Graphics
{
	VkPipeline Pipeline;
	VkPipelineLayout PipelineLayout;
	VkPipeline CubePipeline;
	VkPipelineLayout CubePipelineLayout;

	VkRenderPass RenderPass;
	VkQueue Queue;
	uint32_t QueueFamily;
	VkQueue PresentQueue;
	uint32_t PresentQueueFamily;

	VkDescriptorSetLayout DescriptorSetLayout;
	VkDescriptorPool DescriptorPool;
	std::vector<VkDescriptorSet> DescriptorSets;

	VkCommandPool CommandPool;
	std::vector<VkCommandBuffer> CommandBuffers;
	std::vector<VkSemaphore> FinishedSemaphores;
};

struct Compute
{
	VkPipeline Pipeline;
	VkPipelineLayout PipelineLayout;
	VkQueue Queue;
	uint32_t QueueFamily;

	VkDescriptorSetLayout DescriptorSetLayout;
	VkDescriptorPool DescriptorPool;
	std::vector<VkDescriptorSet> DescriptorSets;

	VkCommandPool CommandPool;
	std::vector<VkCommandBuffer> CommandBuffers;
	std::vector<VkSemaphore> FinishedSemaphores;
};

struct Texture
{
	uint32_t MipLevels;
	VkSampler Sampler;
	VkImage Image;
	VkDeviceMemory ImageMemory;
	VkImageView ImageView;
};

struct Image
{
	VkImage Image;
	VkDeviceMemory Memory;
	VkImageView View;
};

struct Buffer
{
	VkBuffer Buffer;
	VkDeviceMemory Memory;
};

struct VisibleBuffer
{
	VkBuffer Buffer;
	VkDeviceMemory Memory;
	VkDeviceSize BufferSize;
	std::vector<size_t> TypeOffsets;
	void* pMapMemory;
};



class QueueFamilyIndices
{
public:
	std::optional<uint32_t> GraphicsFamily;
	std::optional<uint32_t> PresentFamily;
	std::optional<uint32_t> ComputeFamily;

	bool IsComplete();
};



class VkApp final
{
public:
	GLFWwindow* GetWindow();
	void Run();

	VkApp();
	VkApp(const VkApp& _rOther) = delete;
	VkApp& operator=(const VkApp& _rOther) = delete;
	~VkApp() = default;

private:
	GameData mGameData;

	GLFWwindow* mWindow;
	bool mbInitedVulkan;

	VkInstance mVkInstance;
	VkDebugUtilsMessengerEXT mDebugMessenger;
	VkSurfaceKHR mSurface;

	VkPhysicalDevice mPhysicalDevice;
	VkDevice mDevice;

	Swapchain mSwapchain;
	Graphics mGraphics;
	Compute mCompute;

	Texture mTexture;
	std::vector<Image> mStorageImages;

	Buffer mVertexBuffer;
	Buffer mIndexBuffer;
	std::vector<VisibleBuffer> mInstancesVisibleBuffers;
	std::vector<VisibleBuffer> mCubeInstancesVisibleBuffers;

	std::vector<VisibleBuffer> mUniformVisibleBuffers;

	std::vector<PushConstantCmdObject> mPushConstantCmdData;

	std::vector<VkSemaphore> mImageAvailableSemaphores;
	std::vector<VkFence> mInFlightFences;
	std::vector<VkFence> mImagesInFlight;
	size_t mCurrentFrame;
	bool mbResizedFramebuffer;
	std::mutex mMutexResizedFramebuffer;

private:
	void initWindow();

	void initVulkan();
	void cleanup();

	void mainLoop();

	// Instance
	void createInstance();
	std::vector<const char*> getRequiredExtensions();

	// Surface
	void createSurface();

	// PhysicalDevice
	void pickPhysicalDevice();
	int rateDeviceSuitability(const VkPhysicalDevice _device);
	bool isDeviceSuitable(const VkPhysicalDevice _device);
	QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice _device);
	bool checkDeviceExtensionSupport(const VkPhysicalDevice _physicalDevice);
	SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice _physicalDevice);

	// LogicalDevice
	void createLogicalDevice();

	// SwapChain
	void createSwapChain();
	void recreateSwapChain();
	void cleanupSwapChain();
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _rAvailableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _rAvailablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _rCapabilities);
	static void framebufferResizeCallback(GLFWwindow* _pWindow, int _width, int _height);
	void createSwapChainImageViews();

	// DepthResources
	void createDepthResources();
	VkFormat findDepthFormat();
	bool hasStencilComponent(const VkFormat _format);

	// MSAA
	VkSampleCountFlagBits getMaxUsableSampleCount();
	void createColorResources();

	// GraphicsPipeline
	void createGraphicsPipeline();
	void createGraphicsCubePipeline();
	VkShaderModule createShaderModule(const std::vector<char>& _rCode);
	void createRenderPass();
	void createFramebuffers();

	// ComputePipeline
	void createComputePipeline();

	// GraphicsDescriptorSet
	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSets();

	// ComputeDescriptorSet
	void createComputeDescriptorSetLayout();
	void createComputeDescriptorPool();
	void createComputeDescriptorSets();

	// Image
	void createImage(const uint32_t _width, const uint32_t _height, const uint32_t _mipLevels, 
		const VkSampleCountFlagBits _numSamples, const VkFormat _format, const VkImageTiling _tiling, 
		const VkImageUsageFlags _usage, const VkMemoryPropertyFlags _properties,
		VkImage& _out_rImage, VkDeviceMemory& _out_rImageMemory);
	void transitionImageLayout(const VkImage _image, const VkFormat _format, 
		const VkImageLayout _oldLayout, const VkImageLayout _newLayout, const uint32_t _mipLevels);
	void copyBufferToImage(const VkBuffer _buffer, const VkImage _image, 
		const uint32_t _width, const uint32_t _height);
	VkImageView createImageView(const VkImage _image, const VkFormat _format, 
		const VkImageAspectFlags _aspectFlags, const uint32_t _mipLevels);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& _rCandidates, 
		const VkImageTiling _tiling, const VkFormatFeatureFlags _features);

	// TextureImage
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void generateMipmaps(const VkImage _image, const VkFormat _imageFormat, const int32_t _texWidth, 
		const int32_t _texHeight, const uint32_t _mipLevels);

	// StorageImage
	void createStorageImages();

	// VertexBuffer, IndexBuffer, InstanceBuffer, UniformBuffer
	void createVertexBuffer();
	void createIndexBuffer();
	void createInstanceBuffers();
	void createCubeInstanceBuffers();
	void createUniformBuffers();
	uint32_t findMemoryType(const uint32_t _typeFilter, const VkMemoryPropertyFlags _properties);
	void createBuffer(const VkDeviceSize _size, const VkBufferUsageFlags _usage, 
		const VkMemoryPropertyFlags _properties, VkBuffer& _out_rBuffer, VkDeviceMemory& _out_rBufferMemory);
	void copyBuffer(const VkBuffer _srcBuffer, const VkBuffer _dstBuffer, const VkDeviceSize _size);

	// GraphicsCommand
	void createCommandPool();
	void createCommandBuffers();
	void recordCommandBuffer(const size_t _index);
	void cmdDrawIndexed(const VkCommandBuffer _commandBuffer,
		const VkBuffer _vertexBuffer, const VkDeviceSize _vertexOffset,
		const VkBuffer _indexBuffer, const VkDeviceSize _indexOffset, const uint32_t _indexCount,
		const VkBuffer _instanceBuffer, const VkDeviceSize _instanceOffset, 
		uint32_t& _out_rFirstInstance, const uint32_t _instanceCount);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(const VkCommandBuffer _commandBuffer);

	// ComputeCommand
	void createComputeCommandPool();
	void createComputeCommandBuffers();
	void recordComputeCommandBuffer(const size_t _index);
	void cmdCopyColorImage(const VkCommandBuffer _cmdBuffer,
		const VkImage _srcImage, const VkImageLayout _srcImageLayout, 
		const VkImage _dstImage, const VkImageLayout _dstImageLayout, 
		const uint32_t _width, const uint32_t _height);
	void cmdImageBarrier(const VkCommandBuffer _commandBuffer, const VkImage _image,
		const VkPipelineStageFlags _srcStage, const VkPipelineStageFlags _dstStage,
		const VkAccessFlags _srcAccess, const VkAccessFlags _dstAccess,
		const VkImageLayout _oldLayout, const VkImageLayout _newLayout,
		const uint32_t _oldQueueFamily, const uint32_t _newQueueFamily);

	// DrawFrame
	void drawFrame();
	void createSyncObjects();
	void updateInstanceBuffer(const uint32_t _imageIndex);
	void updateCubeInstanceBuffer(const uint32_t _imageIndex);
	void updateUniformBuffer(const uint32_t _imageIndex);

	
	
	

#ifdef CUBE
	void createCubeVertexBuffer();
	void createCubeIndexBuffer();
	void CubeCleanup();
#endif // CUBE



#ifdef _DEBUG
	bool checkValidationLayerSupport();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _out_rCreateInfo);
	void setupDebugMessenger();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT _messageSeverity, VkDebugUtilsMessageTypeFlagsEXT _messageType, const VkDebugUtilsMessengerCallbackDataEXT* _pCallbackData, void* _pUserData);
#endif
};

VkResult CreateDebugUtilsMessengerEXT(const VkInstance _instance, const VkDebugUtilsMessengerCreateInfoEXT* _pCreateInfo, const VkAllocationCallbacks* _pAllocator, VkDebugUtilsMessengerEXT* _pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(const VkInstance _instance, const VkDebugUtilsMessengerEXT _debugMessenger, const VkAllocationCallbacks* _pAllocator);