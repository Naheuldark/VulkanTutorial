#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <cstdlib>
#include <fstream>
#include <iostream>

#ifdef NDEBUG
constexpr bool VERBOSE                  = false;
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool VERBOSE                  = true;
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif


constexpr uint32_t WIDTH  = 800;
constexpr uint32_t HEIGHT = 600;

const std::vector VALIDATION_LAYERS          = {"VK_LAYER_KHRONOS_validation"};
const std::vector REQUIRED_DEVICE_EXTENSIONS = {vk::KHRSwapchainExtensionName};

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow *_window = nullptr;

    vk::raii::Context                _context;
    vk::raii::Instance               _instance           = nullptr;
    vk::raii::DebugUtilsMessengerEXT _debugMessenger     = nullptr;
    vk::raii::SurfaceKHR             _surface            = nullptr;
    vk::raii::PhysicalDevice         _physicalDevice     = nullptr;
    vk::raii::Device                 _logicalDevice      = nullptr;
    vk::raii::Queue                  _graphicsQueue      = nullptr;
    uint32_t                         _graphicsQueueIndex = UINT32_MAX;

    vk::raii::SwapchainKHR           _swapChain = nullptr;
    std::vector<vk::Image>           _swapChainImages;
    vk::SurfaceFormatKHR             _swapChainSurfaceFormat;
    vk::Extent2D                     _swapChainExtent;
    std::vector<vk::raii::ImageView> _swapChainImageViews;

    vk::raii::PipelineLayout _pipelineLayout   = nullptr;
    vk::raii::Pipeline       _graphicsPipeline = nullptr;

    vk::raii::CommandPool   _commandPool   = nullptr;
    vk::raii::CommandBuffer _commandBuffer = nullptr;

    vk::raii::Semaphore _presentCompleteSemaphore = nullptr;
    vk::raii::Semaphore _renderFinishedSemaphore  = nullptr;
    vk::raii::Fence     _drawFence                = nullptr;

private:
    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        _window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createGraphicsPipeline();
        createCommandPool();
        createCommandBuffer();
        createSyncObjects();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(_window)) {
            glfwPollEvents();
            drawFrame();
        }
        _logicalDevice.waitIdle();
    }

    void cleanup() const {
        glfwDestroyWindow(_window);

        glfwTerminate();
    }

