#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "physicalDevice.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

GLFWwindow* window;
VkInstance instance;
const char *const layerNames[] = {"VK_LAYER_KHRONOS_validation"}; 
const char *const extensionNames[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME}; 
VkPhysicalDevice physicalDevice;
VkQueue graphicsQueue;
VkQueue presentQueue;
VkDevice device;
VkSurfaceKHR surface;
uint32_t graphicsQueueFamilyIndex;
uint32_t presentQueueFamilyIndex;
VkSwapchainKHR swapchain;
uint32_t swapchainImageCount;
VkFormat swapchainImageFormat;
VkColorSpaceKHR swapchainColorSpace;
VkPresentModeKHR swapchainPresentMode;
VkExtent2D swapchainExtent;
VkImage *pSwapchainImages;
VkImageView *pSwapchainImageViews;
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;
VkFramebuffer *pSwapchainFramebuffers;

VkShaderModule vertShader, fragShader;

VkCommandPool commandPool;
VkCommandBuffer commandBuffer;

VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
VkFence inFlightFence;

GLFWwindow* initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  return glfwCreateWindow(800, 600, "Vulkan", NULL, NULL);
}

VkResult createInstance(VkInstance *pInstance) {
  VkApplicationInfo appInfo;
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pNext = NULL;
  appInfo.pApplicationName = "Hello, world!";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  
  VkInstanceCreateInfo createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = glfwExtensionCount;
  createInfo.ppEnabledExtensionNames = glfwExtensions;
  createInfo.enabledLayerCount = 0;
#ifdef DEBUG
  createInfo.enabledLayerCount = 1;
  createInfo.ppEnabledLayerNames = layerNames;
#else
  createInfo.enabledLayerCount = 0;
  createInfo.ppEnabledLayerNames = NULL;
#endif
  return vkCreateInstance(&createInfo, NULL, pInstance);
}

int createSurface(VkInstance *pInstance, GLFWwindow *pWindow, VkSurfaceKHR *pSurface) {
  return glfwCreateWindowSurface(*pInstance, pWindow, NULL, pSurface);
}

VkResult createDevice(VkPhysicalDevice *pPhysicalDevice, uint32_t graphicsQueueFamilyIndex, uint32_t presentQueueFamilyIndex, VkDevice *pDevice) {
  float queuePriority = 1.0f;
  
  VkDeviceQueueCreateInfo *pQueueCreateInfos = (VkDeviceQueueCreateInfo*)malloc(sizeof(VkDeviceQueueCreateInfo) * 2);
  pQueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  pQueueCreateInfos[0].pNext = NULL;
  pQueueCreateInfos[0].flags = 0;
  pQueueCreateInfos[0].queueFamilyIndex = graphicsQueueFamilyIndex;
  pQueueCreateInfos[0].queueCount = 1;
  pQueueCreateInfos[0].pQueuePriorities = &queuePriority;
  if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) { //almost never
    pQueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    pQueueCreateInfos[1].pNext = NULL;
    pQueueCreateInfos[1].flags = 0;
    pQueueCreateInfos[1].queueFamilyIndex = presentQueueFamilyIndex;
    pQueueCreateInfos[1].queueCount = 1;
    pQueueCreateInfos[1].pQueuePriorities = &queuePriority;
  }

  VkDeviceCreateInfo createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.queueCreateInfoCount = graphicsQueueFamilyIndex == presentQueueFamilyIndex ? 1 : 2;
  createInfo.pQueueCreateInfos = pQueueCreateInfos;
  createInfo.enabledExtensionCount = 1;
  createInfo.ppEnabledExtensionNames = extensionNames;
  createInfo.pEnabledFeatures = NULL;
  return vkCreateDevice(*pPhysicalDevice, &createInfo, NULL, pDevice);
}

void chooseImageFormatAndColorSpace(VkPhysicalDevice *pPhysicalDevice, VkSurfaceKHR *pSurface, VkFormat *pImageFormat, VkColorSpaceKHR *pColorSpace) {
  uint32_t surfaceFormatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(*pPhysicalDevice, *pSurface, &surfaceFormatCount, NULL);
  VkSurfaceFormatKHR *surfaceFormats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * surfaceFormatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(*pPhysicalDevice, *pSurface, &surfaceFormatCount, surfaceFormats);

  *pImageFormat = surfaceFormats[0].format;
  *pColorSpace = surfaceFormats[0].colorSpace;
  for (uint32_t i = 0; i < surfaceFormatCount; i++) {
    if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      *pImageFormat = surfaceFormats[i].format;
      *pColorSpace = surfaceFormats[i].colorSpace;
      break;
    }
  }  
}

