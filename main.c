#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h> //for memcpy()

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#include "physicalDevice.h"

const int MAX_FRAMES_IN_FLIGHT = 2;
uint32_t currentFrame = 0;

GLFWwindow* pWindow;
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
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

VkShaderModule vertShader, fragShader;

VkCommandPool commandPool;
VkCommandBuffer *pCommandBuffers;

VkSemaphore *pImageAvailableSemaphores;
VkSemaphore *pRenderFinishedSemaphores;
VkFence *pInFlightFences;

bool framebufferResized = false;

typedef struct vertex {
  vec2 pos;
  vec3 color;
} vertex;

vertex vertices[3] = {    
  {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
  {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
  {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

//void framebufferResizeCallback(GLFWwindow *pWindow, int width, int height) {
void framebufferResizeCallback() {
  framebufferResized = true;
}

void initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  pWindow = glfwCreateWindow(800, 600, "Vulkan", NULL, NULL);
  glfwSetFramebufferSizeCallback(pWindow, framebufferResizeCallback);
}

VkResult createInstance() {
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
  return vkCreateInstance(&createInfo, NULL, &instance);
}

int createSurface() {
  return glfwCreateWindowSurface(instance, pWindow, NULL, &surface);
}

VkResult createDevice() {
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
  return vkCreateDevice(physicalDevice, &createInfo, NULL, &device);
}

void chooseImageFormatAndColorSpace() {
  uint32_t surfaceFormatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, NULL);
  VkSurfaceFormatKHR *surfaceFormats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * surfaceFormatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats);

  swapchainImageFormat = surfaceFormats[0].format;
  swapchainColorSpace = surfaceFormats[0].colorSpace;
  for (uint32_t i = 0; i < surfaceFormatCount; i++) {
    if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      swapchainImageFormat = surfaceFormats[i].format;
      swapchainColorSpace = surfaceFormats[i].colorSpace;
      break;
    }
  }  
}

void choosePresentMode() {
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
  VkPresentModeKHR *presentModes = (VkPresentModeKHR*)malloc(sizeof(VkPresentModeKHR) * presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);

  swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (uint32_t i = 0; i < presentModeCount; i++) 
    if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
      swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
}

void findSwapchainExtent() {
  int width, height;
  glfwGetFramebufferSize(pWindow, &width, &height); 
  swapchainExtent.width = width;
  swapchainExtent.height = height;
}

VkResult createSwapchain() { 
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

  VkSwapchainCreateInfoKHR createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.surface = surface;
  createInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
  createInfo.imageFormat = swapchainImageFormat;
  createInfo.imageColorSpace = swapchainColorSpace;
  createInfo.imageExtent = swapchainExtent;
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
  return vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain);
}

VkResult getSwapchainImages() {
  pSwapchainImages = (VkImage*)malloc(sizeof(VkImage) * swapchainImageCount);
  return vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, pSwapchainImages);
}

void createImageViews() {
  pSwapchainImageViews = (VkImageView*)malloc(sizeof(VkImageView) * swapchainImageCount);
  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    VkImageViewCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.image = pSwapchainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapchainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &createInfo, NULL, &pSwapchainImageViews[i]);
  }
}

VkResult createRenderPass() {
  VkAttachmentDescription colorAttachment;
  colorAttachment.flags = 0;
  colorAttachment.format = swapchainImageFormat;
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
  return vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass);
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
  vertShaderStageCreateInfo.flags = 0;
  vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageCreateInfo.module = vertShader;
  vertShaderStageCreateInfo.pName = "main";
  vertShaderStageCreateInfo.pSpecializationInfo = NULL;

  VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo;
  fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageCreateInfo.pNext = NULL;
  fragShaderStageCreateInfo.flags = 0;
  fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageCreateInfo.module = fragShader;
  fragShaderStageCreateInfo.pName = "main";
  fragShaderStageCreateInfo.pSpecializationInfo = NULL;

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageCreateInfo, fragShaderStageCreateInfo};

  VkVertexInputBindingDescription bindingDescription;
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attributeDescriptions[2];
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(vertex, pos);
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(vertex, color);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo;
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.pNext = NULL;
  vertexInputInfo.flags = 0;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = 2;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

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
  scissor.offset.y = 0;
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
  rasterizer.flags = 0;
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

void createFramebuffers() {
  pSwapchainFramebuffers = (VkFramebuffer*)malloc(sizeof(VkFramebuffer) * swapchainImageCount);
  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    VkImageView attachments = pSwapchainImageViews[i];

    VkFramebufferCreateInfo framebufferInfo;
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = NULL;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &attachments;
    framebufferInfo.width = swapchainExtent.width;
    framebufferInfo.height = swapchainExtent.height;
    framebufferInfo.layers = 1;
    vkCreateFramebuffer(device, &framebufferInfo, NULL, &pSwapchainFramebuffers[i]);
  }
}

VkResult createCommandPool() {
  VkCommandPoolCreateInfo poolInfo;
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.pNext = NULL;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
  return vkCreateCommandPool(device, &poolInfo, NULL, &commandPool);  
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) return i;
  }
  return 0; //should be unreachable
}

