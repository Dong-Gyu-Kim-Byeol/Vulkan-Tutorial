#define STB_IMAGE_IMPLEMENTATION

#include "App.h"

int g_Width = 800;
int g_Height = 600;

int g_ViewportWidth = 800;
int g_ViewportHeight = 800;

const int WIDTH = 800;
const int HEIGHT = 600;

const std::string SHADER_PATH = "./shaders/";
const std::string COMPUTE_SHADER_PATH = "./shaders/comp.spv";
const std::string TEXTURE_PATH = "./textures/chalet.jpg";

constexpr uint32_t COMPUTE_BINDING = 0;

constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

const std::vector<const char*> DEVICE_EXTENSION = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


#ifdef CUBE
VkBuffer CubeVertexBuffer;
VkDeviceMemory CubeVertexBufferMemory;

VkBuffer CubeIndexBuffer;
VkDeviceMemory CubeIndexBufferMemory;

void VkApp::createCubeVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(mGameData.CubeVertices[0]) * mGameData.CubeVertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mGameData.CubeVertices.data(), (size_t)bufferSize);
	vkUnmapMemory(mDevice, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, CubeVertexBuffer, CubeVertexBufferMemory);

	copyBuffer(stagingBuffer, CubeVertexBuffer, bufferSize);


	vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
	vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
}

void VkApp::createCubeIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(mGameData.CubeIndices[0]) * mGameData.CubeIndices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mGameData.CubeIndices.data(), (size_t)bufferSize);
	vkUnmapMemory(mDevice, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, CubeIndexBuffer, CubeIndexBufferMemory);

	copyBuffer(stagingBuffer, CubeIndexBuffer, bufferSize);

	vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
	vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
}

void VkApp::CubeCleanup()
{
	vkDestroyBuffer(mDevice, CubeVertexBuffer, nullptr);
	vkFreeMemory(mDevice, CubeVertexBufferMemory, nullptr);

	vkDestroyBuffer(mDevice, CubeIndexBuffer, nullptr);
	vkFreeMemory(mDevice, CubeIndexBufferMemory, nullptr);
}

#endif // CUBE


bool QueueFamilyIndices::IsComplete()
{
	return GraphicsFamily.has_value() && PresentFamily.has_value() && ComputeFamily.has_value();
}


// VkApp -----------------------------------------------------------------------

// public
VkApp::VkApp() : mbInitedVulkan(false), mCurrentFrame(0), mbResizedFramebuffer(false)
{
	mSwapchain.MSAA.Samples = VK_SAMPLE_COUNT_1_BIT;

	initWindow();
}

void VkApp::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mWindow = glfwCreateWindow(g_Width, g_Height, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(mWindow, this);
	glfwSetFramebufferSizeCallback(mWindow, framebufferResizeCallback);
}

GLFWwindow* VkApp::GetWindow()
{
	return mWindow;
}

void VkApp::Run()
{
	initVulkan();
	mainLoop();
	cleanup();
}

void VkApp::initVulkan()
{
	createInstance();
#ifdef _DEBUG
	setupDebugMessenger();
#endif
	createSurface();

	pickPhysicalDevice();
	createLogicalDevice();

	createSwapChain();
	createSwapChainImageViews();

	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createGraphicsCubePipeline();
	createComputeDescriptorSetLayout();
	createComputePipeline();

	createCommandPool();
	createComputeCommandPool();

	createStorageImages();

	createColorResources();
	createDepthResources();
	createFramebuffers();

	createTextureImage();
	createTextureImageView();
	createTextureSampler();

	mGameData.InitData(static_cast<float>(mSwapchain.Extent.width), static_cast<float>(mSwapchain.Extent.height));

	createVertexBuffer();
	createIndexBuffer();
#ifdef CUBE
	createCubeVertexBuffer();
	createCubeIndexBuffer();
#endif // CUBE

	createInstanceBuffers();
	createCubeInstanceBuffers();
	createUniformBuffers();

	createDescriptorPool();
	createDescriptorSets();
	createComputeDescriptorPool();
	createComputeDescriptorSets();

	createCommandBuffers();
	createComputeCommandBuffers();

	createSyncObjects();

	mbInitedVulkan = true;
	if (mbResizedFramebuffer == true) { recreateSwapChain(); }
	mGameData.InitTime();
}

void VkApp::cleanup()
{
#ifdef CUBE
	CubeCleanup();
#endif // CUBE

	cleanupSwapChain();

	vkDestroySampler(mDevice, mTexture.Sampler, nullptr);
	vkDestroyImageView(mDevice, mTexture.ImageView, nullptr);

	vkDestroyImage(mDevice, mTexture.Image, nullptr);
	vkFreeMemory(mDevice, mTexture.ImageMemory, nullptr);

	vkDestroyDescriptorSetLayout(mDevice, mGraphics.DescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(mDevice, mCompute.DescriptorSetLayout, nullptr);

	for (size_t i = 0; i < mInstancesVisibleBuffers.size(); i++)
	{
		vkDestroyBuffer(mDevice, mInstancesVisibleBuffers[i].Buffer, nullptr);
		vkFreeMemory(mDevice, mInstancesVisibleBuffers[i].Memory, nullptr);
	}
	for (size_t i = 0; i < mInstancesVisibleBuffers.size(); i++)
	{
		vkDestroyBuffer(mDevice, mCubeInstancesVisibleBuffers[i].Buffer, nullptr);
		vkFreeMemory(mDevice, mCubeInstancesVisibleBuffers[i].Memory, nullptr);
	}

	for (size_t i = 0; i < mUniformVisibleBuffers.size(); i++)
	{
		vkDestroyBuffer(mDevice, mUniformVisibleBuffers[i].Buffer, nullptr);
		vkFreeMemory(mDevice, mUniformVisibleBuffers[i].Memory, nullptr);
	}

	vkDestroyBuffer(mDevice, mIndexBuffer.Buffer, nullptr);
	vkFreeMemory(mDevice, mIndexBuffer.Memory, nullptr);

	vkDestroyBuffer(mDevice, mVertexBuffer.Buffer, nullptr);
	vkFreeMemory(mDevice, mVertexBuffer.Memory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(mDevice, mGraphics.FinishedSemaphores[i], nullptr);
		vkDestroySemaphore(mDevice, mCompute.FinishedSemaphores[i], nullptr);
		vkDestroySemaphore(mDevice, mImageAvailableSemaphores[i], nullptr);

		vkDestroyFence(mDevice, mInFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(mDevice, mGraphics.CommandPool, nullptr);
	vkDestroyCommandPool(mDevice, mCompute.CommandPool, nullptr);

	vkDestroyDevice(mDevice, nullptr);

#ifdef _DEBUG
	DestroyDebugUtilsMessengerEXT(mVkInstance, mDebugMessenger, nullptr);
#endif

	vkDestroySurfaceKHR(mVkInstance, mSurface, nullptr);
	vkDestroyInstance(mVkInstance, nullptr);

	glfwDestroyWindow(mWindow);
	glfwTerminate();
}

void VkApp::mainLoop()
{
	while (!glfwWindowShouldClose(mWindow))
	{
		//std::thread update([&]()
		//	{
				mGameData.Update();
		//	});
		drawFrame();

		//update.join();
	}

	vkDeviceWaitIdle(mDevice);
}


// private

// Instance
void VkApp::createInstance()
{
#ifdef _DEBUG
	if (checkValidationLayerSupport() == false)
	{
		Error("App", "createInstance", "validation layers requested, but not available!");
	}
#endif

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef NDEBUG
	createInfo.enabledLayerCount = 0;

	createInfo.pNext = nullptr;
#else
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
	createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

	populateDebugMessengerCreateInfo(debugCreateInfo);
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#endif

	if (vkCreateInstance(&createInfo, nullptr, &mVkInstance) != VK_SUCCESS)
	{
		Error("App", "createInstance", "failed to create instance!");
	}

	/*
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	VkExtensionProperties* extensions = new VkExtensionProperties[extensionCount];
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);

	std::cout << "available extensions:" << std::endl;
	for (uint32_t i = 0; i < extensionCount; i++)
	{
		std::cout << "\t" << extensions[i].extensionName << std::endl;
	}

	delete[] VkExtensionProperties;
	//*/
}

std::vector<const char*> VkApp::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef _DEBUG
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	return extensions;
}

// Surface
void VkApp::createSurface()
{
	if (glfwCreateWindowSurface(mVkInstance, mWindow, nullptr, &mSurface) != VK_SUCCESS)
	{
		Error("App", "createSurface", "failed to create window surface!");
	}
}

// PhysicalDevice
void VkApp::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(mVkInstance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		Error("App", "pickPhysicalDevice", "failed to find GPUs with Vulkan support!");
	}

	VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[deviceCount];
	vkEnumeratePhysicalDevices(mVkInstance, &deviceCount, physicalDevices);

	int* scores = new int[deviceCount];
	for (uint32_t i = 0; i < deviceCount; ++i)
	{
		scores[i] = rateDeviceSuitability(physicalDevices[i]);
	}

	uint32_t biggestScoreIndex = 0;
	int biggestScore = 0;
	for (uint32_t i = 0; i < deviceCount; ++i)
	{
		if (biggestScore < scores[i])
		{
			if (isDeviceSuitable(physicalDevices[i]))
			{
				biggestScore = scores[i];
				biggestScoreIndex = i;
			}
		}
	}

	if (biggestScore == 0)
	{
		Error("App", "pickPhysicalDevice", "failed to find a suitable GPU!");
	}

	mPhysicalDevice = physicalDevices[biggestScoreIndex];
	mSwapchain.MSAA.Samples = getMaxUsableSampleCount();

	delete[] physicalDevices;
	delete[] scores;
}

int VkApp::rateDeviceSuitability(const VkPhysicalDevice _device)
{
	int score = 0;

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(_device, &deviceProperties);

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	/*
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	// Application can't function without geometry shaders
	if (!deviceFeatures.geometryShader) {
		return 0;
	}
	*/

	return score;
}

bool VkApp::isDeviceSuitable(const VkPhysicalDevice _physicalDevice)
{
	QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);

	bool bExtensionsSupported = checkDeviceExtensionSupport(_physicalDevice);

	bool bSwapChainAdequate = false;
	if (bExtensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicalDevice);
		bSwapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(_physicalDevice, &supportedFeatures);

	return indices.IsComplete() && bExtensionsSupported && bSwapChainAdequate
		&& supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices VkApp::findQueueFamilies(const VkPhysicalDevice _physicaldevice)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(_physicaldevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(_physicaldevice, &queueFamilyCount, queueFamilies.data());

	uint32_t i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.GraphicsFamily = i;
		}

		VkBool32 bPresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(_physicaldevice, i, mSurface, &bPresentSupport);
		if (bPresentSupport)
		{
			indices.PresentFamily = i;
		}

		if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			indices.ComputeFamily = i;
		}

		if (indices.IsComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

bool VkApp::checkDeviceExtensionSupport(const VkPhysicalDevice _physicalDevice)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(DEVICE_EXTENSION.begin(), DEVICE_EXTENSION.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails VkApp::querySwapChainSupport(const VkPhysicalDevice _physicalDevice)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, mSurface, &details.Capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, mSurface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		details.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, mSurface, &formatCount, details.Formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, mSurface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		details.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, mSurface, &presentModeCount, details.PresentModes.data());
	}

	return details;
}