void choosePresentMode(VkPhysicalDevice *pPhysicalDevice, VkSurfaceKHR *pSurface, VkPresentModeKHR *pSwapchainPresentMode) {
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(*pPhysicalDevice, *pSurface, &presentModeCount, NULL);
  VkPresentModeKHR *presentModes = (VkPresentModeKHR*)malloc(sizeof(VkPresentModeKHR) * presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(*pPhysicalDevice, *pSurface, &presentModeCount, presentModes);

  *pSwapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (uint32_t i = 0; i < presentModeCount; i++) 
    if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
      *pSwapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
}

void findSwapchainExtent(GLFWwindow *pWindow, VkExtent2D *pSwapchainExtent) {
  int width, height;
  glfwGetFramebufferSize(pWindow, &width, &height); 
  pSwapchainExtent->width = width;
  pSwapchainExtent->height = height;
}

VkResult createSwapchain(VkPhysicalDevice *pPhysicalDevice, VkDevice *pDevice, VkSurfaceKHR *pSurface, VkFormat *pSwapchainImageFormat, VkColorSpaceKHR *pSwapchainColorSpace, VkPresentModeKHR swapchainPresentMode, VkExtent2D *pSwapchainExtent, uint32_t graphicsQueueFamilyIndex, uint32_t presentQueueFamilyIndex, VkSwapchainKHR *pSwapchain) { 
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*pPhysicalDevice, *pSurface, &surfaceCapabilities);

  VkSwapchainCreateInfoKHR createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.surface = *pSurface;
  createInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
  createInfo.imageFormat = *pSwapchainImageFormat;
  createInfo.imageColorSpace = *pSwapchainColorSpace;
  createInfo.imageExtent = *pSwapchainExtent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  if (graphicsQueueFamilyIndex == presentQueueFamilyIndex) {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = NULL;
  } else {
    uint32_t queueFamilyIndices[2] = {graphicsQueueFamilyIndex, presentQueueFamilyIndex};
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  createInfo.preTransform = surfaceCapabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = swapchainPresentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;
  return vkCreateSwapchainKHR(*pDevice, &createInfo, NULL, pSwapchain);
}

VkResult getSwapchainImageCount(VkDevice *pDevice, VkSwapchainKHR *pSwapchain, uint32_t *pSwapchainImageCount) {
  return vkGetSwapchainImagesKHR(*pDevice, *pSwapchain, pSwapchainImageCount, NULL);
}

VkResult getSwapchainImages(VkDevice *pDevice, VkSwapchainKHR *pSwapchain, uint32_t swapchainImageCount, VkImage **ppSwapchainImages) {
  *ppSwapchainImages = (VkImage*)malloc(sizeof(VkImage) * swapchainImageCount);
  return vkGetSwapchainImagesKHR(*pDevice, *pSwapchain, &swapchainImageCount, *ppSwapchainImages);
}

void createImageViews(VkDevice *pDevice, VkImage *pSwapchainImages, VkFormat *pSwapchainImageFormat, uint32_t swapchainImageCount, VkImageView **ppSwapchainImageViews) {
  *ppSwapchainImageViews = (VkImageView*)malloc(sizeof(VkImageView) * swapchainImageCount);
  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    VkImageViewCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.image = pSwapchainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = *pSwapchainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(*pDevice, &createInfo, NULL, &(*ppSwapchainImageViews)[i]);
  }
}

VkResult createRenderPass(VkDevice *pDevice, VkFormat *pSwapchainImageFormat) {
  VkAttachmentDescription colorAttachment;
  colorAttachment.flags = 0;
  colorAttachment.format = *pSwapchainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef;
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass;
  subpass.flags = 0;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount = 0;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.preserveAttachmentCount = 0;

  VkSubpassDependency dependency;
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency.dependencyFlags = 0;

  VkRenderPassCreateInfo renderPassInfo;
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.pNext = NULL;
  renderPassInfo.flags = 0;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;
  return vkCreateRenderPass(*pDevice, &renderPassInfo, NULL, &renderPass);
}