void createVertexBuffer() {
  VkBufferCreateInfo bufferInfo;
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.pNext = NULL;
  bufferInfo.flags = 0;
  bufferInfo.size = sizeof(vertex) * 3;
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkCreateBuffer(device, &bufferInfo, NULL, &vertexBuffer);

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

  VkMemoryAllocateInfo allocateInfo;
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.pNext = NULL;
  allocateInfo.allocationSize = memRequirements.size;
  allocateInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  vkAllocateMemory(device, &allocateInfo, NULL, &vertexBufferMemory);

  vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

  void *data;
  vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
  memcpy(data, vertices, (size_t)bufferInfo.size);
  vkUnmapMemory(device, vertexBufferMemory);
}

VkResult createCommandBuffers() {
  pCommandBuffers = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);
  VkCommandBufferAllocateInfo allocateInfo;
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.pNext = NULL;
  allocateInfo.commandPool = commandPool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
  return vkAllocateCommandBuffers(device, &allocateInfo, pCommandBuffers);
}

void createSyncObjects() {
  pImageAvailableSemaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
  pRenderFinishedSemaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
  pInFlightFences = (VkFence*)malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo;
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphoreInfo.pNext = NULL;
  semaphoreInfo.flags = 0;

  VkFenceCreateInfo fenceInfo;
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.pNext = NULL;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkCreateSemaphore(device, &semaphoreInfo, NULL, &pImageAvailableSemaphores[i]);
    vkCreateSemaphore(device, &semaphoreInfo, NULL, &pRenderFinishedSemaphores[i]);
    vkCreateFence(device, &fenceInfo, NULL, &pInFlightFences[i]);
  }
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
  VkBuffer vertexBuffers[] = {vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
  vkCmdEndRenderPass(commandBuffer);
  vkEndCommandBuffer(commandBuffer);
  return 0;
}

void cleanupSwapchain() {
  for (uint32_t i = 0; i < swapchainImageCount; i++)
    vkDestroyFramebuffer(device, pSwapchainFramebuffers[i], NULL);
  vkDestroyPipeline(device, graphicsPipeline, NULL);
  vkDestroyPipelineLayout(device, pipelineLayout, NULL);
  vkDestroyRenderPass(device, renderPass, NULL);
  for (uint32_t i = 0; i < swapchainImageCount; i++) 
    vkDestroyImageView(device, pSwapchainImageViews[i], NULL);
  free(pSwapchainImageViews);
  free(pSwapchainImages);
  free(pSwapchainFramebuffers);
  vkDestroySwapchainKHR(device, swapchain, NULL);
}

void recreateSwapchain() {
  vkDeviceWaitIdle(device);

  cleanupSwapchain();

  chooseImageFormatAndColorSpace();
  choosePresentMode();  
  findSwapchainExtent();
  createSwapchain();
  vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);
  getSwapchainImages();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
}

void drawFrame() {
  vkWaitForFences(device, 1, &pInFlightFences[currentFrame], VK_TRUE, UINT64_MAX - 1);
  uint32_t imageIndex;
  VkResult res = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX - 1, pImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
    recreateSwapchain();
    return;
  }
  vkResetFences(device, 1, &pInFlightFences[currentFrame]);

  vkResetCommandBuffer(pCommandBuffers[currentFrame], 0);
  recordCommandBuffer(pCommandBuffers[currentFrame], imageIndex);

  VkSubmitInfo submitInfo;
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pNext = NULL;
  VkSemaphore waitSemaphores[] = {pImageAvailableSemaphores[currentFrame]};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &pCommandBuffers[currentFrame];
  VkSemaphore signalSemaphores[] = {pRenderFinishedSemaphores[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;
  vkQueueSubmit(graphicsQueue, 1, &submitInfo, pInFlightFences[currentFrame]);

  VkPresentInfoKHR presentInfo;
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = NULL;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  VkSwapchainKHR swapchains[] = {swapchain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = NULL;
  res = vkQueuePresentKHR(presentQueue, &presentInfo);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || framebufferResized) {
    framebufferResized = false;
    recreateSwapchain();
  }
  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void mainLoop() {
  while(!glfwWindowShouldClose(pWindow)) {
    glfwPollEvents();
    drawFrame();
  }
  vkDeviceWaitIdle(device);
}

void cleanup() {
  cleanupSwapchain();
  vkDestroyBuffer(device, vertexBuffer, NULL);
  vkFreeMemory(device, vertexBufferMemory, NULL);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, pImageAvailableSemaphores[i], NULL);
    vkDestroySemaphore(device, pRenderFinishedSemaphores[i], NULL);
    vkDestroyFence(device, pInFlightFences[i], NULL);
  }
  free(pCommandBuffers);
  vkDestroyCommandPool(device, commandPool, NULL);
  vkDestroyDevice(device, NULL);
  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyInstance(instance, NULL);
  glfwDestroyWindow(pWindow);
  glfwTerminate();
}

int main() {
  initWindow();
  createInstance();
  createSurface();
  pickPhysicalDevice(&physicalDevice, &graphicsQueueFamilyIndex, &presentQueueFamilyIndex);
  createDevice();
  vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
  vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
  chooseImageFormatAndColorSpace();
  choosePresentMode();  
  findSwapchainExtent();
  createSwapchain();
  vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);
  getSwapchainImages();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandPool();
  createVertexBuffer();
  createCommandBuffers();
  createSyncObjects();
  mainLoop();
  cleanup();
  return EXIT_SUCCESS;
}
