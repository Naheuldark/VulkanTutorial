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
    GLFWwindow *                     _window         = nullptr;
    vk::raii::DebugUtilsMessengerEXT _debugMessenger = nullptr;
    vk::raii::Context                _context;
    vk::raii::Instance               _instance       = nullptr;
    vk::raii::SurfaceKHR             _surface        = nullptr;
    vk::raii::PhysicalDevice         _physicalDevice = nullptr;
    vk::raii::Device                 _logicalDevice  = nullptr;
    vk::raii::Queue                  _graphicsQueue  = nullptr;
    vk::raii::SwapchainKHR           _swapChain      = nullptr;
    std::vector<vk::Image>           _swapChainImages;
    vk::SurfaceFormatKHR             _swapChainSurfaceFormat;
    vk::Extent2D                     _swapChainExtent;
    std::vector<vk::raii::ImageView> _swapChainImageViews;
    vk::raii::PipelineLayout         _pipelineLayout = nullptr;

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
    }

    void mainLoop() const {
        while (!glfwWindowShouldClose(_window)) {
            glfwPollEvents();
        }
    }

    void cleanup() const {
        glfwDestroyWindow(_window);

        glfwTerminate();
    }

private:
    /**
     * @brief Create a Vulkan instance with the required application info, layers, and extensions.
     *        This function checks for the availability of required layers and extensions, and throws an exception if any of them are not supported by the Vulkan implementation.
     *        If validation layers are enabled, it also sets up a debug messenger to receive validation messages.
     *        The created instance is stored in the _instance member variable.
     */
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

    /**
     * @brief Set up a debug messenger to receive validation messages from the Vulkan implementation.
     *        This function creates a vk::DebugUtilsMessengerEXT object and stores it in the _debugMessenger member variable.
     *        The debug messenger is only created if validation layers are enabled.
     *        The debug callback function is defined as a static member function of the HelloTriangleApplication class.
     */
    void setupDebugMessenger() {
        if constexpr (!ENABLE_VALIDATION_LAYERS) return;

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
        _debugMessenger = _instance.createDebugUtilsMessengerEXT(debugMessenger);
    }

    /**
     * @brief Create a Vulkan surface for the GLFW window.
     *        This function uses the glfwCreateWindowSurface function to create a VkSurfaceKHR object for the GLFW window and stores it in the _surface member variable.
     *        If the surface creation fails, an exception is thrown.
     *        The created surface is used for presenting rendered images to the window.
     */
    void createSurface() {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(*_instance, _window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create a window surface!");
        }

        _surface = vk::raii::SurfaceKHR(_instance, surface);
    }

    /**
     * @brief Pick a suitable physical device (GPU) for the Vulkan application.
     *        This function enumerates the available physical devices and checks if any of them meet the required criteria for suitability.
     *        The suitability criteria include support for Vulkan 1.3, support for graphics operations, availability of required device extensions, and support for required features.
     */
    void pickPhysicalDevice() {
        const auto physicalDevices  = _instance.enumeratePhysicalDevices();
        const auto physicalDeviceIt = std::ranges::find_if(physicalDevices, [&](auto &&physicalDevice) { return isDeviceSuitable(physicalDevice); });
        if (physicalDeviceIt == physicalDevices.end()) {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }

        _physicalDevice = *physicalDeviceIt;
    }

    /**
     * @brief Create a logical device from the selected physical device.
     *        This function finds a queue family that supports both graphics and presentation operations, and creates a logical device with a graphics queue.
     *        It also queries for Vulkan 1.3 features and enables the required extensions for the logical device.
     *        The created logical device is stored in the _logicalDevice member variable, and the graphics queue is stored in the _graphicsQueue member variable.
     *        If no suitable queue family is found, an exception is thrown.
     */
    void createLogicalDevice() {
        // Get the index of the first queue family that supports graphics and presentation
        const auto queueFamilies = _physicalDevice.getQueueFamilyProperties();

        uint32_t compliantQueueFamilyIdx = UINT32_MAX;
        for (uint32_t queueIdx = 0; queueIdx < queueFamilies.size(); queueIdx++) {
            if (queueFamilies[queueIdx].queueFlags & vk::QueueFlagBits::eGraphics && _physicalDevice.getSurfaceSupportKHR(queueIdx, *_surface)) {
                compliantQueueFamilyIdx = queueIdx;
                break;
            }
        }
        if (compliantQueueFamilyIdx == UINT32_MAX) {
            throw std::runtime_error("Failed to find a queue family supporting graphics and presentation!");
        }

        // Query for Vulkan 1.3 features
        vk::StructureChain<vk::PhysicalDeviceFeatures2,
                           vk::PhysicalDeviceVulkan11Features,
                           vk::PhysicalDeviceVulkan13Features,
                           vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
                featureChain = {
                    {},                             // vk::PhysicalDeviceFeatures2
                    {.shaderDrawParameters = true}, // vk::PhysicalDeviceVulkan11Features
                    {.dynamicRendering = true},     // vk::PhysicalDeviceVulkan13Features
                    {.extendedDynamicState = true}  // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
                };

        // Create a logical device
        constexpr float           queuePriority = 0.5f;
        vk::DeviceQueueCreateInfo graphicsQueue{
            .queueFamilyIndex = compliantQueueFamilyIdx,
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
        _graphicsQueue = vk::raii::Queue(_logicalDevice, compliantQueueFamilyIdx, 0);
    }

    /**
     * @brief Create a swap chain for the logical device and surface.
     *        This function queries the surface capabilities, available formats, and present modes, and chooses suitable values for the swap chain creation.
     *        It then creates a vk::SwapchainKHR object and stores it in the _swapChain member variable, along with the swap chain images in the _swapChainImages member variable.
     *        The swap chain is used for presenting rendered images to the window, and the images are used as color attachments in the rendering pipeline.
     *        If the swap chain creation fails, an exception is thrown.
     */
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

    /**
     * @brief Create image views for the swap chain images.
     *        This function creates a vk::ImageView for each swap chain image, using the chosen surface format and color aspect.
     *        The created image views are stored in the _swapChainImageViews member variable, and are used as color attachments in the rendering pipeline.
     */
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
        const std::vector shaderStages = {vertexShaderStage, fragmentShaderStage};

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
    }