VkResult createShader(const char *fileName, VkShaderModule *module) {
  FILE *f = fopen(fileName, "rb");
  fseek(f, 0, SEEK_END);
  int size = ftell(f);
  char *buffer = (char *)malloc(sizeof(char) * size);
  fseek(f, 0, SEEK_SET);
  fread(buffer, sizeof(char), size, f);
  fclose(f);

  VkShaderModuleCreateInfo createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.codeSize = size;
  createInfo.pCode = (uint32_t*)buffer;
  VkResult res = vkCreateShaderModule(device, &createInfo, NULL, module);
  free(buffer);
  return res;
}

VkResult createGraphicsPipeline() {
  createShader("shaders/vert.spv", &vertShader);
  createShader("shaders/frag.spv", &fragShader);

  VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo;
  vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageCreateInfo.pNext = NULL;
  vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageCreateInfo.module = vertShader;
  vertShaderStageCreateInfo.pName = "main";
  vertShaderStageCreateInfo.pSpecializationInfo = NULL;

  VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo;
  fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageCreateInfo.pNext = NULL;
  fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageCreateInfo.module = fragShader;
  fragShaderStageCreateInfo.pName = "main";
  fragShaderStageCreateInfo.pSpecializationInfo = NULL;

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageCreateInfo, fragShaderStageCreateInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo;
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.pNext = NULL;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = NULL;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = NULL;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly;
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.pNext = NULL;
  inputAssembly.flags = 0;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;
  
  VkViewport viewport;
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapchainExtent.width;
  viewport.height = (float)swapchainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor;
  scissor.offset.x = 0;
  scissor.offset.x = 0;
  scissor.extent = swapchainExtent;

  VkPipelineViewportStateCreateInfo viewportState;
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.pNext = NULL;
  viewportState.flags = 0;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer;
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.pNext = NULL;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;
  
  VkPipelineMultisampleStateCreateInfo multisampling;
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.pNext = NULL;
  multisampling.flags = 0;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = NULL;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;
  
  VkPipelineColorBlendAttachmentState colorBlendAttachment;
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending;
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.pNext = NULL;
  colorBlending.flags = 0;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo;
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.pNext = NULL;
  pipelineLayoutInfo.flags = 0;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = NULL;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = NULL;
  vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout);

  VkGraphicsPipelineCreateInfo pipelineInfo;
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = NULL;
  pipelineInfo.flags = 0;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = NULL;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = NULL;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;
  VkResult res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline);

  vkDestroyShaderModule(device, vertShader, NULL);
  vkDestroyShaderModule(device, fragShader, NULL);
  
  return res;
}

void createFramebuffers(VkDevice *pDevice, VkImageView *pSwapchainImageViews, VkExtent2D *pSwapchainExtent, uint32_t swapchainImageCount, VkFramebuffer **ppSwapchainFramebuffers) {
  *ppSwapchainFramebuffers = (VkFramebuffer*)malloc(sizeof(VkFramebuffer) * swapchainImageCount);
  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    VkImageView attachments = pSwapchainImageViews[i];

    VkFramebufferCreateInfo framebufferInfo;
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = NULL;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &attachments;
    framebufferInfo.width = pSwapchainExtent->width;
    framebufferInfo.height = pSwapchainExtent->height;
    framebufferInfo.layers = 1;
    vkCreateFramebuffer(*pDevice, &framebufferInfo, NULL, &(*ppSwapchainFramebuffers)[i]);
  }
}

VkResult createCommandPool(VkDevice *pDevice, uint32_t graphicsQueueFamilyIndex) {
  VkCommandPoolCreateInfo poolInfo;
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.pNext = NULL;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
  return vkCreateCommandPool(*pDevice, &poolInfo, NULL, &commandPool);  
}

VkResult createCommandBuffer(VkDevice *pDevice, VkCommandPool *pCommandPool, VkCommandBuffer *pCommandBuffer) {
  VkCommandBufferAllocateInfo allocateInfo;
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.pNext = NULL;
  allocateInfo.commandPool = *pCommandPool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = 1;
  return vkAllocateCommandBuffers(*pDevice, &allocateInfo, pCommandBuffer);
}