// LogicalDevice
void VkApp::createLogicalDevice()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(mPhysicalDevice);

	std::set<uint32_t> uniqueQueueFamilies
		= { queueFamilyIndices.GraphicsFamily.value(), queueFamilyIndices.PresentFamily.value(), queueFamilyIndices.ComputeFamily.value() };

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	float queuePriority = 1.0f;	// 0.0 ~ 1.0
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE;
	//deviceFeatures.sampleRateShading = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSION.size());
	createInfo.ppEnabledExtensionNames = DEVICE_EXTENSION.data();

#ifdef NDEBUG
	createInfo.enabledLayerCount = 0;
#else
	/*
	Previous implementations of Vulkan made a distinction between instance and device
	specific validation layers, but this is no longer the case.
	That means that the enabledLayerCount and ppEnabledLayerNames fields of
	VkDeviceCreateInfo are ignored by up-to-date implementations.
	However, it is still a good idea to set them anyway to be compatible with older implementations

	Vulkan의 이전 구현은 인스턴스와 장치 별 유효성 검사 계층을 구분했지만 더 이상 그렇지 않습니다 .
	즉,의 enabledLayerCount및 ppEnabledLayerNames필드는 VkDeviceCreateInfo최신 구현에서 무시됩니다.
	그러나 이전 구현과 호환되도록 설정하는 것이 좋습니다.
	*/
	createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
	createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
#endif

	if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS)
	{
		Error("App", "createLogicalDevice", "failed to create logical device!");
	}

	mGraphics.QueueFamily = queueFamilyIndices.GraphicsFamily.value();
	mCompute.QueueFamily = queueFamilyIndices.ComputeFamily.value();
	mGraphics.PresentQueueFamily = queueFamilyIndices.PresentFamily.value();
	vkGetDeviceQueue(mDevice, mGraphics.QueueFamily, 0, &mGraphics.Queue);
	vkGetDeviceQueue(mDevice, mCompute.QueueFamily, 0, &mCompute.Queue);
	vkGetDeviceQueue(mDevice, mGraphics.PresentQueueFamily, 0, &mGraphics.PresentQueue);
}

// SwapChain
void VkApp::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(mPhysicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.Formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.PresentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.Capabilities);

	uint32_t imageCount = MAX_FRAMES_IN_FLIGHT + 1 - swapChainSupport.Capabilities.minImageCount;
	if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.Capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = mSurface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	/*
	It is also possible that you'll render images to a separate image first to perform operations
	like post-processing. In that case you may use a value like "VK_IMAGE_USAGE_TRANSFER_DST_BIT"
	instead and use a memory operation to transfer the rendered image to a swap chain image.

	사후 처리와 같은 작업을 수행하기 위해 먼저 이미지를 별도의 이미지로 렌더링 할 수도 있습니다.
	이 경우 VK_IMAGE_USAGE_TRANSFER_DST_BIT 값을 사용하고 메모리 작업을 사용하여
	렌더링 된 이미지를 스왑 체인 이미지로 전송할 수 있습니다.
	*/

	QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice);

	uint32_t queueFamilyIndices[2] = { indices.GraphicsFamily.value(), indices.PresentFamily.value() };

	if (indices.GraphicsFamily != indices.PresentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}
	createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;	// opaque : 불투명한
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapchain.Swapchain) != VK_SUCCESS)
	{
		Error("App", "createSwapChain", "failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(mDevice, mSwapchain.Swapchain, &imageCount, nullptr);
	mSwapchain.Images.resize(imageCount);
	vkGetSwapchainImagesKHR(mDevice, mSwapchain.Swapchain, &imageCount, mSwapchain.Images.data());

	mSwapchain.ImageFormat = surfaceFormat.format;
	mSwapchain.Extent = extent;
}

void VkApp::recreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(mWindow, &width, &height);
	if (width == 0 || height == 0)
	{
		return;
	}

	{
		std::scoped_lock<std::mutex> lock(mMutexResizedFramebuffer);

		vkDeviceWaitIdle(mDevice);

		cleanupSwapChain();

		createSwapChain();
		createSwapChainImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createGraphicsCubePipeline();
		createComputePipeline();
		createColorResources();
		createDepthResources();
		createStorageImages();
		createFramebuffers();
		mGameData.SetProjectionMat(static_cast<float>(mSwapchain.Extent.width), static_cast<float>(mSwapchain.Extent.height));
		createDescriptorPool();
		createDescriptorSets();
		createComputeDescriptorPool();
		createComputeDescriptorSets();
		createCommandBuffers();
		createComputeCommandBuffers();

		mbResizedFramebuffer = false;
	}
}

