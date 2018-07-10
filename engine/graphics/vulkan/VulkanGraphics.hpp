#ifndef TRB_GFX_VulkanGraphics_H_
#define TRB_GFX_VulkanGraphics_H_

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <set>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include "../GraphicsInterface.hpp"


//////
// Validation Layer Callbacks

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

// END: Validation Layer Callbacks
/////

namespace trb{
    namespace grfx{
        
        class VulkanGraphics : public GraphicInterface{            
            public:                                
                void init(){
                    initWindow();
                    initVulkan();
                }

                static VulkanGraphics* create(bool enableValidationLayers){
                    VulkanGraphics *graphics = new VulkanGraphics();
                    graphics->init();
                    return graphics;
                }

            private:
                VulkanGraphics(){}

                void initWindow();
                void initVulkan();
        };
    }
}

#endif