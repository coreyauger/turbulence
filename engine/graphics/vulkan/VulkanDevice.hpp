#ifndef TRB_GFX_VulkanDevice_H_
#define TRB_GFX_VulkanDevice_H_

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <map>

#include "../GraphicsInterface.hpp"

namespace trb{
    namespace grfx{

        struct QueueFamilyIndices {
            int graphicsFamily = -1;
            int computeFamily = -1;
            int presentFamily = -1;

            bool isComplete() {
                return graphicsFamily >= 0 && presentFamily >= 0;
            }
        };
        
        struct VulkanDevice{ 
            vk::PhysicalDevice physicalDevice;   
            vk::Device device;  
            vk::PhysicalDeviceProperties deviceProperties;
            vk::PhysicalDeviceFeatures deviceFeatures;   
            vk::PhysicalDeviceFeatures enabledFeatures;     
            std::vector<vk::QueueFamilyProperties> queueFamilyProperties; 
            QueueFamilyIndices queueFamilyIndices;
            vk::CommandPool commandPool;
            vk::SurfaceKHR surface;                     // our window draw surface. TODO: make this SDL2 compatable..
            
            VulkanDevice(){};
            ~VulkanDevice(){
                if (commandPool) {
                    device.destroyCommandPool(commandPool, nullptr);
                }
                if (device) {
                    device.destroy(nullptr);
                }
            }

            void init(vk::Instance instance){
                uint32_t deviceCount = 0;
                instance.enumeratePhysicalDevices(&deviceCount, nullptr);
                if (deviceCount == 0) {
                    throw std::runtime_error("failed to find GPUs with Vulkan support!");
                }

                std::vector<vk::PhysicalDevice> devices(deviceCount);
                instance.enumeratePhysicalDevices(&deviceCount, devices.data());
                // Use an ordered map to automatically sort candidates by increasing score
                std::multimap<int, VkPhysicalDevice> candidates;

                for (const auto& device : devices) {
                    int score = rateDeviceSuitability(device);
                    candidates.insert(std::make_pair(score, device));
                }

                // Check if the best candidate is suitable at all
                if (candidates.rbegin()->first > 0) {
                    physicalDevice = candidates.rbegin()->second;
                } else {
                    throw std::runtime_error("failed to find a suitable GPU!");
                }

                physicalDevice.getFeatures(&deviceFeatures);
                physicalDevice.getProperties(&deviceProperties);

                // Queue family properties, used for setting up requested queues upon device creation
			    uint32_t queueFamilyCount;
                physicalDevice.getQueueFamilyProperties(&queueFamilyCount, nullptr);
			    assert(queueFamilyCount > 0);
			    queueFamilyProperties.resize(queueFamilyCount);
			    physicalDevice.getQueueFamilyProperties(&queueFamilyCount, queueFamilyProperties.data());

                // TODO: pass these requested features in
                vk::PhysicalDeviceFeatures enabledFeatures;
                if (deviceFeatures.samplerAnisotropy) {
                    enabledFeatures.samplerAnisotropy = VK_TRUE;
                }
                std::vector<const char*> enabledExtensions{};
                createLogicalDevice(enabledFeatures, enabledExtensions);

            }

            int rateDeviceSuitability(vk::PhysicalDevice device) {
                int score = 0;
                // Discrete GPUs have a significant performance advantage
                if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ) {
                    score += 1000;
                }
                // Maximum possible size of textures affects graphics quality
                score += deviceProperties.limits.maxImageDimension2D;
                // Application can't function without geometry shaders
                if (!deviceFeatures.geometryShader) {
                    return 0;
                }
                return score;
            }

            QueueFamilyIndices findQueueFamilies(){
                return findQueueFamilies(physicalDevice);
            }

            QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice physicalDevice){
                QueueFamilyIndices indices;

                uint32_t queueFamilyCount = 0;
                physicalDevice.getQueueFamilyProperties(&queueFamilyCount, nullptr);
                
                std::vector<vk::QueueFamilyProperties> queueFamilies(queueFamilyCount);
                physicalDevice.getQueueFamilyProperties(&queueFamilyCount, queueFamilies.data());

                int i = 0;
                for (const auto& queueFamily : queueFamilies) {
                    if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics ) {
                        indices.graphicsFamily = i;
                    }
                    vk::Bool32 presentSupport = false;
                    physicalDevice.getSurfaceSupportKHR(i, surface, &presentSupport);
                    if (queueFamily.queueCount > 0 && presentSupport) {
                        indices.presentFamily = i;
                    }
                    if (indices.isComplete()) {
                        break;
                    }
                    i++;
                }
                return indices;
            }

            uint32_t getQueueFamilyIndex(vk::QueueFlags queueFlags){
                // Dedicated queue for compute
                // Try to find a queue family index that supports compute but not graphics
                if (queueFlags & vk::QueueFlagBits::eCompute){
                    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
                        if ( (queueFamilyProperties[i].queueFlags & queueFlags) && 
                             ( !(queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) ) ) {
                            return i;
                            break;
                        }
                    }
                }
                // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
                for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
                    if (queueFamilyProperties[i].queueFlags & queueFlags) {
                        return i;
                        break;
                    }
                }

                throw std::runtime_error("Could not find a matching queue family index");
            }



            vk::Result createLogicalDevice(vk::PhysicalDeviceFeatures enabledFeatures, std::vector<const char*> enabledExtensions, vk::QueueFlags requestedQueueTypes = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute)
            {			
                // Desired queues need to be requested upon logical device creation
                // Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
                // requests different queue types

                std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};

                // Get queue family indices for the requested queue family types
                // Note that the indices may overlap depending on the implementation

                const float defaultQueuePriority(0.0f);

                // Graphics queue
                if (requestedQueueTypes & vk::QueueFlagBits::eGraphics) {
                    queueFamilyIndices.graphicsFamily = getQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
                    vk::DeviceQueueCreateInfo queueInfo{};
                    queueInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
                    queueInfo.queueCount = 1;
                    queueInfo.pQueuePriorities = &defaultQueuePriority;
                    queueCreateInfos.push_back(queueInfo);
                } else {
                    queueFamilyIndices.graphicsFamily = VK_NULL_HANDLE;
                }

                // Dedicated compute queue
                if (requestedQueueTypes & vk::QueueFlagBits::eCompute) {
                    queueFamilyIndices.computeFamily = getQueueFamilyIndex(vk::QueueFlagBits::eCompute);
                    if (queueFamilyIndices.computeFamily != queueFamilyIndices.graphicsFamily) {
                        // If compute family index differs, we need an additional queue create info for the compute queue
                        vk::DeviceQueueCreateInfo queueInfo;
                        queueInfo.queueFamilyIndex = queueFamilyIndices.computeFamily;
                        queueInfo.queueCount = 1;
                        queueInfo.pQueuePriorities = &defaultQueuePriority;
                        queueCreateInfos.push_back(queueInfo);
                    }
                } else {
                    // Else we use the same queue
                    queueFamilyIndices.computeFamily = queueFamilyIndices.graphicsFamily;
                }

                // Create the logical device representation
                std::vector<const char*> deviceExtensions(enabledExtensions);
                deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

                vk::DeviceCreateInfo deviceCreateInfo;
                deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
                deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
                deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

                if (deviceExtensions.size() > 0) {
                    deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
                    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
                }

                vk::Result result = physicalDevice.createDevice(&deviceCreateInfo, nullptr, &device);
                if (result == vk::Result::eSuccess ) {
                    commandPool = createCommandPool(queueFamilyIndices.graphicsFamily);
                }else{
                    throw std::runtime_error("failed to create logical device!");
                }
                this->enabledFeatures = enabledFeatures;

                return result;
            }


            VkCommandPool createCommandPool(uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags createFlags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer){
                vk::CommandPoolCreateInfo cmdPoolInfo = {};
                cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
                cmdPoolInfo.flags = createFlags;
                vk::CommandPool cmdPool;
                if(device.createCommandPool(&cmdPoolInfo, nullptr, &cmdPool) != vk::Result::eSuccess){
                    throw std::runtime_error("failed to create logical device!");
                }
                return cmdPool;
            }


        };
    }
}

#endif