void VkApp::cleanupSwapChain()
{
	vkDestroyImageView(mDevice, mSwapchain.MSAA.ColorImageView, nullptr);
	vkDestroyImage(mDevice, mSwapchain.MSAA.ColorImage, nullptr);
	vkFreeMemory(mDevice, mSwapchain.MSAA.ColorImageMemory, nullptr);

	vkDestroyImageView(mDevice, mSwapchain.DepthImageView, nullptr);
	vkDestroyImage(mDevice, mSwapchain.DepthImage, nullptr);
	vkFreeMemory(mDevice, mSwapchain.DepthImageMemory, nullptr);

	for (size_t i = 0; i < mStorageImages.size(); i++)
	{
		vkDestroyImageView(mDevice, mStorageImages[i].View, VK_NULL_HANDLE);
		vkDestroyImage(mDevice, mStorageImages[i].Image, VK_NULL_HANDLE);
		vkFreeMemory(mDevice, mStorageImages[i].Memory, VK_NULL_HANDLE);
	}

	for (auto& framebuffer : mSwapchain.Framebuffers)
	{
		vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(mDevice, mGraphics.CommandPool, static_cast<uint32_t>(mGraphics.CommandBuffers.size()), mGraphics.CommandBuffers.data());
	vkFreeCommandBuffers(mDevice, mCompute.CommandPool, static_cast<uint32_t>(mCompute.CommandBuffers.size()), mCompute.CommandBuffers.data());

	vkDestroyPipeline(mDevice, mGraphics.Pipeline, nullptr);
	vkDestroyPipelineLayout(mDevice, mGraphics.PipelineLayout, nullptr);
	vkDestroyPipeline(mDevice, mGraphics.CubePipeline, nullptr);
	vkDestroyPipelineLayout(mDevice, mGraphics.CubePipelineLayout, nullptr);
	vkDestroyRenderPass(mDevice, mGraphics.RenderPass, nullptr);

	vkDestroyPipeline(mDevice, mCompute.Pipeline, VK_NULL_HANDLE);
	vkDestroyPipelineLayout(mDevice, mCompute.PipelineLayout, VK_NULL_HANDLE);

	for (auto imageView : mSwapchain.ImageViews)
	{
		vkDestroyImageView(mDevice, imageView, nullptr);
	}
	vkDestroySwapchainKHR(mDevice, mSwapchain.Swapchain, nullptr);

	vkDestroyDescriptorPool(mDevice, mGraphics.DescriptorPool, nullptr);
	vkDestroyDescriptorPool(mDevice, mCompute.DescriptorPool, nullptr);
}

void VkApp::createSwapChainImageViews()
{
	mSwapchain.ImageViews.resize(mSwapchain.Images.size());

	for (uint32_t i = 0; i < mSwapchain.Images.size(); i++)
	{
		mSwapchain.ImageViews[i] = createImageView(mSwapchain.Images[i], mSwapchain.ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

VkSurfaceFormatKHR VkApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _rAvailableFormats)
{
	VkSurfaceFormatKHR surfaceFormat;
	for (const auto& availableFormat : _rAvailableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
			&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			surfaceFormat = availableFormat;
			goto end;
		}
	}

	assert(false);
	Error("App", "chooseSwapSurfaceFormat", "The required format does not exist.");

end:
	return surfaceFormat;
}

VkPresentModeKHR VkApp::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _rAvailablePresentModes)
{
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : _rAvailablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = availablePresentMode;
			break;
		}
	}

	return presentMode;
}

VkExtent2D VkApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _rCapabilities)
{
	VkExtent2D extent;
	if (_rCapabilities.currentExtent.width != UINT32_MAX)
	{
		extent = _rCapabilities.currentExtent;
	}
	else
	{
		glfwGetFramebufferSize(mWindow, &g_Width, &g_Height);

		extent = { static_cast<uint32_t>(g_Width), static_cast<uint32_t>(g_Height) };
	}

	return extent;
}

void VkApp::framebufferResizeCallback(GLFWwindow* _pWindow, int _width, int _height)
{
	VkApp* app = reinterpret_cast<VkApp*>(glfwGetWindowUserPointer(_pWindow));

	app->mbResizedFramebuffer = true;

	if (app->mbInitedVulkan == false) { return; }

	app->recreateSwapChain();
}