private:
    void createInstance() {
        constexpr vk::ApplicationInfo appInfo{
                .pApplicationName   = "Hello Triangle",
                .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                .pEngineName        = "No Engine",
                .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
                .apiVersion         = vk::ApiVersion14
        };

        // Get the required layers
        std::vector<char const *> requiredLayers;
        if (ENABLE_VALIDATION_LAYERS) {
            requiredLayers.assign(VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
        }

        // Check if the required layers are supported by the Vulkan implementation
        const auto layerProperties    = _context.enumerateInstanceLayerProperties();
        const auto unsupportedLayerIt = std::ranges::find_if(requiredLayers, [&](auto &&requiredLayer) {
            return std::ranges::none_of(layerProperties, [requiredLayer](auto &&layerProperty) {
                return strcmp(layerProperty.layerName, requiredLayer) == 0;
            });
        });
        if (unsupportedLayerIt != requiredLayers.end()) {
            throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
        }

        if (VERBOSE) {
            // List all available layers
            std::cout << "Available layers:\n";
            for (auto &&layer: _context.enumerateInstanceLayerProperties()) {
                std::cout << '\t' << layer.layerName << '\n';
            }
        }

        // Get the required extensions
        auto requiredExtensions = getRequiredInstanceExtensions();

        // Check if the required extensions are supported by the Vulkan implementation
        const auto extensionProperties    = _context.enumerateInstanceExtensionProperties();
        const auto unsupportedExtensionIt = std::ranges::find_if(requiredExtensions, [&](auto &&requiredExtension) {
            return std::ranges::none_of(extensionProperties, [requiredExtension](auto &&extensionProperty) {
                return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
            });
        });
        if (unsupportedExtensionIt != requiredExtensions.end()) {
            throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedExtensionIt));
        }

        if (VERBOSE) {
            // List all available extensions
            std::cout << "Available extensions:\n";
            for (auto &[extensionName, specVersion]: _context.enumerateInstanceExtensionProperties()) {
                std::cout << '\t' << extensionName << '\n';
            }
        }

        const vk::InstanceCreateInfo instance{
                .pApplicationInfo        = &appInfo,
                .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
                .ppEnabledLayerNames     = requiredLayers.data(),
                .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
                .ppEnabledExtensionNames = requiredExtensions.data()
        };
        _instance = vk::raii::Instance(_context, instance);
    }

    void setupDebugMessenger() {
        if constexpr (!ENABLE_VALIDATION_LAYERS)
            return;

        constexpr vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                                      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        constexpr vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                                     vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                                     vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

        constexpr vk::DebugUtilsMessengerCreateInfoEXT debugMessenger{
                .messageSeverity = severityFlags,
                .messageType     = messageTypeFlags,
                .pfnUserCallback = &debugCallback
        };
        _debugMessenger = vk::raii::DebugUtilsMessengerEXT(_instance, debugMessenger);
    }

    void createSurface() {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(*_instance, _window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create a window surface!");
        }

        _surface = vk::raii::SurfaceKHR(_instance, surface);
    }

    void pickPhysicalDevice() {
        const auto physicalDevices  = _instance.enumeratePhysicalDevices();
        const auto physicalDeviceIt = std::ranges::find_if(physicalDevices, [&](auto &&physicalDevice) { return isDeviceSuitable(physicalDevice); });
        if (physicalDeviceIt == physicalDevices.end()) {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }

        _physicalDevice = *physicalDeviceIt;
    }

    void createLogicalDevice() {
        // Get the index of the first queue family that supports graphics and presentation
        const auto queueFamilies = _physicalDevice.getQueueFamilyProperties();

        for (uint32_t queueIdx = 0; queueIdx < queueFamilies.size(); queueIdx++) {
            if (queueFamilies[queueIdx].queueFlags & vk::QueueFlagBits::eGraphics && _physicalDevice.getSurfaceSupportKHR(queueIdx, *_surface)) {
                _graphicsQueueIndex = queueIdx;
                break;
            }
        }
        if (_graphicsQueueIndex == UINT32_MAX) {
            throw std::runtime_error("Failed to find a queue family supporting graphics and presentation!");
        }

        // Query for Vulkan 1.3 features
        vk::StructureChain<vk::PhysicalDeviceFeatures2,
                           vk::PhysicalDeviceVulkan11Features,
                           vk::PhysicalDeviceVulkan13Features,
                           vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
                featureChain = {
                        // vk::PhysicalDeviceFeatures2
                        {},
                        // vk::PhysicalDeviceVulkan11Features
                        {
                                .shaderDrawParameters = true
                        },
                        // vk::PhysicalDeviceVulkan13Features
                        {
                                .synchronization2 = true,
                                .dynamicRendering = true
                        },
                        // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
                        {
                                .extendedDynamicState = true
                        }
                };

        // Create a logical device
        constexpr float           queuePriority = 0.5f;
        vk::DeviceQueueCreateInfo graphicsQueue{
                .queueFamilyIndex = _graphicsQueueIndex,
                .queueCount       = 1,
                .pQueuePriorities = &queuePriority
        };
        vk::DeviceCreateInfo logicalDevice{
                .pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                .queueCreateInfoCount    = 1,
                .pQueueCreateInfos       = &graphicsQueue,
                .enabledExtensionCount   = static_cast<uint32_t>(REQUIRED_DEVICE_EXTENSIONS.size()),
                .ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data()
        };

        _logicalDevice = vk::raii::Device(_physicalDevice, logicalDevice);
        _graphicsQueue = vk::raii::Queue(_logicalDevice, _graphicsQueueIndex, 0);
    }

    void createSwapChain() {
        const auto surfaceCapabilities = _physicalDevice.getSurfaceCapabilitiesKHR(*_surface);
        _swapChainExtent               = chooseSwapChainExtent(surfaceCapabilities);
        const auto minImageCount       = chooseSwapChainMinImageCount(surfaceCapabilities);

        const auto availableFormats = _physicalDevice.getSurfaceFormatsKHR(*_surface);
        _swapChainSurfaceFormat     = chooseSwapChainSurfaceFormat(availableFormats);

        const auto availablePresentModes = _physicalDevice.getSurfacePresentModesKHR(*_surface);
        const auto presentMode           = chooseSwapChainPresentMode(availablePresentModes);

        const vk::SwapchainCreateInfoKHR swapchain{
                .surface          = *_surface,
                .minImageCount    = minImageCount,
                .imageFormat      = _swapChainSurfaceFormat.format,
                .imageColorSpace  = _swapChainSurfaceFormat.colorSpace,
                .imageExtent      = _swapChainExtent,
                .imageArrayLayers = 1,
                .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
                .imageSharingMode = vk::SharingMode::eExclusive,
                .preTransform     = surfaceCapabilities.currentTransform,
                .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                .presentMode      = presentMode,
                .clipped          = true
        };

        _swapChain       = vk::raii::SwapchainKHR(_logicalDevice, swapchain);
        _swapChainImages = _swapChain.getImages();
    }

    void createImageViews() {
        assert(_swapChainImageViews.empty());

        vk::ImageViewCreateInfo imageView{
                .viewType         = vk::ImageViewType::e2D,
                .format           = _swapChainSurfaceFormat.format,
                .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
        };

        for (auto &&image: _swapChainImages) {
            imageView.image = image;
            _swapChainImageViews.emplace_back(_logicalDevice, imageView);
        }
    }

    void createGraphicsPipeline() {
        const auto shaderModule = createShaderModule(readFile("shaders/slang.spv"));

        const vk::PipelineShaderStageCreateInfo vertexShaderStage{
                .stage  = vk::ShaderStageFlagBits::eVertex,
                .module = shaderModule,
                .pName  = "vertMain"
        };
        const vk::PipelineShaderStageCreateInfo fragmentShaderStage{
                .stage  = vk::ShaderStageFlagBits::eFragment,
                .module = shaderModule,
                .pName  = "fragMain"
        };
        const vk::PipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStage, fragmentShaderStage};

        vk::PipelineVertexInputStateCreateInfo   vertexInputState;
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{
                .topology = vk::PrimitiveTopology::eTriangleList
        };
        vk::PipelineViewportStateCreateInfo viewportState{
                .viewportCount = 1,
                .scissorCount  = 1
        };

        vk::PipelineRasterizationStateCreateInfo rasterizationState{
                .depthClampEnable        = vk::False,
                .rasterizerDiscardEnable = vk::False,
                .polygonMode             = vk::PolygonMode::eFill,
                .cullMode                = vk::CullModeFlagBits::eBack,
                .frontFace               = vk::FrontFace::eClockwise,
                .depthBiasEnable         = vk::False,
                .lineWidth               = 1.0f
        };

        vk::PipelineMultisampleStateCreateInfo multisampleState{
                .rasterizationSamples = vk::SampleCountFlagBits::e1,
                .sampleShadingEnable  = vk::False
        };

        vk::PipelineColorBlendAttachmentState colorBlendAttachmentState{
                .blendEnable    = vk::False,
                .colorWriteMask = vk::ColorComponentFlagBits::eR |
                                  vk::ColorComponentFlagBits::eG |
                                  vk::ColorComponentFlagBits::eB |
                                  vk::ColorComponentFlagBits::eA
        };
        vk::PipelineColorBlendStateCreateInfo colorBlendState{
                .logicOpEnable   = vk::False,
                .logicOp         = vk::LogicOp::eCopy,
                .attachmentCount = 1,
                .pAttachments    = &colorBlendAttachmentState
        };

        const std::vector dynamicStates = {
                vk::DynamicState::eViewport,
                vk::DynamicState::eScissor
        };
        vk::PipelineDynamicStateCreateInfo dynamicState{
                .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                .pDynamicStates    = dynamicStates.data()
        };

        vk::PipelineLayoutCreateInfo pipelineLayout{
                .setLayoutCount         = 0,
                .pushConstantRangeCount = 0
        };
        _pipelineLayout = vk::raii::PipelineLayout(_logicalDevice, pipelineLayout);

        vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineChain = {
                {
                        .stageCount          = 2,
                        .pStages             = shaderStages,
                        .pVertexInputState   = &vertexInputState,
                        .pInputAssemblyState = &inputAssemblyState,
                        .pViewportState      = &viewportState,
                        .pRasterizationState = &rasterizationState,
                        .pMultisampleState   = &multisampleState,
                        .pColorBlendState    = &colorBlendState,
                        .pDynamicState       = &dynamicState,
                        .layout              = _pipelineLayout,
                        .renderPass          = nullptr
                },
                {
                        .colorAttachmentCount    = 1,
                        .pColorAttachmentFormats = &_swapChainSurfaceFormat.format
                }
        };

        _graphicsPipeline = vk::raii::Pipeline(_logicalDevice, nullptr, pipelineChain.get<vk::GraphicsPipelineCreateInfo>());
    }

    void createCommandPool() {
        const vk::CommandPoolCreateInfo commandPool{
                .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                .queueFamilyIndex = _graphicsQueueIndex
        };
        _commandPool = vk::raii::CommandPool(_logicalDevice, commandPool);
    }

    void createCommandBuffer() {
        const vk::CommandBufferAllocateInfo commandBuffer{
                .commandPool        = _commandPool,
                .level              = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = 1
        };
        _commandBuffer = std::move(vk::raii::CommandBuffers(_logicalDevice, commandBuffer).front());
    }

    void createSyncObjects() {
        _presentCompleteSemaphore = vk::raii::Semaphore(_logicalDevice, vk::SemaphoreCreateInfo());
        _renderFinishedSemaphore  = vk::raii::Semaphore(_logicalDevice, vk::SemaphoreCreateInfo());
        _drawFence                = vk::raii::Fence(_logicalDevice, {.flags = vk::FenceCreateFlagBits::eSignaled});
    }

    void drawFrame() {
        if (_logicalDevice.waitForFences(*_drawFence, vk::True, UINT64_MAX) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to wait for fence!");
        }
        _logicalDevice.resetFences(*_drawFence);

        auto [result, imageIndex] = _swapChain.acquireNextImage(UINT64_MAX, *_presentCompleteSemaphore, nullptr);

        recordCommandBuffer(imageIndex);

        _graphicsQueue.waitIdle(); // NOTE: for simplicity, wait for the queue to be idle before starting the frame
        // In the next chapter you see how to use multiple frames in flight and fences to sync

        vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        const vk::SubmitInfo   submitInfo{
                .waitSemaphoreCount   = 1,
                .pWaitSemaphores      = &*_presentCompleteSemaphore,
                .pWaitDstStageMask    = &waitDestinationStageMask,
                .commandBufferCount   = 1,
                .pCommandBuffers      = &*_commandBuffer,
                .signalSemaphoreCount = 1,
                .pSignalSemaphores    = &*_renderFinishedSemaphore
        };
        _graphicsQueue.submit(submitInfo, *_drawFence);

        const vk::PresentInfoKHR presentInfoKHR{
                .waitSemaphoreCount = 1,
                .pWaitSemaphores    = &*_renderFinishedSemaphore,
                .swapchainCount     = 1,
                .pSwapchains        = &*_swapChain,
                .pImageIndices      = &imageIndex
        };
        result = _graphicsQueue.presentKHR(presentInfoKHR);

        switch (result) {
            case vk::Result::eSuccess:
                break;
            case vk::Result::eSuboptimalKHR:
                std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n";
                break;
            default:
                break; // an unexpected result is returned!
        }
    }

