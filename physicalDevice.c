#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "physicalDevice.h"

extern VkInstance instance;
extern VkPhysicalDevice physicalDevice;
static uint32_t graphicsQueueFamilyIndex;
static uint32_t presentQueueFamilyIndex;
extern VkSurfaceKHR surface;
extern GLFWwindow* window;

bool findQueueFamiliesIndices(VkPhysicalDevice physicalDevice) {
  bool isGraphicsQueueFamilyFound = 0;
  bool isPresentQueueFamilyFound = 0;
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
  VkQueueFamilyProperties *pQueueFamilies = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, pQueueFamilies);
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    if (!isGraphicsQueueFamilyFound)
      if (pQueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        graphicsQueueFamilyIndex = i; //implying family with VK_QUEUE_GRAPHICS_BIT exist
        isGraphicsQueueFamilyFound = true;
    }
    if (!isPresentQueueFamilyFound) {
      VkBool32 presentSupport = 0;
      vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
      if (presentSupport) {
        presentQueueFamilyIndex = i; //implying family that support present exist
        isPresentQueueFamilyFound = true;
      }
    }
  }
  free(pQueueFamilies);
  return isGraphicsQueueFamilyFound && isPresentQueueFamilyFound;
}

bool isExtensionsSupported(VkPhysicalDevice physicalDevice) {
  bool isSupported = 0;
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, NULL);
  VkExtensionProperties *supportedExtensions = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * extensionCount);
  vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, supportedExtensions);
  for (uint32_t i = 0; i < extensionCount; i++) {
    if (strcmp(supportedExtensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) isSupported = true;
  }
  free(supportedExtensions);
  return isSupported;
} 

bool isSwapchainSuitable(VkPhysicalDevice physicalDevice) { 
  //VkSurfaceCapabilitiesKHR surfaceCapabilities;
  //vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

  uint32_t surfaceFormatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, NULL);
  //VkSurfaceFormatKHR *surfaceFormats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * surfaceFormatCount);
  //vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats);

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
  //VkPresentModeKHR *presentModes = (VkPresentModeKHR*)malloc(sizeof(VkPresentModeKHR) * presentModeCount);
  //vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);
  //free(surfaceFormats);
  //free(presentModes);
  return surfaceFormatCount && presentModeCount; //make sure we have at least 1 format and present mode
}

bool isDeviceSuitable(VkPhysicalDevice physicalDevice) {
  return (findQueueFamiliesIndices(physicalDevice) && 
          isExtensionsSupported(physicalDevice) && 
          isSwapchainSuitable(physicalDevice));
}

uint8_t pickPhysicalDevice(VkPhysicalDevice *pPhysicalDevice, uint32_t *pGraphicsQueueFamilyIndex, uint32_t *pPresentQueueFamilyIndex) {
  uint32_t physicalDeviceCount;  
  vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL);
  VkPhysicalDevice* pPhysicalDevices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
  vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, pPhysicalDevices);
  for (uint32_t i = 0; i < physicalDeviceCount; i++) 
    if (isDeviceSuitable(pPhysicalDevices[i])) {
      *pPhysicalDevice = pPhysicalDevices[i];
      free(pPhysicalDevices);
      *pGraphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
      *pPresentQueueFamilyIndex = presentQueueFamilyIndex;
      return 0;
    }
  free(pPhysicalDevices);
  return 1;
}