// DepthResources
void VkApp::createDepthResources()
{
	VkFormat depthFormat = findDepthFormat();

	createImage(mSwapchain.Extent.width, mSwapchain.Extent.height, 1, mSwapchain.MSAA.Samples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mSwapchain.DepthImage, mSwapchain.DepthImageMemory);
	mSwapchain.DepthImageView = createImageView(mSwapchain.DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	transitionImageLayout(mSwapchain.DepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

VkFormat VkApp::findDepthFormat()
{
	return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool VkApp::hasStencilComponent(const VkFormat _format)
{
	return _format == VK_FORMAT_D32_SFLOAT_S8_UINT || _format == VK_FORMAT_D24_UNORM_S8_UINT;
}

// MSAA
VkSampleCountFlagBits VkApp::getMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(mPhysicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts
		& physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

void VkApp::createColorResources()
{
	VkFormat colorFormat = mSwapchain.ImageFormat;

	createImage(mSwapchain.Extent.width, mSwapchain.Extent.height, 1, mSwapchain.MSAA.Samples, colorFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mSwapchain.MSAA.ColorImage, mSwapchain.MSAA.ColorImageMemory);
	mSwapchain.MSAA.ColorImageView = createImageView(mSwapchain.MSAA.ColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

// GraphicsPipeline
void VkApp::createGraphicsPipeline()
{
	std::vector<char> vertShaderCode = ReadFile((SHADER_PATH + "vert.spv").c_str());
	std::vector<char> fragShaderCode = ReadFile((SHADER_PATH + "frag.spv").c_str());

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	bindingDescriptions.reserve(2);
	bindingDescriptions.push_back(Vertex::InputBindingDescription());
	bindingDescriptions.push_back(GameObject::InputBindingDescription());

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	attributeDescriptions.reserve(10);
	std::vector<VkVertexInputAttributeDescription> vertAttributes = Vertex::InputAttributeDescriptions();
	for (auto& attribute : vertAttributes)
	{
		attributeDescriptions.push_back(std::move(attribute));
	}
	std::vector<VkVertexInputAttributeDescription> InstAttributes = GameObject::InputAttributeDescriptions();
	for (auto& attribute : InstAttributes)
	{
		attributeDescriptions.push_back(std::move(attribute));
	}

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	// VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)mSwapchain.Extent.width;	// Remember that the size of the swap chain and its images may differ from the WIDTH and HEIGHT of the window.
	viewport.height = (float)mSwapchain.Extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = mSwapchain.Extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	//rasterizer.rasterizerDiscardEnable = VK_TRUE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE; // VK_TRUE; // enable sample shading in the pipeline
	multisampling.rasterizationSamples = mSwapchain.MSAA.Samples;
	multisampling.minSampleShading = 1.0f; // 0.2f; // min fraction for sample shading; closer to one is smoother
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional // OP == operator
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
	/*
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	*/

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	/*
	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;	// 0
	dynamicState.pDynamicStates = dynamicStates;	// nullptr
	*/

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstantCmdObject);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &mGraphics.DescriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

	if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mGraphics.PipelineLayout) != VK_SUCCESS)
	{
		Error("App", "createGraphicsPipeline", "failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = mGraphics.PipelineLayout;
	pipelineInfo.renderPass = mGraphics.RenderPass;
	pipelineInfo.subpass = 0;
	//pipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT; // VK_PIPELINE_CREATE_DISPATCH_BASE 
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphics.Pipeline) != VK_SUCCESS)
	{
		Error("App", "createGraphicsPipeline", "failed to create graphics pipeline!");
	}


	vkDestroyShaderModule(mDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(mDevice, vertShaderModule, nullptr);
}

void VkApp::createGraphicsCubePipeline()
{
	std::vector<char> vertShaderCode = ReadFile((SHADER_PATH + "cube_vert.spv").c_str());
	std::vector<char> fragShaderCode = ReadFile((SHADER_PATH + "cube_frag.spv").c_str());

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	bindingDescriptions.reserve(2);
	bindingDescriptions.push_back(Vertex::InputBindingDescription());
	bindingDescriptions.push_back(CubeObject::InputBindingDescription());

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	attributeDescriptions.reserve(10);
	std::vector<VkVertexInputAttributeDescription> vertAttributes = Vertex::InputAttributeDescriptions();
	for (auto& attribute : vertAttributes)
	{
		attributeDescriptions.push_back(std::move(attribute));
	}
	std::vector<VkVertexInputAttributeDescription> InstAttributes = CubeObject::InputAttributeDescriptions();
	for (auto& attribute : InstAttributes)
	{
		attributeDescriptions.push_back(std::move(attribute));
	}

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	// VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)mSwapchain.Extent.width;	// Remember that the size of the swap chain and its images may differ from the WIDTH and HEIGHT of the window.
	viewport.height = (float)mSwapchain.Extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = mSwapchain.Extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	//rasterizer.rasterizerDiscardEnable = VK_TRUE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE; // VK_TRUE; // enable sample shading in the pipeline
	multisampling.rasterizationSamples = mSwapchain.MSAA.Samples;
	multisampling.minSampleShading = 1.0f; // 0.2f; // min fraction for sample shading; closer to one is smoother
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional // OP == operator
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
	/*
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	*/

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	/*
	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;	// 0
	dynamicState.pDynamicStates = dynamicStates;	// nullptr
	*/

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstantCmdObject);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &mGraphics.DescriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

	if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mGraphics.CubePipelineLayout) != VK_SUCCESS)
	{
		Error("App", "createGraphicsPipeline", "failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = mGraphics.CubePipelineLayout;
	pipelineInfo.renderPass = mGraphics.RenderPass;
	pipelineInfo.subpass = 0;
	//pipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT; // VK_PIPELINE_CREATE_DISPATCH_BASE 
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphics.CubePipeline) != VK_SUCCESS)
	{
		Error("App", "createGraphicsPipeline", "failed to create graphics pipeline!");
	}


	vkDestroyShaderModule(mDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(mDevice, vertShaderModule, nullptr);
}

VkShaderModule VkApp::createShaderModule(const std::vector<char>& _rCode)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = _rCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(_rCode.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(mDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		Error("App", "createShaderModule", "failed to create shader module!");
	}

	return shaderModule;
}

void VkApp::createRenderPass()
{
	VkAttachmentDescription colorAttachmentDescription = {};
	colorAttachmentDescription.format = mSwapchain.ImageFormat;
	colorAttachmentDescription.samples = mSwapchain.MSAA.Samples;
	colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: 컬러 첨부 파일로 사용되는 이미지 // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: 스왑 체인에 표시 될 이미지 // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : 메모리 복사 작업의 대상으로 사용되는 이미지

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;	// VkAttachmentDescription index
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	VkAttachmentDescription depthAttachmentDescription = {};
	depthAttachmentDescription.format = findDepthFormat();
	depthAttachmentDescription.samples = mSwapchain.MSAA.Samples;
	depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// 그리기가 끝난 후에는 사용되지 않기 때문
	depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;	// VkAttachmentDescription index
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	VkAttachmentDescription colorAttachmentResolve = {};
	colorAttachmentResolve.format = mSwapchain.ImageFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2;	// VkAttachmentDescription index
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	// Vulkan은 향후 컴퓨팅 서브 패스도 지원할 수 있으므로 이를 그래픽 서브 패스로 명시해야합니다.
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;
	// 이 배열의 첨부 색인은 layout(location = 0) out vec4 outColor지시문을 사용하여 프레그먼트 쉐이더에서 직접 참조됩니다 !
	// pInputAttachments, pResolveAttachments, pDepthStencilAttachment, pPreserveAttachments


	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;	// ~0U
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachmentDescriptions =
	{ colorAttachmentDescription, depthAttachmentDescription, colorAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.pAttachments = attachmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mGraphics.RenderPass) != VK_SUCCESS)
	{
		Error("App", "createRenderPass", "failed to create render pass!");
	}
}

void VkApp::createFramebuffers()
{
	mSwapchain.Framebuffers.resize(mSwapchain.ImageViews.size());

	for (size_t i = 0; i < mSwapchain.ImageViews.size(); i++)
	{
		std::array<VkImageView, 3> attachments = { mSwapchain.MSAA.ColorImageView, mSwapchain.DepthImageView, mSwapchain.ImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = mGraphics.RenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = mSwapchain.Extent.width;
		framebufferInfo.height = mSwapchain.Extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mSwapchain.Framebuffers[i]) != VK_SUCCESS)
		{
			Error("App", "createFramebuffers", "failed to create framebuffer!");
		}
	}
}

// ComputePipeline
void VkApp::createComputePipeline()
{
	std::vector<char> compShaderCode = ReadFile(COMPUTE_SHADER_PATH.c_str());
	VkShaderModule computeShaderModule = createShaderModule(compShaderCode);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &mCompute.DescriptorSetLayout; // Optional
	//pipelineLayoutInfo.pushConstantRangeCount; // Optional
	//pipelineLayoutInfo.pPushConstantRanges; // Optional

	if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mCompute.PipelineLayout) != VK_SUCCESS)
	{
		Error("App", "createComputePipeline", "failed to create pipeline layout!");
	}


	VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = computeShaderModule;
	computeShaderStageInfo.pName = "main";


	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	//pipelineInfo.flags;	// ?
	pipelineInfo.stage = computeShaderStageInfo;
	pipelineInfo.layout = mCompute.PipelineLayout;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateComputePipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mCompute.Pipeline) != VK_SUCCESS)
	{
		Error("App", "createComputePipeline", "failed to create graphics pipeline!");
	}


	vkDestroyShaderModule(mDevice, computeShaderModule, VK_NULL_HANDLE);
}

// GraphicsDescriptorSet
void VkApp::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;	// use shader
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &mGraphics.DescriptorSetLayout) != VK_SUCCESS)
	{
		Error("App", "createDescriptorSetLayout", "failed to create descriptor set layout!");
	}
}

void VkApp::createDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes;

	VkDescriptorPoolSize poolSize = {};

	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(mSwapchain.Images.size());
	poolSizes.push_back(poolSize);

	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = static_cast<uint32_t>(mSwapchain.Images.size());
	poolSizes.push_back(poolSize);

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(mSwapchain.Images.size());

	if (vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mGraphics.DescriptorPool) != VK_SUCCESS)
	{
		Error("App", "createDescriptorPool", "failed to create descriptor pool!");
	}
}

void VkApp::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(mSwapchain.Images.size(), mGraphics.DescriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = mGraphics.DescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(mSwapchain.Images.size());
	allocInfo.pSetLayouts = layouts.data();

	mGraphics.DescriptorSets.resize(mSwapchain.Images.size());
	if (vkAllocateDescriptorSets(mDevice, &allocInfo, mGraphics.DescriptorSets.data()) != VK_SUCCESS)
	{
		Error("App", "createDescriptorSets", "failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < mSwapchain.Images.size(); i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = mUniformVisibleBuffers[i].Buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo textureImageInfo = {};
		textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		textureImageInfo.imageView = mTexture.ImageView;
		textureImageInfo.sampler = mTexture.Sampler;


		std::vector<VkWriteDescriptorSet> descriptorWrites;

		VkWriteDescriptorSet descriptorWrite = {};

		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = mGraphics.DescriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pTexelBufferView = nullptr; // Optional
		descriptorWrites.push_back(descriptorWrite);

		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = mGraphics.DescriptorSets[i];
		descriptorWrite.dstBinding = 1;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &textureImageInfo;
		descriptorWrites.push_back(descriptorWrite);

		vkUpdateDescriptorSets(mDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

// ComputeDescriptorSet
void VkApp::createComputeDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding computeLayoutBinding = {};
	computeLayoutBinding.binding = COMPUTE_BINDING;	// use shader
	computeLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	computeLayoutBinding.descriptorCount = 1;
	computeLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { computeLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &mCompute.DescriptorSetLayout) != VK_SUCCESS)
	{
		Error("App", "createDescriptorSetLayout", "failed to create descriptor set layout!");
	}
}

void VkApp::createComputeDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes;

	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSize.descriptorCount = static_cast<uint32_t>(mSwapchain.Images.size());
	poolSizes.push_back(poolSize);

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(mSwapchain.Images.size());

	if (vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mCompute.DescriptorPool) != VK_SUCCESS)
	{
		Error("App", "createDescriptorPool", "failed to create descriptor pool!");
	}
}

