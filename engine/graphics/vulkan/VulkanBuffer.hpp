#ifndef TRB_GFX_VulkanBuffer_H_
#define TRB_GFX_VulkanBuffer_H_

#include <vulkan/vulkan.hpp>

namespace trb{
    namespace grfx{

        /**
        * @brief Encapsulates access to a Vulkan buffer backed up by device memory
        * @note To be filled by an external source like the VulkanDevice
        */
        struct Buffer
        {
            vk::Device device;
            vk::Buffer buffer;
            vk::DeviceMemory memory;
            vk::DescriptorBufferInfo descriptor;
            vk::DeviceSize size = 0;
            vk::DeviceSize alignment = 0;
            void* mapped = nullptr;

            /** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
            vk::BufferUsageFlags usageFlags;
            /** @brief Memory propertys flags to be filled by external source at buffer creation (to query at some later point) */
            vk::MemoryPropertyFlags memoryPropertyFlags;

            /** 
            * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
            * 
            * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
            * @param offset (Optional) Byte offset from beginning
            * 
            * @return VkResult of the buffer mapping call
            */
            vk::Result map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0){
                return device.mapMemory(memory, offset, size, (vk::MemoryMapFlagBits)0, &mapped);
            }

            /**
            * Unmap a mapped memory range
            *
            * @note Does not return a result as vkUnmapMemory can't fail
            */
            void unmap(){
                if (mapped){
                    device.unmapMemory(memory);
                    mapped = nullptr;
                }
            }

            /** 
            * Attach the allocated memory block to the buffer
            * 
            * @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
            * 
            * @return VkResult of the bindBufferMemory call
            */
            void bind(vk::DeviceSize offset = 0){
                device.bindBufferMemory(buffer, memory, offset);
            }

            /**
            * Setup the default descriptor for this buffer
            *
            * @param size (Optional) Size of the memory range of the descriptor
            * @param offset (Optional) Byte offset from beginning
            *
            */
            void setupDescriptor(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0){
                descriptor.offset = offset;
                descriptor.buffer = buffer;
                descriptor.range = size;
            }

            /**
            * Copies the specified data to the mapped buffer
            * 
            * @param data Pointer to the data to copy
            * @param size Size of the data to copy in machine units
            *
            */
            void copyTo(void* data, vk::DeviceSize size)
            {
                assert(mapped);
                memcpy(mapped, data, size);
            }

            /** 
            * Flush a memory range of the buffer to make it visible to the device
            *
            * @note Only required for non-coherent memory
            *
            * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
            * @param offset (Optional) Byte offset from beginning
            *
            * @return VkResult of the flush call
            */
            vk::Result flush(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0)
            {
                vk::MappedMemoryRange mappedRange;
                mappedRange.memory = memory;
                mappedRange.offset = offset;
                mappedRange.size = size;
                return device.flushMappedMemoryRanges(1, &mappedRange);
            }

            /**
            * Invalidate a memory range of the buffer to make it visible to the host
            *
            * @note Only required for non-coherent memory
            *
            * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
            * @param offset (Optional) Byte offset from beginning
            *
            * @return VkResult of the invalidate call
            */
            vk::Result invalidate(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0)
            {
                vk::MappedMemoryRange mappedRange;
                mappedRange.memory = memory;
                mappedRange.offset = offset;
                mappedRange.size = size;
                return device.invalidateMappedMemoryRanges(1, &mappedRange);
            }

            /** 
            * Release all Vulkan resources held by this buffer
            */
            void destroy(){
                if (buffer){
                    device.destroyBuffer(buffer, nullptr);                
                }
                if (memory){
                    device.freeMemory(memory, nullptr);                    
                }
            }

        };
    }
}


#endif