void createSyncObjects(VkDevice *pDevice) {
  VkSemaphoreCreateInfo semaphoreInfo;
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphoreInfo.pNext = NULL;
  semaphoreInfo.flags = 0;
  vkCreateSemaphore(*pDevice, &semaphoreInfo, NULL, &imageAvailableSemaphore);
  vkCreateSemaphore(*pDevice, &semaphoreInfo, NULL, &renderFinishedSemaphore);

  VkFenceCreateInfo fenceInfo;
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.pNext = NULL;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  vkCreateFence(*pDevice, &fenceInfo, NULL, &inFlightFence);
}

int recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo;
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pNext = NULL;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = NULL;
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkRenderPassBeginInfo renderPassInfo;
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.pNext = NULL;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = pSwapchainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset.x = 0;
  renderPassInfo.renderArea.offset.y = 0;
  renderPassInfo.renderArea.extent = swapchainExtent;
  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;
  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
  vkCmdEndRenderPass(commandBuffer);
  vkEndCommandBuffer(commandBuffer);
  return 0;
}

void drawFrame() {
  vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX - 1);
  vkResetFences(device, 1, &inFlightFence);
  uint32_t imageIndex;
  vkAcquireNextImageKHR(device, swapchain, UINT64_MAX - 1, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
  vkResetCommandBuffer(commandBuffer, 0);
  recordCommandBuffer(commandBuffer, imageIndex);

  VkSubmitInfo submitInfo;
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pNext = NULL;
  VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;
  vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);

  VkPresentInfoKHR presentInfo;
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  VkSwapchainKHR swapchains[] = {swapchain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = NULL;
  vkQueuePresentKHR(presentQueue, &presentInfo);
}

void mainLoop() {
  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    drawFrame();
  }
  vkDeviceWaitIdle(device);
}

void cleanup() {
  vkDestroySemaphore(device, imageAvailableSemaphore, NULL);
  vkDestroySemaphore(device, renderFinishedSemaphore, NULL);
  vkDestroyFence(device, inFlightFence, NULL);
  vkDestroyCommandPool(device, commandPool, NULL);
  for (uint32_t i = 0; i < swapchainImageCount; i++)
    vkDestroyFramebuffer(device, pSwapchainFramebuffers[i], NULL);
  vkDestroyPipeline(device, graphicsPipeline, NULL);
  vkDestroyPipelineLayout(device, pipelineLayout, NULL);
  vkDestroyRenderPass(device, renderPass, NULL);
  for (uint32_t i = 0; i < swapchainImageCount; i++) 
    vkDestroyImageView(device, pSwapchainImageViews[i], NULL);
  free(pSwapchainImageViews);
  free(pSwapchainImages);
  vkDestroySwapchainKHR(device, swapchain, NULL);
  vkDestroyDevice(device, NULL);
  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyInstance(instance, NULL);
  glfwDestroyWindow(window);
  glfwTerminate();
}

int main() {
  window = initWindow();
  createInstance(&instance);
  createSurface(&instance, window, &surface);
  pickPhysicalDevice(&physicalDevice, &graphicsQueueFamilyIndex, &presentQueueFamilyIndex);
  createDevice(&physicalDevice, graphicsQueueFamilyIndex, presentQueueFamilyIndex, &device);
  vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
  vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
  chooseImageFormatAndColorSpace(&physicalDevice, &surface, &swapchainImageFormat, &swapchainColorSpace);
  choosePresentMode(&physicalDevice, &surface, &swapchainPresentMode);  
  findSwapchainExtent(window, &swapchainExtent);
  createSwapchain(&physicalDevice, &device, &surface, &swapchainImageFormat, &swapchainColorSpace, swapchainPresentMode, &swapchainExtent, graphicsQueueFamilyIndex, presentQueueFamilyIndex, &swapchain);
  getSwapchainImageCount(&device, &swapchain, &swapchainImageCount);
  getSwapchainImages(&device, &swapchain, swapchainImageCount, &pSwapchainImages);
  createImageViews(&device, pSwapchainImages, &swapchainImageFormat, swapchainImageCount, &pSwapchainImageViews);
  createRenderPass(&device, &swapchainImageFormat);
  createGraphicsPipeline();
  createFramebuffers(&device, pSwapchainImageViews, &swapchainExtent, swapchainImageCount, &pSwapchainFramebuffers);
  createCommandPool(&device, graphicsQueueFamilyIndex);
  createCommandBuffer(&device, &commandPool, &commandBuffer);
  createSyncObjects(&device);
  mainLoop();
  cleanup();
  return EXIT_SUCCESS;
}