void VkApp::createComputeDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(mSwapchain.Images.size(), mCompute.DescriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = mCompute.DescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(mSwapchain.Images.size());
	allocInfo.pSetLayouts = layouts.data();

	mCompute.DescriptorSets.resize(mSwapchain.Images.size());
	if (vkAllocateDescriptorSets(mDevice, &allocInfo, mCompute.DescriptorSets.data()) != VK_SUCCESS)
	{
		Error("App", "createDescriptorSets", "failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < mSwapchain.Images.size(); i++)
	{
		VkDescriptorImageInfo computeImageInfo = {};
		computeImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		computeImageInfo.imageView = mStorageImages[i].View;
		computeImageInfo.sampler = VK_NULL_HANDLE;

		std::vector<VkWriteDescriptorSet> descriptorWrites;

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = mCompute.DescriptorSets[i];
		descriptorWrite.dstBinding = COMPUTE_BINDING;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &computeImageInfo;
		descriptorWrites.push_back(descriptorWrite);

		vkUpdateDescriptorSets(mDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

// Image
void VkApp::createImage(const uint32_t _width, const uint32_t _height, const uint32_t _mipLevels, 
	const VkSampleCountFlagBits _numSamples, const VkFormat _format, const VkImageTiling _tiling, 
	const VkImageUsageFlags _usage, const VkMemoryPropertyFlags _properties,
	VkImage& _out_rImage, VkDeviceMemory& _out_rImageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = _width;
	imageInfo.extent.height = _height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = _mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = _format;
	imageInfo.tiling = _tiling;	// VK_IMAGE_TILING_LINEAR: 텍셀은 pixels배열 처럼 행 주요 순서로 배치됩니다 // VK_IMAGE_TILING_OPTIMAL: 텍셀은 최적의 액세스를 위해 구현 정의 순서로 배치됩니다.
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	// VK_IMAGE_LAYOUT_PREINITIALIZED
	imageInfo.usage = _usage;
	imageInfo.samples = _numSamples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(mDevice, &imageInfo, nullptr, &_out_rImage) != VK_SUCCESS)
	{
		Error("App", "createImage", "failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(mDevice, _out_rImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, _properties);

	if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &_out_rImageMemory) != VK_SUCCESS)
	{
		Error("App", "createImage", "failed to allocate image memory!");
	}

	vkBindImageMemory(mDevice, _out_rImage, _out_rImageMemory, 0);
}

void VkApp::transitionImageLayout(const VkImage _image, const VkFormat _format, 
	const VkImageLayout _oldLayout, const  VkImageLayout _newLayout, const uint32_t _mipLevels)
{
	/*
	VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: 프리젠 테이션에 최적
	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : 조각 쉐이더에서 색상을 쓰기위한 첨부 파일로 최적
	VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : 다음과 같은 전송 작업에서 소스로 최적 vkCmdCopyImageToBuffer
	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : 다음과 같은 전송 작업에서 목적지로 최적 vkCmdCopyBufferToImage
	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : 셰이더에서 샘플링하기에 최적
	VK_IMAGE_LAYOUT_GENERAL
	*/

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = _oldLayout;
		barrier.newLayout = _newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;	// 큐 패밀리 소유권을 전송하기 위해 장벽을 사용하는 경우 이 두 필드는 큐 패밀리의 색인
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = _image;
		//barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = _mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		//barrier.srcAccessMask = 0; // TODO
		//barrier.dstAccessMask = 0; // TODO

		if (_newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (hasStencilComponent(_format))
			{
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (_oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && _newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (_oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && _newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (_oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && _newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			Error("App", "transitionImageLayout", "unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage /* TODO */, destinationStage /* TODO */,	// "생산자( producer )" 스테이지와 "소비자( consumer )" 스테이지
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

	}
	endSingleTimeCommands(commandBuffer);
}

void VkApp::copyBufferToImage(const VkBuffer _buffer, const VkImage _image, 
	const uint32_t _width, const uint32_t _height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { _width, _height, 1 };

	vkCmdCopyBufferToImage(
		commandBuffer,
		_buffer,
		_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region);

	endSingleTimeCommands(commandBuffer);
}

VkImageView VkApp::createImageView(const VkImage _image, const VkFormat _format, 
	const VkImageAspectFlags _aspectFlags, const uint32_t _mipLevels)
{
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = _image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = _format;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;	// ??
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = _aspectFlags;	// VK_IMAGE_ASPECT_COLOR_BIT; // used as color targets
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = _mipLevels;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(mDevice, &createInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		Error("App", "createImageViews", "failed to create image views!");
	}

	return imageView;
}

VkFormat VkApp::findSupportedFormat(const std::vector<VkFormat>& _rCandidates, 
	const VkImageTiling _tiling, const VkFormatFeatureFlags _features)
{
	VkFormat result;
	for (VkFormat format : _rCandidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &props);

		if (_tiling == VK_IMAGE_TILING_LINEAR
			&& (props.linearTilingFeatures & _features) == _features)
		{
			result = format;
			goto end;
		}
		else if (_tiling == VK_IMAGE_TILING_OPTIMAL
			&& (props.optimalTilingFeatures & _features) == _features)
		{
			result = format;
			goto end;
		}
	}

	Error("App", "findSupportedFormat", "failed to find supported format!");
end:
	return result;
}

// TextureImage
void VkApp::createTextureImage()
{
	int texWidth, texHeight, texChannels;
	//stbi_uc* pixels = stbi_load("textures/texture.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth * texHeight * 4);
	mTexture.MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	if (pixels == nullptr)
	{
		Error("App", "createTextureImage", "failed to load texture image!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(mDevice, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(mDevice, stagingBufferMemory);

	stbi_image_free(pixels);

	createImage(texWidth, texHeight, mTexture.MipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mTexture.Image, mTexture.ImageMemory);

	transitionImageLayout(mTexture.Image, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mTexture.MipLevels);

	copyBufferToImage(stagingBuffer, mTexture.Image,
		static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	generateMipmaps(mTexture.Image, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mTexture.MipLevels);


	vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
	vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
}

void VkApp::createTextureImageView()
{
	mTexture.ImageView = createImageView(mTexture.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mTexture.MipLevels);
}

void VkApp::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;	// VK_FILTER_NEAREST
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// 축 X, Y, Z -> U, V, W
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;	// 이방성 필터링
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.minLod = 0; // Optional // static_cast<float>(mTextureMipLevels/2); 
	samplerInfo.maxLod = static_cast<float>(mTexture.MipLevels);
	samplerInfo.mipLodBias = 0; // Optional

	if (vkCreateSampler(mDevice, &samplerInfo, nullptr, &mTexture.Sampler) != VK_SUCCESS)
	{
		Error("App", "createTextureSampler", "failed to create texture sampler!");
	}
}

void VkApp::generateMipmaps(const VkImage _image, const VkFormat _imageFormat, const int32_t _texWidth, 
	const int32_t _texHeight, const uint32_t _mipLevels)
{
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, _imageFormat, &formatProperties);

	if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == false)
	{
		Error("App", "generateMipmaps", "texture image format does not support linear blitting!");
	}


	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = _image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;


		int32_t mipWidth = _texWidth;
		int32_t mipHeight = _texHeight;

		for (uint32_t i = 1; i < _mipLevels; i++)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);


			VkImageBlit blit = {};

			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;

			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);


			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);


			if (mipWidth > 1)
			{
				mipWidth /= 2;
			}
			if (mipHeight > 1)
			{
				mipHeight /= 2;
			}
		}

		barrier.subresourceRange.baseMipLevel = _mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

	}
	endSingleTimeCommands(commandBuffer);
}

// StorageImage
void VkApp::createStorageImages()
{
	mStorageImages.resize(mSwapchain.Images.size());

	VkFormat format = findSupportedFormat({ VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM },
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);
	for (size_t i = 0; i < mSwapchain.Images.size(); i++)
	{
		createImage(mSwapchain.Extent.width, mSwapchain.Extent.height, 1, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mStorageImages[i].Image, mStorageImages[i].Memory);

		mStorageImages[i].View = createImageView(mStorageImages[i].Image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

// VertexBuffer, IndexBuffer, InstanceBuffer, UniformBuffer
void VkApp::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(mGameData.GetVertices()[0]) * mGameData.GetVertices().size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mGameData.GetVertices().data(), (size_t)bufferSize);
	vkUnmapMemory(mDevice, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mVertexBuffer.Buffer, mVertexBuffer.Memory);

	copyBuffer(stagingBuffer, mVertexBuffer.Buffer, bufferSize);


	vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
	vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
}

void VkApp::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(mGameData.GetIndices()[0]) * mGameData.GetIndices().size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mGameData.GetIndices().data(), (size_t)bufferSize);
	vkUnmapMemory(mDevice, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mIndexBuffer.Buffer, mIndexBuffer.Memory);

	copyBuffer(stagingBuffer, mIndexBuffer.Buffer, bufferSize);

	vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
	vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
}

void VkApp::createInstanceBuffers()
{
	mInstancesVisibleBuffers.resize(mSwapchain.Images.size());

	VkDeviceSize bufferSize = mGameData.GetInstancesDataByteCapacitySize();

	for (uint32_t i = 0; i < mSwapchain.Images.size(); i++)
	{
		mInstancesVisibleBuffers[i].BufferSize = bufferSize;
		mInstancesVisibleBuffers[i].TypeOffsets.resize(static_cast<size_t>(eGameObjectType::COUNT));

		createBuffer(mInstancesVisibleBuffers[i].BufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			mInstancesVisibleBuffers[i].Buffer, mInstancesVisibleBuffers[i].Memory);
	}
}

void VkApp::createCubeInstanceBuffers()
{
	mCubeInstancesVisibleBuffers.resize(mSwapchain.Images.size());

	VkDeviceSize bufferSize = mGameData.GetCubeDataByteCapacitySize();

	for (uint32_t i = 0; i < mSwapchain.Images.size(); i++)
	{
		mCubeInstancesVisibleBuffers[i].BufferSize = bufferSize;

		createBuffer(mCubeInstancesVisibleBuffers[i].BufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
			mCubeInstancesVisibleBuffers[i].Buffer, mCubeInstancesVisibleBuffers[i].Memory);

		vkMapMemory(mDevice, mCubeInstancesVisibleBuffers[i].Memory, 0,
			mCubeInstancesVisibleBuffers[i].BufferSize, 0, &mCubeInstancesVisibleBuffers[i].pMapMemory);
	}
}

void VkApp::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	mUniformVisibleBuffers.resize(mSwapchain.Images.size());

	for (uint32_t i = 0; i < mSwapchain.Images.size(); i++)
	{
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			mUniformVisibleBuffers[i].Buffer, mUniformVisibleBuffers[i].Memory);
		updateUniformBuffer(i);
	}
}

