#include "Renderer.h"
#include <format>
#include <cstdlib>
#include <set>
#include <algorithm>
#include <fstream>


#define LOG_COLOUR_RESET   "\033[0m"
#define LOG_COLOUR_RED     "\033[31m"      /* Red */
#define LOG_COLOUR_YELLOW  "\033[33m"      /* Yellow */
#define LOG_COLOUR_MAGENTA "\033[35m"      /* Magenta */
#define LOG_COLOUR_CYAN    "\033[36m"      /* Cyan */

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to find file [ {} ]", filename));
    }

    size_t filesize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(filesize);

    file.seekg(0);
    file.read(buffer.data(), filesize);

    file.close();

    return buffer;
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void Renderer::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void Renderer::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    Window = glfwCreateWindow(Width, Height, "MyVulkan", nullptr, nullptr);
}

void Renderer::initVulkan() {

    createVKInstance();
    CreateVKDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffer();
    CreateSyncObjects();
}

void Renderer::CreateSurface() {
    if (glfwCreateWindowSurface(Instance,Window,nullptr,&VKSurface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VK window surface");
    }
}

void Renderer::createVKInstance() {
    if (enableValidationLayers && !checkValidationSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo AppInfo{};

    AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName = "My Lovely Vulkan tutorial triangle";
    AppInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    AppInfo.pEngineName = "No Engine";
    AppInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    AppInfo.apiVersion = VK_API_VERSION_1_4; // TRY Version 1.0 if errors.

    VkInstanceCreateInfo CreateInfo{};
    CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CreateInfo.pApplicationInfo = &AppInfo;

    //uint32_t glfw_ExtentionsCount = 0;
    //const char** glfw_Extentions;
    //glfw_Extentions = glfwGetRequiredInstanceExtensions(&glfw_ExtentionsCount);
    //CreateInfo.enabledExtensionCount = glfw_ExtentionsCount;
    //CreateInfo.ppEnabledExtensionNames = glfw_Extentions;

    //Debug extensions if in debug mode.
    std::vector<const char*> extensions = GetRequiredExtensions();
    CreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    CreateInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        CreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        CreateInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        CreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        CreateInfo.enabledLayerCount = 0;
    }

    if (VkResult result = vkCreateInstance(&CreateInfo, nullptr, &Instance); result != VK_SUCCESS)
    {
        throw std::runtime_error(
            std::format("Failed to create instance. Error {}", static_cast<int>(result))
        );

    }

#if _DEBUG //Check extensions available.

    uint32_t extensionCount = 0;
    std::vector<VkExtensionProperties> ExtensionsAvailable(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << "available extensions:\n";

    for (const auto& extension : ExtensionsAvailable) {
        std::cout << '\t' << extension.extensionName << '\n';
    }

    //Finish inisialisation message
    std::cout << "Initialisation okay \n";
#endif

}

void Renderer::CreateVKDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(Instance, &createInfo, nullptr, &DebugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }

}

void Renderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT; // Noice and verbose messaging please.
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional
}

void Renderer::PickPhysicalDevice() {

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find a physical device for renderer");
    }

    std::vector<VkPhysicalDevice> Devices(deviceCount);
    vkEnumeratePhysicalDevices(Instance, &deviceCount, Devices.data());

    for (auto& Device : Devices) {
        if (IsPhysicalDeviceSuitable(Device)) {
            PhysicalDevice = Device;
        }
    }
    if (PhysicalDevice == VK_NULL_HANDLE) {
        throw("Failed to find suitable device");
    }
}

