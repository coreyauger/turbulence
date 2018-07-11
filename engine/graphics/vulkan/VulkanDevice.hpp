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

#include "VulkanBuffer.hpp"

// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

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
            vk::PhysicalDeviceProperties properties;
            vk::PhysicalDeviceFeatures features;   
            vk::PhysicalDeviceMemoryProperties memoryProperties;
            vk::PhysicalDeviceFeatures enabledFeatures;     
            std::vector<vk::QueueFamilyProperties> queueFamilyProperties; 
            QueueFamilyIndices queueFamilyIndices;
            std::vector<std::string> supportedExtensions;

            vk::CommandPool commandPool;        
            
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

                physicalDevice.getFeatures(&features);
                physicalDevice.getProperties(&properties);

                // Get list of supported extensions
                uint32_t extCount = 0;
                physicalDevice.enumerateDeviceExtensionProperties(nullptr, &extCount, nullptr);
                if (extCount > 0){
                    std::vector<vk::ExtensionProperties> extensions(extCount);
                    if (physicalDevice.enumerateDeviceExtensionProperties(nullptr, &extCount, &extensions.front()) == vk::Result::eSuccess){
                        for (auto ext : extensions){
                            supportedExtensions.push_back(ext.extensionName);
                        }
                    }
                }

                // Queue family properties, used for setting up requested queues upon device creation
			    uint32_t queueFamilyCount;
                physicalDevice.getQueueFamilyProperties(&queueFamilyCount, nullptr);
			    assert(queueFamilyCount > 0);
			    queueFamilyProperties.resize(queueFamilyCount);
			    physicalDevice.getQueueFamilyProperties(&queueFamilyCount, queueFamilyProperties.data());

                // TODO: pass these requested features in
                vk::PhysicalDeviceFeatures enabledFeatures;
                if (features.samplerAnisotropy) {
                    enabledFeatures.samplerAnisotropy = VK_TRUE;
                }
                std::vector<const char*> enabledExtensions{};
                createLogicalDevice(enabledFeatures, enabledExtensions);

            }

            int rateDeviceSuitability(vk::PhysicalDevice device) {
                int score = 0;
                // Discrete GPUs have a significant performance advantage
                if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ) {
                    score += 1000;
                }
                // Maximum possible size of textures affects graphics quality
                score += properties.limits.maxImageDimension2D;
                // Application can't function without geometry shaders
                if (!features.geometryShader) {
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
                    //physicalDevice.getSurfaceSupportKHR(i, surface, &presentSupport);
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


            uint32_t getMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags properties, vk::Bool32 *memTypeFound = nullptr){
                for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++){
                    if ((typeBits & 1) == 1){
                        if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties){
                            if (memTypeFound){
                                *memTypeFound = true;
                            }
                            return i;
                        }
                    }
                    typeBits >>= 1;
                }
                if (memTypeFound){
                    *memTypeFound = false;
                    return 0;
                }
                else{
                    throw std::runtime_error("Could not find a matching memory type");
                }
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


            vk::CommandPool createCommandPool(uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags createFlags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer){
                vk::CommandPoolCreateInfo cmdPoolInfo = {};
                cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
                cmdPoolInfo.flags = createFlags;
                vk::CommandPool cmdPool;
                if(device.createCommandPool(&cmdPoolInfo, nullptr, &cmdPool) != vk::Result::eSuccess){
                    throw std::runtime_error("failed to create logical device!");
                }
                return cmdPool;
            }


            vk::Result createBuffer(vk::BufferUsageFlags usageFlags, vk::MemoryPropertyFlags memoryPropertyFlags, vk::DeviceSize size, vk::Buffer *buffer, vk::DeviceMemory *memory, void *data = nullptr){
                // Create the buffer handle
                vk::BufferCreateInfo bufferCreateInfo;
                bufferCreateInfo.usage = usageFlags;
                bufferCreateInfo.size = size;
                bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
                if( device.createBuffer(&bufferCreateInfo, nullptr, buffer) != vk::Result::eSuccess ){
                    throw std::runtime_error("could not create buffer");
                }
                // Create the memory backing up the buffer handle
                vk::MemoryRequirements memReqs;
                vk::MemoryAllocateInfo memAlloc;
                device.getBufferMemoryRequirements(*buffer, &memReqs);
                memAlloc.allocationSize = memReqs.size;
                // Find a memory type index that fits the properties of the buffer
                memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
                if( device.allocateMemory(&memAlloc, nullptr, memory) != vk::Result::eSuccess ){
                    throw std::runtime_error("failed to allocate memory on device");
                }
                
                // If a pointer to the buffer data has been passed, map the buffer and copy over the data
                if (data != nullptr){
                    void *mapped;
                    //mapMemory( DeviceMemory memory, DeviceSize offset, DeviceSize size, MemoryMapFlags flags, void** ppData )
                    if( device.mapMemory(*memory, (vk::DeviceSize)0, size, (vk::MemoryMapFlags)0, &mapped) != vk::Result::eSuccess ){
                        throw std::runtime_error("failed to map memory to device");
                    }
                    memcpy(mapped, data, size);
                    // If host coherency hasn't been requested, do a manual flush to make writes visible
                    if ( !(memoryPropertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent) ){
                        vk::MappedMemoryRange mappedRange;
                        mappedRange.memory = *memory;
                        mappedRange.offset = 0;
                        mappedRange.size = size;
                        device.flushMappedMemoryRanges(1, &mappedRange);                        
                    }
                    device.unmapMemory(*memory);                    
                }

                // Attach the memory to the buffer object
                device.bindBufferMemory(*buffer, *memory, 0);
                return vk::Result::eSuccess;
            }

            /**
            * Create a buffer on the device
            *
            * @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
            * @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
            * @param buffer Pointer to a vk::Vulkan buffer object
            * @param size Size of the buffer in byes
            * @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
            *
            * @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
            */
            vk::Result createBuffer(vk::BufferUsageFlags usageFlags, vk::MemoryPropertyFlags memoryPropertyFlags, Buffer *buffer, vk::DeviceSize size, void *data = nullptr){
                buffer->device = device;

                // Create the buffer handle
                vk::BufferCreateInfo bufferCreateInfo;
                bufferCreateInfo.usage = usageFlags;
                bufferCreateInfo.size = size;
                if(device.createBuffer(&bufferCreateInfo, nullptr, &buffer->buffer) != vk::Result::eSuccess ){
                    throw std::runtime_error("could not create buffer");
                }

                // Create the memory backing up the buffer handle
                vk::MemoryRequirements memReqs;
                vk::MemoryAllocateInfo memAlloc;
                device.getBufferMemoryRequirements(buffer->buffer, &memReqs);
                memAlloc.allocationSize = memReqs.size;
                // Find a memory type index that fits the properties of the buffer
                memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
                if( device.allocateMemory(&memAlloc, nullptr, &buffer->memory) != vk::Result::eSuccess ){
                    throw std::runtime_error("failed to allocate memory on device");
                }
                buffer->alignment = memReqs.alignment;
                buffer->size = memAlloc.allocationSize;
                buffer->usageFlags = usageFlags;
                buffer->memoryPropertyFlags = memoryPropertyFlags;

                // If a pointer to the buffer data has been passed, map the buffer and copy over the data
                if (data != nullptr){
                    if(buffer->map() != vk::Result::eSuccess){
                        throw std::runtime_error("could not map memory");
                    }
                    memcpy(buffer->mapped, data, size);
                    buffer->unmap();
                }
                // Initialize a default descriptor that covers the whole buffer size
                buffer->setupDescriptor();
                // Attach the memory to the buffer object
                buffer->bind();
                return vk::Result::eSuccess;
            }
            
            void copyBuffer(Buffer *src, Buffer *dst, vk::Queue queue, vk::BufferCopy *copyRegion = nullptr){
                assert(dst->size <= src->size);
                assert(src->buffer);
                vk::CommandBuffer copyCmd = createCommandBuffer( vk::CommandBufferLevel::ePrimary, true);
                vk::BufferCopy bufferCopy;
                if (copyRegion == nullptr){
                    bufferCopy.size = src->size;
                }else{
                    bufferCopy = *copyRegion;
                }
                copyCmd.copyBuffer(src->buffer, dst->buffer, 1, &bufferCopy);
                
                flushCommandBuffer(copyCmd, queue);
            }

            void flushCommandBuffer(vk::CommandBuffer commandBuffer, vk::Queue queue, bool free = true){
                if (!commandBuffer){
                    return;
                }
                commandBuffer.end();
                
                vk::SubmitInfo submitInfo;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffer;

                // Create fence to ensure that the command buffer has finished executing
                vk::FenceCreateInfo fenceInfo;
                fenceInfo.flags = (vk::FenceCreateFlags)0;
                vk::Fence fence;
                if( device.createFence(&fenceInfo, nullptr, &fence) != vk::Result::eSuccess ){
                    throw std::runtime_error("failed to create device semephor fence");
                }                
                if(queue.submit(1,  &submitInfo, fence) != vk::Result::eSuccess ){
                    throw std::runtime_error("failed to submit CommandBuffer to queue");
                }
                if(device.waitForFences(1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT) != vk::Result::eSuccess ){
                    throw std::runtime_error("failed to create wait state for Fence");
                }  
                device.destroyFence(fence, nullptr);                                    
                if (free){
                    device.freeCommandBuffers(commandPool, 1, &commandBuffer);                    
                }
            }
            

            vk::CommandBuffer createCommandBuffer(vk::CommandBufferLevel level, bool begin = false){
                vk::CommandBufferAllocateInfo cmdBufAllocateInfo; // = vks::initializers::commandBufferAllocateInfo(commandPool, level, 1);
                cmdBufAllocateInfo.commandPool = commandPool;
                cmdBufAllocateInfo.level = level;
                cmdBufAllocateInfo.commandBufferCount = 1;

                vk::CommandBuffer cmdBuffer;
                if(device.allocateCommandBuffers(&cmdBufAllocateInfo, &cmdBuffer) != vk::Result::eSuccess ) {
                    throw std::runtime_error("failed to allocate command buffer!");
                }
                // If requested, also start recording for the new command buffer
                if (begin){
                    vk::CommandBufferBeginInfo cmdBufInfo;
                    cmdBuffer.begin(cmdBufInfo);                  
                }
                return cmdBuffer;
            }

            bool extensionSupported(std::string extension){
			    return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end());
		    }

        };
    }
}

#endif