uint32_t VkApp::findMemoryType(const uint32_t _typeFilter, const VkMemoryPropertyFlags _properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProperties);

	uint32_t i = 0;
	for (; i < memProperties.memoryTypeCount; i++)
	{
		if ((_typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & _properties) == _properties)
		{
			goto end;
		}
	}

	Error("App", "findMemoryType", "failed to find suitable memory type!");

end:
	return i;
}

void VkApp::createBuffer(const VkDeviceSize _size, const VkBufferUsageFlags _usage, 
	const VkMemoryPropertyFlags _properties, VkBuffer& _out_rBuffer, VkDeviceMemory& _out_rBufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = _size;
	bufferInfo.usage = _usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &_out_rBuffer) != VK_SUCCESS)
	{
		Error("App", "createBuffer", "failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(mDevice, _out_rBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, _properties);

	if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &_out_rBufferMemory) != VK_SUCCESS)
	{
		Error("App", "createBuffer", "failed to allocate buffer memory!");
	}

	vkBindBufferMemory(mDevice, _out_rBuffer, _out_rBufferMemory, 0);
}

void VkApp::copyBuffer(const VkBuffer _srcBuffer, const VkBuffer _dstBuffer, const VkDeviceSize _size)
{
	// 메모리 할당 최적화 : 임시 커맨드 풀 생성 : flag : VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = _size;
	vkCmdCopyBuffer(commandBuffer, _srcBuffer, _dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}


// GraphicsCommand
void VkApp::createCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = mGraphics.QueueFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

	if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mGraphics.CommandPool) != VK_SUCCESS)
	{
		Error("App", "createCommandPool", "failed to create command pool!");
	}
}

void VkApp::createCommandBuffers()
{
	mGraphics.CommandBuffers.resize(mSwapchain.Framebuffers.size());

	mPushConstantCmdData.resize(mSwapchain.Framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = mGraphics.CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)mGraphics.CommandBuffers.size();

	if (vkAllocateCommandBuffers(mDevice, &allocInfo, mGraphics.CommandBuffers.data()) != VK_SUCCESS)
	{
		Error("App", "createCommandBuffers", "failed to allocate command buffers!");
	}

	for (size_t i = 0; i < mGraphics.CommandBuffers.size(); i++)
	{
		recordCommandBuffer(i);
	}
}

void VkApp::recordCommandBuffer(const size_t _index)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(mGraphics.CommandBuffers[_index], &beginInfo) != VK_SUCCESS)
	{
		Error("App", "createCommandBuffer", "failed to begin recording command buffer!");
	}
	{
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = mGraphics.RenderPass;
		renderPassBeginInfo.framebuffer = mSwapchain.Framebuffers[_index];
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = mSwapchain.Extent;
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(mGraphics.CommandBuffers[_index], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			mPushConstantCmdData[_index].ProjView = mGameData.GetPushConstantData().ProjView;

			// cube
#if 1
			{
				uint32_t nextFirstInstance = 0;

				vkCmdPushConstants(mGraphics.CommandBuffers[_index], mGraphics.CubePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantCmdObject), &mPushConstantCmdData[_index]);

				vkCmdBindDescriptorSets(mGraphics.CommandBuffers[_index], VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphics.CubePipelineLayout,
					0, 1, &mGraphics.DescriptorSets[_index], 0, nullptr);

				vkCmdBindPipeline(mGraphics.CommandBuffers[_index], VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphics.CubePipeline);


				uint32_t count = static_cast<uint32_t>(mGameData.GetShowCubeDataCount());
				cmdDrawIndexed(mGraphics.CommandBuffers[_index], CubeVertexBuffer, 0,
					CubeIndexBuffer, 0, static_cast<uint32_t>(mGameData.CubeIndices.size()),
					mCubeInstancesVisibleBuffers[_index].Buffer, 0, nextFirstInstance, count);
			}
#endif
#if 0
			// Draw
			{
				uint32_t nextFirstInstance = 0;

				vkCmdPushConstants(mGraphics.CommandBuffers[_index], mGraphics.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantCmdObject), &mPushConstantCmdData[_index]);

				vkCmdBindDescriptorSets(mGraphics.CommandBuffers[_index], VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphics.PipelineLayout,
					0, 1, &mGraphics.DescriptorSets[_index], 0, nullptr);

				vkCmdBindPipeline(mGraphics.CommandBuffers[_index], VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphics.Pipeline);

				for (size_t i = 0; i < static_cast<size_t>(eGameObjectType::COUNT); i++)
				{
					uint32_t count = static_cast<uint32_t>(mGameData.GetInstancesDataCount(static_cast<eGameObjectType>(i)));
					if (count == 0) { continue; }
					cmdDrawIndexed(mGraphics.CommandBuffers[_index], mVertexBuffer.Buffer, 0,
						mIndexBuffer.Buffer, 0, static_cast<uint32_t>(mGameData.GetIndices().size()),
						mInstancesVisibleBuffers[_index].Buffer, mInstancesVisibleBuffers[_index].TypeOffsets[i],
						nextFirstInstance, count);
				}
			}
