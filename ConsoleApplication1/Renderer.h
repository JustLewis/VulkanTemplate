#define GLFW_INCLUDE_VULKAN
#define GL_KHR_vulkan_glsl = true
//#define VK_USE_PLATFORM_WIN64_KHR
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <optional>

#include <vector>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool IsComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR> PresentModes;
};

class Renderer {
public:
    void run();

private:

    GLFWwindow* Window;
    const uint32_t Width = 1920, Height = 1080;

    const int MAX_FRAMES_IN_FLIGHT = 3;
    uint32_t CurrentFrame = 0;

    VkInstance Instance;
    VkDebugUtilsMessengerEXT DebugMessenger;
    VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
    VkDevice LogicalDevice = VK_NULL_HANDLE;
    VkQueue GraphicsQueue = VK_NULL_HANDLE;
    VkSurfaceKHR VKSurface = VK_NULL_HANDLE;
    VkQueue PresentQueue;
    VkSwapchainKHR SwapChain;
    
    std::vector<VkImage> SwapChainImages;
    VkFormat SwapChainImageFormat;
    VkExtent2D SwapChainExtent;
    std::vector<VkImageView> SwapChainImageViews;

    VkRenderPass RenderPass;
    VkPipelineLayout PipelineLayout;
    VkPipeline GraphicsPipeline;
    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;
    std::vector<VkSemaphore> ImageAvailableSemaphores;
    std::vector<VkSemaphore> RenderFinishedSemaphores;
    std::vector<VkFence> InFlightFences;

    std::vector<VkFramebuffer> SwapChainFramebuffers;

    const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef _DEBUG
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif

    void initWindow();

    void initVulkan();

    void CreateSurface();

    void CreateVKInstance();
    void CreateVKDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffer();
    void CreateSyncObjects();

    void DrawFrame();

    void RecordBufferCommand(VkCommandBuffer CommandBufferIn, uint32_t ImageIndex);

    VkShaderModule CreateShaderModule(const std::vector<char>& code);

    void mainLoop();

    bool checkValidationSupport();

    bool IsPhysicalDeviceSuitable(VkPhysicalDevice DeviceIn);

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice DeviceIn);

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice Device);

    void cleanup();

    std::vector<const char*> GetRequiredExtensions();

    bool CheckDeviceExtensionSupport(VkPhysicalDevice DeviceIn);

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& AvailableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& AvailableModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& AvailableExtents);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT MessageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};