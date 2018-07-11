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

namespace trb{
    namespace grfx{
        
        class VulkanGraphics : public GraphicInterface{            
            public:                                
                void init(){
                    // TODO: where do we want to get defualts from?
                    const int width = 800;
                    const int height = 600;
                    initWindow(width, height);
                    initVulkan();
                }

                static VulkanGraphics* create(bool enableValidationLayers){
                    VulkanGraphics *graphics = new VulkanGraphics(enableValidationLayers);
                    graphics->init();
                    return graphics;
                }

            private:
                bool enableValidationLayers;
                GLFWwindow* window;                         // TODO: swap this to SDL2
                vk::Instance instance;    
                vk::SurfaceKHR surface;                     // our window draw surface.

                VkDebugReportCallbackEXT callback;          // NOTE: could not get c++ syntax to work here.. so using C  

                VulkanGraphics(bool enableValidationLayers) 
                    : enableValidationLayers(enableValidationLayers)
                {}

                void initWindow(int width, int height);
                void initVulkan();

                // vulkan init functions
                void createInstance();
                bool checkValidationLayerSupport();                
                std::vector<const char*> getRequiredExtensions();
                void setupDebugCallback();
                void createSurface();


                static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
                    std::cerr << "validation layer: " << msg << std::endl;
                    return VK_FALSE;
                }
        };
    }
}

#endif