#endif

		}
		vkCmdEndRenderPass(mGraphics.CommandBuffers[_index]);


		cmdImageBarrier(mGraphics.CommandBuffers[_index], mStorageImages[_index].Image,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED);

		cmdCopyColorImage(mGraphics.CommandBuffers[_index],
			mSwapchain.Images[_index], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mStorageImages[_index].Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			mSwapchain.Extent.width, mSwapchain.Extent.height);

		cmdImageBarrier(mGraphics.CommandBuffers[_index], mStorageImages[_index].Image,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT, 0,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
			mGraphics.QueueFamily, mCompute.QueueFamily);

	}
	if (vkEndCommandBuffer(mGraphics.CommandBuffers[_index]) != VK_SUCCESS)
	{
		Error("App", "createCommandBuffer", "failed to record command buffer!");
	}
}

void VkApp::cmdDrawIndexed(const VkCommandBuffer _commandBuffer,
	const VkBuffer _vertexBuffer, const VkDeviceSize _vertexOffset,
	const VkBuffer _indexBuffer, const VkDeviceSize _indexOffset, const uint32_t _indexCount,
	const VkBuffer _instanceBuffer, const VkDeviceSize _instanceOffset, 
	uint32_t& _out_rFirstInstance, const uint32_t _instanceCount)
{
	VkBuffer vertexBuffers[] = { _vertexBuffer };
	VkDeviceSize vertexOffsets[] = { _vertexOffset };
	vkCmdBindVertexBuffers(_commandBuffer, VERTEX_BINDING, 1, vertexBuffers, vertexOffsets);

	vkCmdBindIndexBuffer(_commandBuffer, _indexBuffer, _indexOffset, VK_INDEX_TYPE_UINT32);

	VkBuffer instanceBuffers[] = { _instanceBuffer };
	VkDeviceSize instanceOffsets[] = { _instanceOffset };
	vkCmdBindVertexBuffers(_commandBuffer, INSTANCE_BINDING, 1, instanceBuffers, instanceOffsets);

	vkCmdDrawIndexed(_commandBuffer, _indexCount, _instanceCount, 0, 0, _out_rFirstInstance);

	_out_rFirstInstance += _instanceCount;
}

VkCommandBuffer VkApp::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = mGraphics.CommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VkApp::endSingleTimeCommands(const VkCommandBuffer _commandBuffer)
{
	vkEndCommandBuffer(_commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_commandBuffer;

	vkQueueSubmit(mGraphics.Queue, 1, &submitInfo, VK_NULL_HANDLE);	// 펜스를 사용하면 여러 전송을 동시에 예약하고 한 번에 하나씩 실행하는 대신 모든 전송이 완료 될 때까지 기다릴 수 있습니다. 이는 운전자에게 더 많은 최적화 기회를 제공 할 수 있습니다.

	vkQueueWaitIdle(mGraphics.Queue);

	vkFreeCommandBuffers(mDevice, mGraphics.CommandPool, 1, &_commandBuffer);
}

// ComputeCommand
void VkApp::createComputeCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = mCompute.QueueFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

	if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCompute.CommandPool) != VK_SUCCESS)
	{
		Error("App", "createCommandPool", "failed to create command pool!");
	}
}

void VkApp::createComputeCommandBuffers()
{
	mCompute.CommandBuffers.resize(mSwapchain.Framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = mCompute.CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)mCompute.CommandBuffers.size();

	if (vkAllocateCommandBuffers(mDevice, &allocInfo, mCompute.CommandBuffers.data()) != VK_SUCCESS)
	{
		Error("App", "createCommandBuffers", "failed to allocate command buffers!");
	}

	for (size_t i = 0; i < mCompute.CommandBuffers.size(); i++)
	{
		recordComputeCommandBuffer(i);
	}
}

void VkApp::recordComputeCommandBuffer(const size_t _index)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	QueueFamilyIndices queueIndices = findQueueFamilies(mPhysicalDevice);

	if (vkBeginCommandBuffer(mCompute.CommandBuffers[_index], &beginInfo) != VK_SUCCESS)
	{
		Error("App", "createCommandBuffer", "failed to begin recording command buffer!");
	}
	{
		vkCmdBindPipeline(mCompute.CommandBuffers[_index], VK_PIPELINE_BIND_POINT_COMPUTE, mCompute.Pipeline);

		vkCmdBindDescriptorSets(mCompute.CommandBuffers[_index], VK_PIPELINE_BIND_POINT_COMPUTE, mCompute.PipelineLayout,
			0, 1, &mCompute.DescriptorSets[_index], 0, nullptr);


		vkCmdDispatch(mCompute.CommandBuffers[_index], mSwapchain.Extent.width, mSwapchain.Extent.height, 1);


		cmdImageBarrier(mCompute.CommandBuffers[_index], mStorageImages[_index].Image,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED);
		cmdImageBarrier(mCompute.CommandBuffers[_index], mSwapchain.Images[_index],
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED);

		cmdCopyColorImage(mCompute.CommandBuffers[_index],
			mStorageImages[_index].Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mSwapchain.Images[_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			mSwapchain.Extent.width, mSwapchain.Extent.height);

		cmdImageBarrier(mCompute.CommandBuffers[_index], mSwapchain.Images[_index],
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT, 0,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			mCompute.QueueFamily, mGraphics.QueueFamily);
	}
	if (vkEndCommandBuffer(mCompute.CommandBuffers[_index]) != VK_SUCCESS)
	{
		Error("App", "createCommandBuffer", "failed to record command buffer!");
	}
}

void VkApp::cmdCopyColorImage(const VkCommandBuffer _cmdBuffer,
	const VkImage _srcImage, const VkImageLayout _srcImageLayout, 
	const VkImage _dstImage, const VkImageLayout _dstImageLayout,
	const uint32_t _width, const uint32_t _height)
{
	VkImageSubresourceLayers subresource = {};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.mipLevel = 0;
	subresource.baseArrayLayer = 0;
	subresource.layerCount = 1;

	VkExtent3D extent3D = {};
	extent3D.width = mSwapchain.Extent.width;
	extent3D.height = mSwapchain.Extent.height;
	extent3D.depth = 0;

	VkImageCopy imageCopy = {};
	imageCopy.srcSubresource = subresource;
	imageCopy.srcOffset = { 0, 0, 0 };
	imageCopy.dstSubresource = subresource;
	imageCopy.dstOffset = { 0, 0, 0 };
	imageCopy.extent = extent3D;

	vkCmdCopyImage(_cmdBuffer, _srcImage, _srcImageLayout, _dstImage, _dstImageLayout, 1, &imageCopy);
}

void VkApp::cmdImageBarrier(const VkCommandBuffer _commandBuffer, const VkImage _image,
	const VkPipelineStageFlags _srcStage, const VkPipelineStageFlags _dstStage,
	const VkAccessFlags _srcAccess, const VkAccessFlags _dstAccess,
	const VkImageLayout _oldLayout, const VkImageLayout _newLayout,
	const uint32_t _oldQueueFamily, const uint32_t _newQueueFamily)
{
	VkImageSubresourceRange range = {};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	VkImageMemoryBarrier imageBarrier = {};
	imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier.srcAccessMask = _srcAccess;
	imageBarrier.dstAccessMask = _dstAccess;
	imageBarrier.oldLayout = _oldLayout;
	imageBarrier.newLayout = _newLayout;
	imageBarrier.srcQueueFamilyIndex = _oldQueueFamily;
	imageBarrier.dstQueueFamilyIndex = _newQueueFamily;
	imageBarrier.image = _image;
	imageBarrier.subresourceRange = range;

	vkCmdPipelineBarrier(_commandBuffer,
		_srcStage, _dstStage,
		0 /*or VK_DEPENDENCY_BY_REGION_BIT*/,
		0, nullptr,
		0, nullptr,
		1, &imageBarrier);
}