void Renderer::CreateLogicalDevice() {
    QueueFamilyIndices indices = FindQueueFamilies(PhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> QueueFamilyCreateInfos;
    std::set<uint32_t> UniqueQueueFamilies = { 
        indices.graphicsFamily.value(),
        indices.presentFamily.value() 
    };
    
    float QueuePriority = 1.0f;
    for (uint32_t QueueFamily : UniqueQueueFamilies) {
        VkDeviceQueueCreateInfo QueueCreateInfo{};
        QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfo.queueFamilyIndex = QueueFamily;
        QueueCreateInfo.queueCount = 1;
        float QueuePriority = 1.f;
        QueueCreateInfo.pQueuePriorities = &QueuePriority;
        QueueFamilyCreateInfos.push_back(QueueCreateInfo);
    }
    

    VkPhysicalDeviceFeatures deviceFeatures{};


    VkDeviceCreateInfo CreateInfo{};
    CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    CreateInfo.pQueueCreateInfos = QueueFamilyCreateInfos.data();
    CreateInfo.queueCreateInfoCount = 1;
    CreateInfo.pEnabledFeatures = &deviceFeatures;
    
    CreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
    CreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

    //Causes error because this is deprecated. Tutorial recommends this but it's incorrect. https://stackoverflow.com/questions/13280895/how-can-i-use-colors-in-my-console-app-c
    //if (enableValidationLayers) {
    //    CreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    //    CreateInfo.ppEnabledLayerNames = validationLayers.data();
    //}
    //else {
    //    CreateInfo.enabledLayerCount = 0;
    //}

    if (vkCreateDevice(PhysicalDevice, &CreateInfo, nullptr, &LogicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(LogicalDevice, indices.graphicsFamily.value(), 0, &GraphicsQueue);
    vkGetDeviceQueue(LogicalDevice, indices.presentFamily.value(), 0, &PresentQueue);

}

void Renderer::CreateSwapChain() {
    SwapChainSupportDetails Details = QuerySwapChainSupport(PhysicalDevice);
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(Details.Formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(Details.PresentModes);
    VkExtent2D Extent = ChooseSwapExtent(Details.Capabilities);

    uint32_t SwapChainImageCount = std::min(Details.Capabilities.minImageCount + 2,Details.Capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR CreateInfo{};
    CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    CreateInfo.surface = VKSurface;
    
    CreateInfo.minImageCount = SwapChainImageCount;
    CreateInfo.imageFormat = surfaceFormat.format;
    CreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    CreateInfo.imageExtent = Extent;
    CreateInfo.imageArrayLayers = 1;
    CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    CreateInfo.presentMode = presentMode; // Is this missing? https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain

    QueueFamilyIndices indices = FindQueueFamilies(PhysicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        CreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        CreateInfo.queueFamilyIndexCount = 2;
        CreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        CreateInfo.queueFamilyIndexCount = 0; // Optional
        CreateInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    CreateInfo.preTransform = Details.Capabilities.currentTransform;
    CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    CreateInfo.presentMode = presentMode;
    CreateInfo.clipped = VK_TRUE;
    CreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(LogicalDevice, &CreateInfo, nullptr, &SwapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    
    vkGetSwapchainImagesKHR(LogicalDevice, SwapChain, &SwapChainImageCount, nullptr);
    SwapChainImages.resize(SwapChainImageCount);
    vkGetSwapchainImagesKHR(LogicalDevice, SwapChain, &SwapChainImageCount, SwapChainImages.data());
    
    SwapChainImageFormat = surfaceFormat.format;
    SwapChainExtent = Extent;

}

void Renderer::CreateImageViews() {

    SwapChainImageViews.resize(SwapChainImages.size());

    for (size_t i = 0; i < SwapChainImages.size(); i++) {

        VkImageViewCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        CreateInfo.image = SwapChainImages[i];

        CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        CreateInfo.format = SwapChainImageFormat;

        CreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        CreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        CreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        CreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        CreateInfo.subresourceRange.baseMipLevel = 0;
        CreateInfo.subresourceRange.levelCount = 1;
        CreateInfo.subresourceRange.baseArrayLayer = 0;
        CreateInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(LogicalDevice, &CreateInfo, nullptr, &SwapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error(std::format("Failed to create image view {}",i));
        }
    }

    
}

void Renderer::CreateRenderPass() {

    VkAttachmentDescription ColourAttachmentDesc{};
    ColourAttachmentDesc.format = SwapChainImageFormat;
    ColourAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    ColourAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColourAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ColourAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColourAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ColourAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ColourAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference ColourAttachmentRef{};
    ColourAttachmentRef.attachment = 0;
    ColourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription SubpassDesc{};
    SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SubpassDesc.colorAttachmentCount = 1;
    SubpassDesc.pColorAttachments = &ColourAttachmentRef;

    VkSubpassDependency Dependency{};
    Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    Dependency.dstStageMask = 0;

    Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    Dependency.srcAccessMask = 0;

    Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo RenderPassInfo{};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = 1;
    RenderPassInfo.pAttachments = &ColourAttachmentDesc;
    RenderPassInfo.subpassCount = 1;
    RenderPassInfo.pSubpasses = &SubpassDesc;
    RenderPassInfo.dependencyCount = 1;
    RenderPassInfo.pDependencies = &Dependency;

    if(vkCreateRenderPass(LogicalDevice,&RenderPassInfo,nullptr,&RenderPass) != VK_SUCCESS){
        throw std::runtime_error("Failed to create render pass");
    }





}

void Renderer::CreateGraphicsPipeline() {
    auto vertShaderCode = readFile("vert.spv");
    auto fragShaderCode = readFile("frag.spv");

    VkShaderModule VertShaderModule = CreateShaderModule(vertShaderCode);
    VkShaderModule FragShaderModule = CreateShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo VertShaderStageInfo = {};
    VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    VertShaderStageInfo.module = VertShaderModule;
    VertShaderStageInfo.pName = "main";
    VertShaderStageInfo.pSpecializationInfo = nullptr; //Might want this later.

    VkPipelineShaderStageCreateInfo FragShaderStageInfo = {};
    FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    FragShaderStageInfo.module = FragShaderModule;
    FragShaderStageInfo.pName = "main";
    FragShaderStageInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo VertexInputInfo{};
    VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputInfo.vertexBindingDescriptionCount = 0;
    VertexInputInfo.pVertexBindingDescriptions = nullptr;
    VertexInputInfo.vertexBindingDescriptionCount = 0;
    VertexInputInfo.pVertexBindingDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo{};
    InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport Viewport{};
    Viewport.x = 0.f;
    Viewport.y = 0.f;
    Viewport.height = static_cast<float>(SwapChainExtent.height);
    Viewport.width = static_cast<float>(SwapChainExtent.width);
    Viewport.minDepth = 0.f;
    Viewport.maxDepth = 1.0f;

    VkRect2D Scissor = {};
    Scissor.offset = { 0,0 };
    Scissor.extent = SwapChainExtent;

    std::vector<VkDynamicState> DynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo DynamicStateInfo{};
    DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
    DynamicStateInfo.pDynamicStates = DynamicStates.data();

    VkPipelineViewportStateCreateInfo ViewportInfo{};
    ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportInfo.viewportCount = 1;
    ViewportInfo.pViewports = &Viewport;
    ViewportInfo.scissorCount = 1;
    ViewportInfo.pScissors = &Scissor;


    VkPipelineRasterizationStateCreateInfo Rasterizer{};
    Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    Rasterizer.depthClampEnable = VK_FALSE;
    Rasterizer.rasterizerDiscardEnable = VK_FALSE;
    Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    Rasterizer.lineWidth = 1.0f;
    Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    Rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    Rasterizer.depthBiasEnable = VK_FALSE;
    /*  Rasterizer.depthBiasConstantFactor = 0.f; //Not needed because depth bias enabled is false.
      Rasterizer.depthBiasClamp = 0.f;
      Rasterizer.depthBiasSlopeFactor = 0.f;*/

    VkPipelineMultisampleStateCreateInfo MultisamplingInfo{};
    MultisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MultisamplingInfo.sampleShadingEnable = VK_FALSE;
    MultisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

      //Colour blending between frames or blending pixels etc. I dunno. It's a bit mad 
      //See "color blending" part https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
    VkPipelineColorBlendAttachmentState ColourBlendAttachment{};
    ColourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColourBlendAttachment.blendEnable = VK_TRUE;
    ColourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    ColourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    ColourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    ColourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    ColourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    ColourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    //Just copy pasting these two structs. This function is so massive...
    VkPipelineColorBlendStateCreateInfo ColourBlending{};
    ColourBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColourBlending.logicOpEnable = VK_FALSE;
    ColourBlending.logicOp = VK_LOGIC_OP_COPY;
    ColourBlending.attachmentCount = 1;
    ColourBlending.pAttachments = &ColourBlendAttachment;
    ColourBlending.blendConstants[0] = 0.0f; // Optional
    ColourBlending.blendConstants[1] = 0.0f; // Optional
    ColourBlending.blendConstants[2] = 0.0f; // Optional
    ColourBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineLayoutCreateInfo PipelineLayoutInfo{};
    PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(LogicalDevice, &PipelineLayoutInfo, nullptr, &PipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo PipelineInfo = {};
    PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineInfo.stageCount = 2;
    PipelineInfo.pStages = ShaderStages;
    PipelineInfo.pVertexInputState = &VertexInputInfo;
    PipelineInfo.pInputAssemblyState = &InputAssemblyInfo;
    PipelineInfo.pViewportState = &ViewportInfo;
    PipelineInfo.pRasterizationState = &Rasterizer;
    PipelineInfo.pMultisampleState = &MultisamplingInfo;
    PipelineInfo.pDepthStencilState = nullptr;
    PipelineInfo.pColorBlendState = &ColourBlending;
    PipelineInfo.pDynamicState = &DynamicStateInfo;
    PipelineInfo.layout = PipelineLayout;
    PipelineInfo.renderPass = RenderPass;
    PipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &GraphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    vkDestroyShaderModule(LogicalDevice, FragShaderModule, nullptr);
    vkDestroyShaderModule(LogicalDevice, VertShaderModule, nullptr);

}

void Renderer::CreateFramebuffers() {
    SwapChainFramebuffers.resize(SwapChainImageViews.size());
    for (size_t i = 0; i < SwapChainFramebuffers.size(); i++) {
        VkImageView Attachments[] = {
            SwapChainImageViews[i]
        };
        VkFramebufferCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        CreateInfo.renderPass = RenderPass;
        CreateInfo.attachmentCount = 1;
        CreateInfo.pAttachments = Attachments;
        CreateInfo.width = SwapChainExtent.width;
        CreateInfo.height = SwapChainExtent.height;
        CreateInfo.layers = 1;
        if (vkCreateFramebuffer(LogicalDevice, &CreateInfo, nullptr, &SwapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error(std::format("Failed to create frame buffer {}", i));
        }
    }
}

void Renderer::CreateCommandPool() {
    
    QueueFamilyIndices queueFamilyIndicies = FindQueueFamilies(PhysicalDevice);

    VkCommandPoolCreateInfo PoolInfo{};
    PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    PoolInfo.queueFamilyIndex = queueFamilyIndicies.graphicsFamily.value();

    if (vkCreateCommandPool(LogicalDevice, &PoolInfo, nullptr, &CommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
}

void Renderer::CreateCommandBuffer() {

    VkCommandBufferAllocateInfo AllocInfo{};
    AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocInfo.commandPool = CommandPool;
    AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    AllocInfo.commandBufferCount = 1;

    if(vkAllocateCommandBuffers(LogicalDevice,&AllocInfo,&CommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }
}

void Renderer::CreateSyncObjects() {
    VkSemaphoreCreateInfo SemaphoreInfo{};
    SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo FenceInfo{};
    FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(LogicalDevice, &SemaphoreInfo, nullptr, &ImageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(LogicalDevice, &SemaphoreInfo, nullptr, &RenderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(LogicalDevice,&FenceInfo,nullptr,&Fence) != VK_SUCCESS) {
        throw std::runtime_error("Some fuck up with creating semaphores or Fence.");
    }
    

}

void Renderer::DrawFrame() {

    vkWaitForFences(LogicalDevice, 1, &Fence, VK_TRUE, UINT64_MAX);
    vkResetFences(LogicalDevice, 1, &Fence);

    uint32_t ImageIndex;
    vkAcquireNextImageKHR(LogicalDevice, SwapChain, UINT64_MAX, ImageAvailableSemaphore, VK_NULL_HANDLE, &ImageIndex);

    vkResetCommandBuffer(CommandBuffer, 0);

    RecordBufferCommand(CommandBuffer, ImageIndex);

    VkSubmitInfo SubmitInfo{};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore WaitSemaphores[] = { ImageAvailableSemaphore };
    VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    SubmitInfo.waitSemaphoreCount = 1;
    SubmitInfo.pWaitSemaphores = WaitSemaphores;
    SubmitInfo.pWaitDstStageMask = WaitStages;

    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CommandBuffer;

    VkSemaphore SignalSemaphores[] = { RenderFinishedSemaphore };
    SubmitInfo.signalSemaphoreCount = 1;
    SubmitInfo.pSignalSemaphores = SignalSemaphores;

    if (vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, Fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer to queue");
    }

    VkPresentInfoKHR PresentInfo{};
    PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.pWaitSemaphores = SignalSemaphores;
    PresentInfo.waitSemaphoreCount = 1;

    VkSwapchainKHR SwapChains[] = {SwapChain};

    PresentInfo.swapchainCount = 1;
    PresentInfo.pSwapchains = SwapChains;
    PresentInfo.pImageIndices = &ImageIndex;
    PresentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(PresentQueue, &PresentInfo);

}

void Renderer::RecordBufferCommand(VkCommandBuffer CommandBufferIn, uint32_t ImageIndex) {

    VkCommandBufferBeginInfo BeginInfo{};
    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BeginInfo.flags= 0; //Optional
    BeginInfo.pInheritanceInfo = nullptr;//Optional

    if (vkBeginCommandBuffer(CommandBufferIn, &BeginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer");
    }

    VkRenderPassBeginInfo RenderInfo{};
    RenderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderInfo.renderPass = RenderPass;
    RenderInfo.framebuffer = SwapChainFramebuffers[ImageIndex];
    RenderInfo.renderArea.offset = { 0,0 };
    RenderInfo.renderArea.extent = SwapChainExtent;

    VkClearValue ClearValue = { { 0.0,0.0,0.0,1.0 } };
    RenderInfo.clearValueCount = 1;
    RenderInfo.pClearValues = &ClearValue;

    vkCmdBeginRenderPass(CommandBufferIn, &RenderInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(CommandBufferIn, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(SwapChainExtent.width);
    viewport.height = static_cast<float>(SwapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(CommandBufferIn, 0, 1, &viewport);
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = SwapChainExtent;
    vkCmdSetScissor(CommandBufferIn, 0, 1, &scissor);

    vkCmdDraw(CommandBufferIn, 3, 1, 0, 0);
    vkCmdEndRenderPass(CommandBufferIn);
    
    if (vkEndCommandBuffer(CommandBufferIn) != VK_SUCCESS) {
        throw std::runtime_error("Faield to end command buffer");
    }
}

VkShaderModule Renderer::CreateShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo CreateInfo{};

    CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    CreateInfo.codeSize = code.size();
    CreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule ShaderModule;
    if (vkCreateShaderModule(LogicalDevice, &CreateInfo, nullptr, &ShaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    };

    return ShaderModule;
}

void Renderer::mainLoop() {
    while (!glfwWindowShouldClose(Window)) {
        glfwPollEvents();
        DrawFrame();
    }

    vkDeviceWaitIdle(LogicalDevice);
}

bool Renderer::checkValidationSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;

}

bool Renderer::IsPhysicalDeviceSuitable(VkPhysicalDevice DeviceIn)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(DeviceIn, &deviceProperties);
    vkGetPhysicalDeviceFeatures(DeviceIn, &deviceFeatures);

    QueueFamilyIndices indices = FindQueueFamilies(DeviceIn);

    bool ExtensionsSupported = CheckDeviceExtensionSupport(DeviceIn);
    
    bool swapChainAdaquate = false;
    if (ExtensionsSupported) {
        SwapChainSupportDetails Details = QuerySwapChainSupport(DeviceIn);
        swapChainAdaquate = !Details.Formats.empty() && !Details.PresentModes.empty();
    }

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        deviceFeatures.geometryShader && 
        indices.IsComplete() && 
        swapChainAdaquate; //Note swap chain will only be adaquate if extensions are supported too. Might be a misleading false here.
}

SwapChainSupportDetails Renderer::QuerySwapChainSupport(VkPhysicalDevice DeviceIn) {
    SwapChainSupportDetails Details;
     
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(DeviceIn, VKSurface, &Details.Capabilities);

    uint32_t formatcount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(DeviceIn, VKSurface, &formatcount, nullptr);
    if (formatcount != 0) {
        Details.Formats.resize(formatcount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(DeviceIn, VKSurface, &formatcount, Details.Formats.data());
    }

    uint32_t presentmodescount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(DeviceIn, VKSurface, &presentmodescount, nullptr);
    if (presentmodescount != 0) {
        Details.PresentModes.resize(presentmodescount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(DeviceIn, VKSurface, &presentmodescount, Details.PresentModes.data());
    }

    return Details;
}

QueueFamilyIndices Renderer::FindQueueFamilies(VkPhysicalDevice Device)
{
    QueueFamilyIndices Indicies;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queuFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &queueFamilyCount, queuFamilyProperties.data());

    int i = 0;
    for (const auto& queueFamily : queuFamilyProperties) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            Indicies.graphicsFamily = i;
            VkBool32 PresentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(Device, i,VKSurface,&PresentSupport);
            if (PresentSupport) {
                Indicies.presentFamily = i;
                break;
            }
        }
        i++;
    }

    return Indicies;
}

void Renderer::cleanup() {
    
    vkDestroySemaphore(LogicalDevice, ImageAvailableSemaphore, nullptr);
    vkDestroySemaphore(LogicalDevice, RenderFinishedSemaphore, nullptr);
    vkDestroyFence(LogicalDevice, Fence, nullptr);

    vkDestroyCommandPool(LogicalDevice, CommandPool, nullptr);

    for (auto frame : SwapChainFramebuffers) {
        vkDestroyFramebuffer(LogicalDevice, frame, nullptr);
    }
   
    vkDestroyPipeline(LogicalDevice, GraphicsPipeline, nullptr);

    vkDestroyRenderPass(LogicalDevice, RenderPass, nullptr);
    vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);

    for (auto View : SwapChainImageViews) {
        vkDestroyImageView(LogicalDevice, View, nullptr);
    }
    vkDestroySwapchainKHR(LogicalDevice, SwapChain, nullptr);
    vkDestroyDevice(LogicalDevice, nullptr);
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(Instance, VKSurface, nullptr);
    vkDestroyInstance(Instance, nullptr);
    glfwDestroyWindow(Window);
    glfwTerminate();
}

std::vector<const char*> Renderer::GetRequiredExtensions()
{
    uint32_t glfwExtensionsCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

    std::vector<const char*> extenions(glfwExtensions, glfwExtensions + glfwExtensionsCount);
    if (enableValidationLayers) {
        extenions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extenions;
}

bool Renderer::CheckDeviceExtensionSupport(VkPhysicalDevice DeviceIn) {

    uint32_t ExtensionsCount = 0;
    vkEnumerateDeviceExtensionProperties(DeviceIn, nullptr, &ExtensionsCount, nullptr);
    
    std::vector<VkExtensionProperties> ExtensionsFound(ExtensionsCount);
    vkEnumerateDeviceExtensionProperties(DeviceIn, nullptr, &ExtensionsCount, ExtensionsFound.data());
    
    std::set<std::string> requiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

    for (const auto& extension : ExtensionsFound) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

VkSurfaceFormatKHR Renderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& AvailableFormats)
{
    for (const auto& availableFormat : AvailableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return AvailableFormats[0];
}

VkPresentModeKHR Renderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& AvailableModes)
{
    for (const auto& Mode : AvailableModes) {
        if (Mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return Mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& AvailableExtents)
{
    if (AvailableExtents.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return AvailableExtents.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(Window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, AvailableExtents.minImageExtent.width, AvailableExtents.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, AvailableExtents.minImageExtent.height, AvailableExtents.maxImageExtent.height);

        return actualExtent;
    }
}


VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{   
    const char* TextColor = LOG_COLOUR_RESET;
    switch (MessageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        TextColor = LOG_COLOUR_RED;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        TextColor = LOG_COLOUR_YELLOW;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        TextColor = LOG_COLOUR_MAGENTA;
        break;
    default:
        TextColor = LOG_COLOUR_CYAN; //So we know what we're not looking at.
        break;
    }

    std::cerr << TextColor << "VK validation layer: " << pCallbackData->pMessage << LOG_COLOUR_RESET << std::endl;

    return VK_FALSE;
}
