#ifndef TRB_GFX_VulkanGraphics_H_
#define TRB_GFX_VulkanGraphics_H_

#define VK_USE_PLATFORM_XCB_KHR 1


#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>
#include "VulkanAndroid.h"
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <wayland-client.h>
#elif defined(_DIRECT2DISPLAY)
//
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#endif

#include "vulkan/vulkan.hpp"

#include <set>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "../GraphicsInterface.hpp"
#include "../Camera.hpp"

namespace trb{
    namespace grfx{
        
        class VulkanGraphics : public GraphicInterface{            
            public: 
                bool prepared = false;
                uint32_t width = 1280;
                uint32_t height = 720;
                /** @brief Last frame time measured using a high performance timer (if available) */
	            float frameTimer = 1.0f;
                // Defines a frame rate independent timer value clamped from -1.0...1.0
                // For use in animations, rotations, etc.
                float timer = 0.0f;
                // Multiplier for speeding up (or slowing down) the global timer
                float timerSpeed = 0.25f;

                /** @brief Example settings that can be changed e.g. by command line arguments */
                struct Settings {
                    /** @brief Activates validation layers (and message output) when set to true */
                    bool validation = false;
                    /** @brief Set to true if fullscreen mode has been requested via command line */
                    bool fullscreen = false;
                    /** @brief Set to true if v-sync will be forced for the swapchain */
                    bool vsync = false;
                    /** @brief Enable UI overlay */
                    bool overlay = false;
                } settings;

                void init(){
                    setupWindow();
                    initVulkan();
                }

                const std::string getWindowTitle() const {
                    // TODO ..
                    return std::string("Engine Turbulence");
                } 
                void renderLoop();               

            protected:
                  // Frame counter to display fps
	            uint32_t frameCounter = 0;
	            uint32_t lastFPS = 0;
                Camera camera;

                // Destination dimensions for resizing the window
                float fpsTimer = 0.0f;  // fps timer (one second interval)	            
                uint32_t destWidth;
                uint32_t destHeight;
                bool viewUpdated = false;
                bool resizing = false;
                bool paused = false;
                std::string title = "Engine Turbulence";

                bool enableValidationLayers;
                vk::Instance instance;    
                VulkanDevice vulkanDevice;
                VulkanSwapChain swapChain;

                VkDebugReportCallbackEXT callback;          // NOTE: could not get c++ syntax to work here.. so using C  

                VulkanGraphics(bool enableValidationLayers) 
                    : enableValidationLayers(enableValidationLayers)
                {

// TODO: other platform setup...
// https://github.com/SaschaWillems/Vulkan/blob/b4fb49504e714ecbd4485dfe98514a47b4e9c2cc/base/vulkanexamplebase.cpp#L662

	                settings.validation = enableValidationLayers;
	
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	                // Vulkan library is loaded dynamically on Android
	                bool libLoaded = vks::android::loadVulkanLibrary();
	                assert(libLoaded);
#elif defined(_DIRECT2DISPLAY)

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	                initWaylandConnection();
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	                initxcbConnection();
#endif

#if defined(_WIN32)
                    // Enable console if validation is active
                    // Debug message callback will output to it
                    if (this->settings.validation)
                    {
                        setupConsole("Vulkan validation output");
                    }
#endif
                }
                virtual ~VulkanGraphics(){}

                void initVulkan();

                // vulkan init functions
                void createInstance();
                bool checkValidationLayerSupport();                
                std::vector<const char*> getRequiredExtensions();
                void setupDebugCallback();                               
                void updateOverlay();

                // Pure virtual render function (override in derived class)
                virtual void render() = 0;
                // Called when view change occurs
                // Can be overriden in derived class to e.g. update uniform buffers 
                // Containing view dependant matrices
                virtual void viewChanged() {};
                /** @brief (Virtual) Called after a key was pressed, can be used to do custom key handling */
                virtual void keyPressed(uint32_t) {};
                /** @brief (Virtual) Called after th mouse cursor moved and before internal events (like camera rotation) is handled */
                virtual void mouseMoved(double x, double y, bool &handled) {};
                // Called when the window has been resized
                // Can be overriden in derived class to recreate or rebuild resources attached to the frame buffer / swapchain
                virtual void windowResized() {};
                // Pure virtual function to be overriden by the dervice class
                // Called in case of an event where e.g. the framebuffer has to be rebuild and thus
                // all command buffers that may reference this
                virtual void buildCommandBuffers() {};


#if defined(VK_USE_PLATFORM_XCB_KHR)
                bool quit = false;
                xcb_connection_t *connection;
                xcb_screen_t *screen;
                xcb_window_t window;
                xcb_intern_atom_reply_t *atom_wm_delete_window;

                xcb_window_t setupWindow();
                void initxcbConnection();
                void handleEvent(const xcb_generic_event_t *event);            
#endif                 


                static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
                    std::cerr << "validation layer: " << msg << std::endl;
                    return VK_FALSE;
                }
        };
    }
}

#endif