// DrawFrame
void VkApp::drawFrame()
{
	std::scoped_lock<std::mutex> lock(mMutexResizedFramebuffer);

	if (mbResizedFramebuffer == true) { return; }

	vkWaitForFences(mDevice, 1, &mInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(mDevice, mSwapchain.Swapchain, UINT64_MAX,
		mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		mbResizedFramebuffer = true;
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		Error("App", "drawFrame", "failed to acquire swap chain image!");
	}

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (mImagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(mDevice, 1, &mImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	// Mark the image as now being in use by this frame
	mImagesInFlight[imageIndex] = mInFlightFences[mCurrentFrame];


	// update buffer
	{
		//std::scoped_lock<std::mutex> lock(mGameData.mMutexGameData);

		updateInstanceBuffer(imageIndex);
		updateCubeInstanceBuffer(imageIndex);

		vkResetCommandBuffer(mGraphics.CommandBuffers[imageIndex], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		recordCommandBuffer(imageIndex);

		vkResetCommandBuffer(mCompute.CommandBuffers[imageIndex], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		recordComputeCommandBuffer(imageIndex);
	}

	// graphics
	VkSubmitInfo graphicsSubmitInfo = {};
	graphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore graphicsWaitSemaphores[1] = { mImageAvailableSemaphores[mCurrentFrame] };
	VkPipelineStageFlags graphicsWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	graphicsSubmitInfo.waitSemaphoreCount = 1;
	graphicsSubmitInfo.pWaitSemaphores = graphicsWaitSemaphores;
	graphicsSubmitInfo.pWaitDstStageMask = graphicsWaitStages;
	graphicsSubmitInfo.commandBufferCount = 1;	// commandBufferCount !!!
	graphicsSubmitInfo.pCommandBuffers = &mGraphics.CommandBuffers[imageIndex];
	VkSemaphore graphicsSignalSemaphores[] = { mGraphics.FinishedSemaphores[mCurrentFrame] };
	graphicsSubmitInfo.signalSemaphoreCount = 1;
	graphicsSubmitInfo.pSignalSemaphores = graphicsSignalSemaphores;

	vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrame]);

	if (vkQueueSubmit(mGraphics.Queue, 1, &graphicsSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		Error("App", "drawFrame", "failed to submit draw command buffer!");
	}


	// compute
	VkSubmitInfo computeSubmitInfo = {};
	computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore computeWaitSemaphores[1] = { mGraphics.FinishedSemaphores[mCurrentFrame] };
	VkPipelineStageFlags computeWaitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	computeSubmitInfo.waitSemaphoreCount = 1;
	computeSubmitInfo.pWaitSemaphores = computeWaitSemaphores;
	computeSubmitInfo.pWaitDstStageMask = computeWaitStages;
	computeSubmitInfo.commandBufferCount = 1;
	computeSubmitInfo.pCommandBuffers = &mCompute.CommandBuffers[imageIndex];
	VkSemaphore computeSignalSemaphores[] = { mCompute.FinishedSemaphores[mCurrentFrame] };
	computeSubmitInfo.signalSemaphoreCount = 1;
	computeSubmitInfo.pSignalSemaphores = computeSignalSemaphores;

	if (vkQueueSubmit(mCompute.Queue, 1, &computeSubmitInfo, mInFlightFences[mCurrentFrame]) != VK_SUCCESS)
	{
		Error("App", "drawFrame", "failed to submit draw command buffer!");
	}


	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = computeSignalSemaphores;
	VkSwapchainKHR swapChains[] = { mSwapchain.Swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(mGraphics.PresentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		mbResizedFramebuffer = true;
	}
	else if (result != VK_SUCCESS)
	{
		Error("App", "drawFrame", "failed to present swap chain image!");
	}

	mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VkApp::createSyncObjects()
{
	assert(MAX_FRAMES_IN_FLIGHT >= mSwapchain.Images.size());

	mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	mGraphics.FinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	mCompute.FinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	mImagesInFlight.resize(mSwapchain.Images.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS
			|| vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mGraphics.FinishedSemaphores[i]) != VK_SUCCESS
			|| vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mCompute.FinishedSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS)
		{
			Error("App", "createSemaphores", "failed to create semaphores!");
		}
	}
}

void VkApp::updateInstanceBuffer(const uint32_t _imageIndex)
{
	VkDeviceSize dataSize = 0;
	for (size_t i = 0; i < static_cast<size_t>(eGameObjectType::COUNT); i++)
	{
		dataSize += mGameData.GetInstancesDataByteSize(static_cast<eGameObjectType>(i));
	}

	if (mInstancesVisibleBuffers[_imageIndex].BufferSize < dataSize)
	{
		vkDestroyBuffer(mDevice, mInstancesVisibleBuffers[_imageIndex].Buffer, nullptr);
		vkFreeMemory(mDevice, mInstancesVisibleBuffers[_imageIndex].Memory, nullptr);

		const VkDeviceSize capacitySize = mGameData.GetInstancesDataByteCapacitySize();

		mInstancesVisibleBuffers[_imageIndex].BufferSize = capacitySize;

		createBuffer(mInstancesVisibleBuffers[_imageIndex].BufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT |VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			mInstancesVisibleBuffers[_imageIndex].Buffer, mInstancesVisibleBuffers[_imageIndex].Memory);
	}

	void* data;
	vkMapMemory(mDevice, mInstancesVisibleBuffers[_imageIndex].Memory, 0, dataSize, 0, &data);
	dataSize = 0;
	for (size_t i = 0; i < static_cast<size_t>(eGameObjectType::COUNT); i++)
	{
		mGameData.CpyInstancesData(static_cast<eGameObjectType>(i), (void*)((char*)data + dataSize));
		mInstancesVisibleBuffers[_imageIndex].TypeOffsets[i] = dataSize;
		dataSize += mGameData.GetInstancesDataByteSize(static_cast<eGameObjectType>(i));
	}
	vkUnmapMemory(mDevice, mInstancesVisibleBuffers[_imageIndex].Memory);
}

void VkApp::updateCubeInstanceBuffer(const uint32_t _imageIndex)
{
	mGameData.ThreadCpyShowCubeData(mCubeInstancesVisibleBuffers[_imageIndex].pMapMemory);

	VkMappedMemoryRange range = {};
	range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.memory = mCubeInstancesVisibleBuffers[_imageIndex].Memory;
	range.offset = 0;
	range.size = VK_WHOLE_SIZE;

	vkFlushMappedMemoryRanges(mDevice, 1, &range);
}

void VkApp::updateUniformBuffer(const uint32_t _imageIndex)
{
	UniformBufferObject ubo = {};

	//ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.view = glm::mat4(1.0f);

	ubo.proj = glm::mat4(1.0f);


	void* data;
	vkMapMemory(mDevice, mUniformVisibleBuffers[_imageIndex].Memory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(mDevice, mUniformVisibleBuffers[_imageIndex].Memory);
}





//  DEBUG -------------------------------------------------------------------------------------------------

#ifdef _DEBUG
VkResult CreateDebugUtilsMessengerEXT(const VkInstance _instance, const VkDebugUtilsMessengerCreateInfoEXT* _pCreateInfo, const VkAllocationCallbacks* _pAllocator, VkDebugUtilsMessengerEXT* _pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(_instance, _pCreateInfo, _pAllocator, _pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(const VkInstance _instance, const VkDebugUtilsMessengerEXT _debugMessenger, const VkAllocationCallbacks* _pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(_instance, _debugMessenger, _pAllocator);
	}
}

// Instance
bool VkApp::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	bool bLayerFound = false;
	for (const char* layerName : VALIDATION_LAYERS)
	{
		bLayerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				bLayerFound = true;
				break;
			}
		}

		if (bLayerFound == false)
		{
			goto end;
		}
	}

end:
	return bLayerFound;
}

void VkApp::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _out_rCreateInfo)
{
	_out_rCreateInfo = {};
	_out_rCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	_out_rCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	_out_rCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	_out_rCreateInfo.pfnUserCallback = debugCallback;
}

void VkApp::setupDebugMessenger()
{
#ifdef NDEBUG
	return;
#endif // NDEBUG

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(mVkInstance, &createInfo, nullptr, &mDebugMessenger) != VK_SUCCESS)
	{
		Error("App", "setupDebugMessenger", "failed to set up debug messenger!");
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkApp::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT _messageSeverity, VkDebugUtilsMessageTypeFlagsEXT _messageType, const VkDebugUtilsMessengerCallbackDataEXT* _pCallbackData, void* _pUserData)
{
	std::cerr << "validation layer: " << _pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

#endif