private:
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                          const vk::DebugUtilsMessageTypeFlagsEXT       type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                          void *                                        pUserData) {
        std::cerr << "Validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
        return vk::False;
    }

    static std::vector<const char *> getRequiredInstanceExtensions() {
        uint32_t   glfwExtensionCount = 0;
        const auto glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (ENABLE_VALIDATION_LAYERS) {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

        return extensions;
    }

    static bool isDeviceSuitable(const vk::raii::PhysicalDevice &physicalDevice) {
        // Check if the physical device supports the Vulkan 1.3 API version
        bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

        // Check if any of the queue families support graphics operations
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();
        bool supportGraphics = std::ranges::any_of(queueFamilies, [](auto &&queue) { return !!(queue.queueFlags & vk::QueueFlagBits::eGraphics); });

        // Check if all required physical device extensions are available
        auto availableDeviceExtensions     = physicalDevice.enumerateDeviceExtensionProperties();
        bool supportsAllRequiredExtensions = std::ranges::all_of(REQUIRED_DEVICE_EXTENSIONS, [&](auto &&requiredDeviceExtension) {
            return std::ranges::any_of(availableDeviceExtensions, [requiredDeviceExtension](auto &&availableDeviceExtension) {
                return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
            });
        });

        // Check if the physical device supports the required features
        const auto availableDeviceFeatures = physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2,
                                                                         vk::PhysicalDeviceVulkan11Features,
                                                                         vk::PhysicalDeviceVulkan13Features,
                                                                         vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool supportRequiredFeatures = availableDeviceFeatures.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
                                       availableDeviceFeatures.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                       availableDeviceFeatures.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

        // Return true if the physical devices meets all the criteria
        return supportsVulkan1_3 && supportGraphics && supportsAllRequiredExtensions && supportRequiredFeatures;
    }

    [[nodiscard]] vk::Extent2D chooseSwapChainExtent(const vk::SurfaceCapabilitiesKHR &surfaceCapabilities) const {
        if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
            return surfaceCapabilities.currentExtent;
        }

        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);

        return {
                std::clamp<uint32_t>(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
                std::clamp<uint32_t>(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
        };
    }

    static uint32_t chooseSwapChainMinImageCount(const vk::SurfaceCapabilitiesKHR &surfaceCapabilities) {
        auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
        if (0 < surfaceCapabilities.maxImageCount && surfaceCapabilities.maxImageCount < minImageCount) {
            minImageCount = surfaceCapabilities.maxImageCount;
        }
        return minImageCount;
    }

    static vk::SurfaceFormatKHR chooseSwapChainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
        assert(!availableFormats.empty());
        const auto formatIt = std::ranges::find_if(availableFormats, [](auto &&format) {
            return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
        return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
    }

    static vk::PresentModeKHR chooseSwapChainPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes) {
        assert(std::ranges::any_of(availablePresentModes, [](auto &&presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
        return std::ranges::any_of(availablePresentModes, [](auto &&presentMode) { return presentMode == vk::PresentModeKHR::eMailbox; })
                   ? vk::PresentModeKHR::eMailbox
                   : vk::PresentModeKHR::eFifo;
    }

    static std::vector<char> readFile(const std::string &filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        std::vector<char> buffer(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        file.close();
        return buffer;
    }

    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char> &code) const {
        const vk::ShaderModuleCreateInfo shaderModule{
                .codeSize = code.size() * sizeof(char),
                .pCode    = reinterpret_cast<const uint32_t *>(code.data())
        };

        return {_logicalDevice, shaderModule};
    }

    void recordCommandBuffer(const uint32_t imageIndex) const {
        _commandBuffer.begin({});

        // Before starting rendering, transition the swapchain image to vk::ImageLayout::eColorAttachmentOptimal
        transitionImageLayout(
                imageIndex,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal,
                {},                                                 // srcAccessMask (no need to wait for previous operations)
                vk::AccessFlagBits2::eColorAttachmentWrite,         // dstAccessMask
                vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
                vk::PipelineStageFlagBits2::eColorAttachmentOutput  // dstStage
                );

        constexpr vk::ClearValue    clearColor     = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        vk::RenderingAttachmentInfo attachmentInfo = {
                .imageView   = _swapChainImageViews[imageIndex],
                .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                .loadOp      = vk::AttachmentLoadOp::eClear,
                .storeOp     = vk::AttachmentStoreOp::eStore,
                .clearValue  = clearColor};
        const vk::RenderingInfo renderingInfo = {
                .renderArea = {
                        .offset = {0, 0},
                        .extent = _swapChainExtent
                },
                .layerCount           = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments    = &attachmentInfo};

        _commandBuffer.beginRendering(renderingInfo);
        {
            _commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_graphicsPipeline);
            _commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f,
                                                       static_cast<float>(_swapChainExtent.width), static_cast<float>(_swapChainExtent.height),
                                                       0.0f, 1.0f));
            _commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _swapChainExtent));
            _commandBuffer.draw(3, 1, 0, 0);
        }
        _commandBuffer.endRendering();

        // After rendering, transition the swapchain image to vk::ImageLayout::ePresentSrcKHR
        transitionImageLayout(
                imageIndex,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::ePresentSrcKHR,
                vk::AccessFlagBits2::eColorAttachmentWrite,         // srcAccessMask
                {},                                                 // dstAccessMask
                vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
                vk::PipelineStageFlagBits2::eBottomOfPipe           // dstStage
                );

        _commandBuffer.end();
    }

    void transitionImageLayout(const uint32_t                imageIndex,
                               const vk::ImageLayout         oldLayout,
                               const vk::ImageLayout         newLayout,
                               const vk::AccessFlags2        srcAccessMask,
                               const vk::AccessFlags2        dstAccessMask,
                               const vk::PipelineStageFlags2 srcStageMask,
                               const vk::PipelineStageFlags2 dstStageMask) const {
        vk::ImageMemoryBarrier2 barrier = {
                .srcStageMask        = srcStageMask,
                .srcAccessMask       = srcAccessMask,
                .dstStageMask        = dstStageMask,
                .dstAccessMask       = dstAccessMask,
                .oldLayout           = oldLayout,
                .newLayout           = newLayout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = _swapChainImages[imageIndex],
                .subresourceRange    = {
                        .aspectMask     = vk::ImageAspectFlagBits::eColor,
                        .baseMipLevel   = 0,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1
                }
        };

        const vk::DependencyInfo dependency_info = {
                .dependencyFlags         = {},
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers    = &barrier
        };

        _commandBuffer.pipelineBarrier2(dependency_info);
    }
};

int main() {
    try {
        HelloTriangleApplication app;
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