private:
    /**
     * @brief Debug callback function for Vulkan validation layers.
     *        This function is called by the Vulkan implementation when a validation message is generated.
     *        It prints the message type and message content to the standard error stream.
     * @param severity flags indicating the severity of the message (e.g., warning, error)
     * @param type flags indicating the type of the message (e.g., general, performance, validation)
     * @param pCallbackData pointer to a structure containing the message data, including the message string
     * @param pUserData pointer to user-defined data passed to the callback
     * @return vk::Bool32 indicating whether the Vulkan call that triggered the validation message should be aborted (vk::False means continue execution)
     */
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                          const vk::DebugUtilsMessageTypeFlagsEXT       type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                          void *                                        pUserData) {
        std::cerr << "Validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
        return vk::False;
    }

    /**
     * @brief Get the required Vulkan instance extensions for GLFW and validation layers (if enabled).
     *        This function queries GLFW for the required instance extensions and adds the VK_EXT_debug_utils extension if validation layers are enabled.
     * @return A vector of C-style strings containing the names of the required instance extensions.
     */
    static std::vector<const char *> getRequiredInstanceExtensions() {
        uint32_t   glfwExtensionCount = 0;
        const auto glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (ENABLE_VALIDATION_LAYERS) {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

        return extensions;
    }

    /**
     * @brief Check if a physical device is suitable for the Vulkan application.
     *        This function checks if the physical device supports Vulkan 1.3, has a queue family that supports graphics operations, has all required device extensions, and supports the required features.
     * @param physicalDevice The physical device to check for suitability.
     * @return true if the physical device meets all the criteria for suitability, false otherwise.
     */
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

    /**
     * @brief Choose the swap chain extent (resolution) based on the surface capabilities and the current framebuffer size of the GLFW window.
     *        If the surface capabilities specify a current extent (not equal to UINT32_MAX), it is returned as the swap chain extent.
     *        Otherwise, the current framebuffer size of the GLFW window is queried and clamped to the minimum and maximum extents specified by the surface capabilities.
     * @param surfaceCapabilities The surface capabilities of the physical device and surface, which include the minimum and maximum extents for the swap chain.
     * @return The chosen swap chain extent as a vk::Extent2D object, which represents the width and height of the swap chain images.
     */
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

    /**
     * @brief Choose the minimum number of images for the swap chain based on the surface capabilities.
     *        The minimum number of images is determined by taking the maximum of 3 and the minimum image count specified in the surface capabilities.
     *        If the maximum image count is greater than 0 and less than the calculated minimum image count, the maximum image count is used instead.
     * @param surfaceCapabilities The surface capabilities of the physical device and surface, which include the minimum and maximum image counts for the swap chain.
     * @return The chosen minimum number of images for the swap chain as a uint32_t value.
     */
    static uint32_t chooseSwapChainMinImageCount(const vk::SurfaceCapabilitiesKHR &surfaceCapabilities) {
        auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
        if (0 < surfaceCapabilities.maxImageCount && surfaceCapabilities.maxImageCount < minImageCount) {
            minImageCount = surfaceCapabilities.maxImageCount;
        }
        return minImageCount;
    }

    /**
     * @brief Choose the swap chain surface format from the available formats.
     *        This function searches for a preferred format (vk::Format::eB8G8R8A8Srgb with vk::ColorSpaceKHR::eSrgbNonlinear) in the list of available formats.
     *        If the preferred format is found, it is returned; otherwise, the first available format is returned.
     * @param availableFormats A vector of vk::SurfaceFormatKHR objects representing the available surface formats for the swap chain.
     * @return The chosen swap chain surface format as a vk::SurfaceFormatKHR object, which includes the format and color space for the swap chain images.
     */
    static vk::SurfaceFormatKHR chooseSwapChainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
        assert(!availableFormats.empty());
        const auto formatIt = std::ranges::find_if(availableFormats, [](auto &&format) {
            return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
        return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
    }

    /**
     * @brief Choose the swap chain present mode from the available present modes.
     *        This function checks if the preferred present mode (vk::PresentModeKHR::eMailbox) is available in the list of available present modes.
     *        If the preferred present mode is found, it is returned; otherwise, the fallback present mode (vk::PresentModeKHR::eFifo) is returned.
     *        The function asserts that the fallback present mode is always available, as it is guaranteed to be supported by all Vulkan implementations.
     * @param availablePresentModes A vector of vk::PresentModeKHR values representing the available present modes for the swap chain.
     * @return The chosen swap chain present mode as a vk::PresentModeKHR value, which determines how images are presented to the surface (e.g., mailbox, fifo, immediate).
     */
    static vk::PresentModeKHR chooseSwapChainPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes) {
        assert(std::ranges::any_of(availablePresentModes, [](auto &&presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
        return std::ranges::any_of(availablePresentModes, [](auto &&presentMode) { return presentMode == vk::PresentModeKHR::eMailbox; })
                   ? vk::PresentModeKHR::eMailbox
                   : vk::PresentModeKHR::eFifo;
    }

    /**
     * @brief Read the contents of a file into a vector of characters.
     *        This function opens the specified file in binary mode, reads its contents into a vector of characters, and returns the vector.
     *        If the file cannot be opened, an exception is thrown.
     * @param filename The name of the file to read.
     * @return A vector of characters containing the contents of the file.
     */
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

    /**
     * @brief Create a Vulkan shader module from SPIR-V bytecode.
     *        This function takes a vector of characters containing the SPIR-V bytecode for the shader module, creates a vk::ShaderModuleCreateInfo structure with the appropriate parameters, and creates a vk::raii::ShaderModule object using the logical device.
     *        The created shader module is returned to the caller. If the shader module creation fails, an exception is thrown.
     * @param code A vector of characters containing the SPIR-V bytecode for the shader module.
     * @return A vk::raii::ShaderModule object representing the created shader module.
     */
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char> &code) const {
        const vk::ShaderModuleCreateInfo shaderModule{
            .codeSize = code.size() * sizeof(char),
            .pCode    = reinterpret_cast<const uint32_t *>(code.data())
        };

        return {_logicalDevice, shaderModule};
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
