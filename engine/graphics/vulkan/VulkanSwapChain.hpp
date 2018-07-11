#ifndef TRB_GFX_SwapChain_H_
#define TRB_GFX_SwapChain_H_

#define VK_USE_PLATFORM_XCB_KHR 1

#include "vulkan/vulkan.hpp"
#include "VulkanDevice.hpp"


// Macro to get a procedure address based on a vulkan instance
#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                        \
{                                                                       \
	fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetInstanceProcAddr(inst, "vk"#entrypoint)); \
	if (fp##entrypoint == NULL)                                         \
	{																    \
		exit(1);                                                        \
	}                                                                   \
}

// Macro to get a procedure address based on a vulkan device
#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                           \
{                                                                       \
	fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetDeviceProcAddr(dev, "vk"#entrypoint));   \
	if (fp##entrypoint == NULL)                                         \
	{																    \
		exit(1);                                                        \
	}                                                                   \
}

typedef struct _SwapChainBuffers {
	vk::Image image;
	vk::ImageView view;
} SwapChainBuffer;


namespace trb{
    namespace grfx{

        class VulkanSwapChain{
        private: 
            VulkanDevice vulkanDevice;
            vk::SurfaceKHR surface;
            // Function pointers
            PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
            PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR; 
            PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
            PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
            PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
            PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
            PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
            PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
            PFN_vkQueuePresentKHR fpQueuePresentKHR;
        public:
            vk::Format colorFormat;
            vk::ColorSpaceKHR colorSpace;
            /** @brief Handle to the current swap chain, required for recreation */
            vk::SwapchainKHR swapChain;	
            uint32_t imageCount;
            std::vector<vk::Image> images;
            std::vector<SwapChainBuffer> buffers;
            /** @brief Queue family index of the detected graphics and presenting device queue */
            uint32_t queueNodeIndex = UINT32_MAX;

            /** @brief Creates the platform specific surface abstraction of the native platform window used for presentation */	
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
            void initSurface(void* platformHandle, void* platformWindow)
        #elif defined(VK_USE_PLATFORM_ANDROID_KHR)
            void initSurface(ANativeWindow* window)
        #elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
            void initSurface(wl_display *display, wl_surface *window)
        #elif defined(VK_USE_PLATFORM_XCB_KHR)
            void initSurface(vk::Instance instance, xcb_connection_t* connection, xcb_window_t window)
        #elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
            void initSurface(void* view)
        #elif defined(_DIRECT2DISPLAY)
            void initSurface(uint32_t width, uint32_t height)
        #endif
            {
                vk::Result err = vk::Result::eSuccess;

                // Create the os-specific surface
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
                VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
                surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
                surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
                surfaceCreateInfo.hwnd = (HWND)platformWindow;
                err = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
        #elif defined(VK_USE_PLATFORM_ANDROID_KHR)
                VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
                surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
                surfaceCreateInfo.window = window;
                err = vkCreateAndroidSurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
        #elif defined(VK_USE_PLATFORM_IOS_MVK)
                VkIOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
                surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
                surfaceCreateInfo.pNext = NULL;
                surfaceCreateInfo.flags = 0;
                surfaceCreateInfo.pView = view;
                err = vkCreateIOSSurfaceMVK(instance, &surfaceCreateInfo, nullptr, &surface);
        #elif defined(VK_USE_PLATFORM_MACOS_MVK)
                VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
                surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
                surfaceCreateInfo.pNext = NULL;
                surfaceCreateInfo.flags = 0;
                surfaceCreateInfo.pView = view;
                err = vkCreateMacOSSurfaceMVK(instance, &surfaceCreateInfo, NULL, &surface);
        #elif defined(_DIRECT2DISPLAY)
                createDirect2DisplaySurface(width, height);
        #elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
                VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = {};
                surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
                surfaceCreateInfo.display = display;
                surfaceCreateInfo.surface = window;
                err = vkCreateWaylandSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
        #elif defined(VK_USE_PLATFORM_XCB_KHR)
                VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
                surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
                surfaceCreateInfo.connection = connection;
                surfaceCreateInfo.window = window;
                err = static_cast<vk::Result>(vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, (VkSurfaceKHR_T**)&surface) );
        #endif

                if (err != vk::Result::eSuccess) {
                    //vks::tools::exitFatal("Could not create surface!", err);
                    throw std::runtime_error("Could not create surface!");
                }

                // Get available queue family properties
                uint32_t queueCount;
                vulkanDevice.physicalDevice.getQueueFamilyProperties(&queueCount, NULL);                            
                assert(queueCount >= 1);

                std::vector<vk::QueueFamilyProperties> queueProps(queueCount);
                vulkanDevice.physicalDevice.getQueueFamilyProperties(&queueCount, queueProps.data());

                // Iterate over each queue to learn whether it supports presenting:
                // Find a queue with present support
                // Will be used to present the swap chain images to the windowing system
                std::vector<vk::Bool32> supportsPresent(queueCount);

                for (uint32_t i = 0; i < queueCount; i++) {                    
                    fpGetPhysicalDeviceSurfaceSupportKHR(vulkanDevice.physicalDevice, i, surface, &supportsPresent[i]);
                }

                // Search for a graphics and a present queue in the array of queue
                // families, try to find one that supports both
                uint32_t graphicsQueueNodeIndex = UINT32_MAX;
                uint32_t presentQueueNodeIndex = UINT32_MAX;
                for (uint32_t i = 0; i < queueCount; i++) {
                    if ( (queueProps[i].queueFlags & vk::QueueFlagBits::eGraphics) ) {
                        if (graphicsQueueNodeIndex == UINT32_MAX) {
                            graphicsQueueNodeIndex = i;
                        }
                        if (supportsPresent[i] == VK_TRUE) {
                            graphicsQueueNodeIndex = i;
                            presentQueueNodeIndex = i;
                            break;
                        }
                    }
                }
                if (presentQueueNodeIndex == UINT32_MAX) {	
                    // If there's no queue that supports both present and graphics
                    // try to find a separate present queue
                    for (uint32_t i = 0; i < queueCount; ++i) {
                        if (supportsPresent[i] == VK_TRUE) {
                            presentQueueNodeIndex = i;
                            break;
                        }
                    }
                }

                // Exit if either a graphics or a presenting queue hasn't been found
                if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX) {
                    throw std::runtime_error("Could not find a graphics and/or presenting queue!");
                }

                // todo : Add support for separate graphics and presenting queue
                if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
                    throw std::runtime_error("Separate graphics and presenting queues are not supported yet!");
                }

                queueNodeIndex = graphicsQueueNodeIndex;

                // Get list of supported surface formats
                uint32_t formatCount;
                if( fpGetPhysicalDeviceSurfaceFormatsKHR(vulkanDevice.physicalDevice, surface, &formatCount, NULL) != VK_SUCCESS ){
                    throw std::runtime_error("fpGetPhysicalDeviceSurfaceFormatsKHR Failed");
                }
                assert(formatCount > 0);

                std::vector<vk::SurfaceFormatKHR> surfaceFormats(formatCount);
                if( fpGetPhysicalDeviceSurfaceFormatsKHR(vulkanDevice.physicalDevice, surface, &formatCount, (VkSurfaceFormatKHR*)surfaceFormats.data()) != VK_SUCCESS){
                    throw std::runtime_error("fpGetPhysicalDeviceSurfaceFormatsKHR Failed");
                }

                // If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
                // there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
                if ((formatCount == 1) && (surfaceFormats[0].format == vk::Format::eUndefined)){
                    colorFormat = vk::Format::eB8G8R8A8Unorm;
                    colorSpace = surfaceFormats[0].colorSpace;
                }
                else{
                    // iterate over the list of available surface format and
                    // check for the presence of VK_FORMAT_B8G8R8A8_UNORM
                    bool found_B8G8R8A8_UNORM = false;
                    for (auto&& surfaceFormat : surfaceFormats){
                        if (surfaceFormat.format == vk::Format::eB8G8R8A8Unorm){
                            colorFormat = surfaceFormat.format;
                            colorSpace = surfaceFormat.colorSpace;
                            found_B8G8R8A8_UNORM = true;
                            break;
                        }
                    }

                    // in case VK_FORMAT_B8G8R8A8_UNORM is not available
                    // select the first available color format
                    if (!found_B8G8R8A8_UNORM){
                        colorFormat = surfaceFormats[0].format;
                        colorSpace = surfaceFormats[0].colorSpace;
                    }
                }

            }

            /**
            * Set instance, physical and logical device to use for the swapchain and get all required function pointers
            * 
            * @param instance Vulkan instance to use
            * @param physicalDevice Physical device used to query properties and formats relevant to the swapchain
            * @param device Logical representation of the device to create the swapchain for
            *
            */
            void connect(vk::Instance instance, VulkanDevice vulkanDevice){
                this->vulkanDevice = vulkanDevice;                
                GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceSupportKHR);
                GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
                GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceFormatsKHR);
                GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfacePresentModesKHR);
                GET_DEVICE_PROC_ADDR(vulkanDevice.device, CreateSwapchainKHR);
                GET_DEVICE_PROC_ADDR(vulkanDevice.device, DestroySwapchainKHR);
                GET_DEVICE_PROC_ADDR(vulkanDevice.device, GetSwapchainImagesKHR);
                GET_DEVICE_PROC_ADDR(vulkanDevice.device, AcquireNextImageKHR);
                GET_DEVICE_PROC_ADDR(vulkanDevice.device, QueuePresentKHR);
            }

            /** 
            * Create the swapchain and get it's images with given width and height
            * 
            * @param width Pointer to the width of the swapchain (may be adjusted to fit the requirements of the swapchain)
            * @param height Pointer to the height of the swapchain (may be adjusted to fit the requirements of the swapchain)
            * @param vsync (Optional) Can be used to force vsync'd rendering (by using VK_PRESENT_MODE_FIFO_KHR as presentation mode)
            */
            void create(uint32_t *width, uint32_t *height, bool vsync = false){
                vk::SwapchainKHR oldSwapchain = swapChain;

                // Get physical device surface properties and formats
                vk::SurfaceCapabilitiesKHR surfCaps;
                if(fpGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanDevice.physicalDevice, surface, (VkSurfaceCapabilitiesKHR*)&surfCaps) != VK_SUCCESS){
                    throw std::runtime_error("fpGetPhysicalDeviceSurfaceCapabilitiesKHR Failed");
                }

                // Get available present modes
                uint32_t presentModeCount;
                if(fpGetPhysicalDeviceSurfacePresentModesKHR(vulkanDevice.physicalDevice, surface, &presentModeCount, NULL) != VK_SUCCESS){
                     throw std::runtime_error("fpGetPhysicalDeviceSurfacePresentModesKHR Failed");
                }
                assert(presentModeCount > 0);

                std::vector<vk::PresentModeKHR> presentModes(presentModeCount);
                if(fpGetPhysicalDeviceSurfacePresentModesKHR(vulkanDevice.physicalDevice, surface, &presentModeCount, (VkPresentModeKHR*)presentModes.data()) != VK_SUCCESS){
                     throw std::runtime_error("fpGetPhysicalDeviceSurfacePresentModesKHR Failed");
                }

                vk::Extent2D swapchainExtent = {};
                // If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
                if (surfCaps.currentExtent.width == (uint32_t)-1){
                    // If the surface size is undefined, the size is set to
                    // the size of the images requested.
                    swapchainExtent.width = *width;
                    swapchainExtent.height = *height;
                }
                else{
                    // If the surface size is defined, the swap chain size must match
                    swapchainExtent = surfCaps.currentExtent;
                    *width = surfCaps.currentExtent.width;
                    *height = surfCaps.currentExtent.height;
                }


                // Select a present mode for the swapchain

                // The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
                // This mode waits for the vertical blank ("v-sync")
                vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;

                // If v-sync is not requested, try to find a mailbox mode
                // It's the lowest latency non-tearing present mode available
                if (!vsync){
                    for (size_t i = 0; i < presentModeCount; i++){
                        if (presentModes[i] == vk::PresentModeKHR::eMailbox){
                            swapchainPresentMode = vk::PresentModeKHR::eMailbox;
                            break;
                        }
                        if ((swapchainPresentMode != vk::PresentModeKHR::eMailbox) && (presentModes[i] == vk::PresentModeKHR::eImmediate)){
                            swapchainPresentMode = vk::PresentModeKHR::eImmediate;
                        }
                    }
                }

                // Determine the number of images
                uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
                if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount)){
                    desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
                }

                // Find the transformation of the surface
                vk::SurfaceTransformFlagBitsKHR preTransform;
                if (surfCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity ){
                    // We prefer a non-rotated transform
                    preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity ;
                }else{
                    preTransform = surfCaps.currentTransform;
                }

                // Find a supported composite alpha format (not all devices support alpha opaque)
                vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
                // Simply select the first composite alpha format available
                std::vector<vk::CompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
                    vk::CompositeAlphaFlagBitsKHR::eOpaque,
                    vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
                    vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
                    vk::CompositeAlphaFlagBitsKHR::eInherit,
                };
                for (auto& compositeAlphaFlag : compositeAlphaFlags) {
                    if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
                        compositeAlpha = compositeAlphaFlag;
                        break;
                    };
                }

                vk::SwapchainCreateInfoKHR swapchainCI;
                swapchainCI.pNext = NULL;
                swapchainCI.surface = surface;
                swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
                swapchainCI.imageFormat = colorFormat;
                swapchainCI.imageColorSpace = colorSpace;
                swapchainCI.imageExtent = vk::Extent2D(swapchainExtent.width, swapchainExtent.height);
                swapchainCI.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
                swapchainCI.preTransform = preTransform;
                swapchainCI.imageArrayLayers = 1;
                swapchainCI.imageSharingMode = vk::SharingMode::eExclusive;
                swapchainCI.queueFamilyIndexCount = 0;
                swapchainCI.pQueueFamilyIndices = NULL;
                swapchainCI.presentMode = swapchainPresentMode;
                swapchainCI.oldSwapchain = oldSwapchain;
                // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
                swapchainCI.clipped = VK_TRUE;
                swapchainCI.compositeAlpha = compositeAlpha;

                // Enable transfer source on swap chain images if supported
                if (surfCaps.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc ) {
                    swapchainCI.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc ;
                }

                // Enable transfer destination on swap chain images if supported
                if (surfCaps.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst) {
                    swapchainCI.imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
                }

                if(fpCreateSwapchainKHR(vulkanDevice.device, (VkSwapchainCreateInfoKHR*)&swapchainCI, nullptr, (VkSwapchainKHR_T**)&swapChain )  != VK_SUCCESS){
                     throw std::runtime_error("fpCreateSwapchainKHR Failed");
                }

                // If an existing swap chain is re-created, destroy the old swap chain
                // This also cleans up all the presentable images
                if (oldSwapchain) { 
                    for (uint32_t i = 0; i < imageCount; i++){
                        vkDestroyImageView(vulkanDevice.device, buffers[i].view, nullptr);
                    }
                    fpDestroySwapchainKHR(vulkanDevice.device, oldSwapchain, nullptr);
                }
                if(fpGetSwapchainImagesKHR(vulkanDevice.device, swapChain, &imageCount, NULL) != VK_SUCCESS){
                     throw std::runtime_error("fpGetSwapchainImagesKHR Failed");
                }

                // Get the swap chain images
                images.resize(imageCount);
                if(fpGetSwapchainImagesKHR(vulkanDevice.device, swapChain, &imageCount, (VkImage_T**)images.data()  ) != VK_SUCCESS){
                     throw std::runtime_error("fpGetSwapchainImagesKHR Failed");
                }

                // Get the swap chain buffers containing the image and imageview
                buffers.resize(imageCount);
                for (uint32_t i = 0; i < imageCount; i++)
                {
                    vk::ImageViewCreateInfo colorAttachmentView;
                    colorAttachmentView.pNext = NULL;
                    colorAttachmentView.format = colorFormat;
                    colorAttachmentView.components.r = vk::ComponentSwizzle::eIdentity;
                    colorAttachmentView.components.g = vk::ComponentSwizzle::eIdentity;
                    colorAttachmentView.components.b = vk::ComponentSwizzle::eIdentity;
                    colorAttachmentView.components.a = vk::ComponentSwizzle::eIdentity;
                    colorAttachmentView.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                    colorAttachmentView.subresourceRange.baseMipLevel = 0;
                    colorAttachmentView.subresourceRange.levelCount = 1;
                    colorAttachmentView.subresourceRange.baseArrayLayer = 0;
                    colorAttachmentView.subresourceRange.layerCount = 1;
                    colorAttachmentView.viewType = vk::ImageViewType::e2D;
                    colorAttachmentView.flags = (vk::ImageViewCreateFlags)0;

                    buffers[i].image = images[i];

                    colorAttachmentView.image = buffers[i].image;

                    if( vulkanDevice.device.createImageView(&colorAttachmentView, nullptr, &buffers[i].view) != vk::Result::eSuccess ){                    
                        throw std::runtime_error("fpGetSwapchainImagesKHR Failed");
                    }
                }
            }

            /** 
            * Acquires the next image in the swap chain
            *
            * @param presentCompleteSemaphore (Optional) Semaphore that is signaled when the image is ready for use
            * @param imageIndex Pointer to the image index that will be increased if the next image could be acquired
            *
            * @note The function will always wait until the next image has been acquired by setting timeout to UINT64_MAX
            *
            * @return VkResult of the image acquisition
            */
            VkResult acquireNextImage(vk::Semaphore presentCompleteSemaphore, uint32_t *imageIndex)
            {
                // By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual error is thrown
                // With that we don't have to handle VK_NOT_READY
                return fpAcquireNextImageKHR(vulkanDevice.device, swapChain, UINT64_MAX, presentCompleteSemaphore, (VkFence)nullptr, imageIndex);
            }

            /**
            * Queue an image for presentation
            *
            * @param queue Presentation queue for presenting the image
            * @param imageIndex Index of the swapchain image to queue for presentation
            * @param waitSemaphore (Optional) Semaphore that is waited on before the image is presented (only used if != VK_NULL_HANDLE)
            *
            * @return VkResult of the queue presentation
            */
            VkResult queuePresent(vk::Queue queue, uint32_t imageIndex, vk::Semaphore waitSemaphore)
            {
                vk::PresentInfoKHR presentInfo;
                presentInfo.pNext = NULL;
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = &swapChain;
                presentInfo.pImageIndices = &imageIndex;
                // Check if a wait semaphore has been specified to wait for before presenting the image
                if (waitSemaphore){
                    presentInfo.pWaitSemaphores = &waitSemaphore;
                    presentInfo.waitSemaphoreCount = 1;
                }
                return fpQueuePresentKHR(queue, reinterpret_cast<const VkPresentInfoKHR*>(&presentInfo) );
            }


            /**
            * Destroy and free Vulkan resources used for the swapchain
            */
            void cleanup(vk::Instance instance){
                if (swapChain){
                    for (uint32_t i = 0; i < imageCount; i++){
                        vulkanDevice.device.destroyImageView(buffers[i].view, nullptr);                        
                    }
                }
                if (surface){
                    vulkanDevice.device.destroySwapchainKHR(swapChain, nullptr);
                    instance.destroySurfaceKHR(surface, nullptr);
                }
                surface = nullptr;
                swapChain = nullptr;
            }

        #if defined(_DIRECT2DISPLAY)
            /**
            * Create direct to display surface
            */	
            void createDirect2DisplaySurface(uint32_t width, uint32_t height)
            {
                uint32_t displayPropertyCount;
                
                // Get display property
                vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &displayPropertyCount, NULL);
                VkDisplayPropertiesKHR* pDisplayProperties = new VkDisplayPropertiesKHR[displayPropertyCount];
                vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &displayPropertyCount, pDisplayProperties);

                // Get plane property
                uint32_t planePropertyCount;
                vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, &planePropertyCount, NULL);
                VkDisplayPlanePropertiesKHR* pPlaneProperties = new VkDisplayPlanePropertiesKHR[planePropertyCount];
                vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, &planePropertyCount, pPlaneProperties);

                VkDisplayKHR display = VK_NULL_HANDLE;
                VkDisplayModeKHR displayMode;
                VkDisplayModePropertiesKHR* pModeProperties;
                bool foundMode = false;

                for(uint32_t i = 0; i < displayPropertyCount;++i)
                {
                    display = pDisplayProperties[i].display;
                    uint32_t modeCount;
                    vkGetDisplayModePropertiesKHR(physicalDevice, display, &modeCount, NULL);
                    pModeProperties = new VkDisplayModePropertiesKHR[modeCount];
                    vkGetDisplayModePropertiesKHR(physicalDevice, display, &modeCount, pModeProperties);

                    for (uint32_t j = 0; j < modeCount; ++j)
                    {
                        const VkDisplayModePropertiesKHR* mode = &pModeProperties[j];

                        if (mode->parameters.visibleRegion.width == width && mode->parameters.visibleRegion.height == height)
                        {
                            displayMode = mode->displayMode;
                            foundMode = true;
                            break;
                        }
                    }
                    if (foundMode)
                    {
                        break;
                    }
                    delete [] pModeProperties;
                }

                if(!foundMode)
                {
                    vks::tools::exitFatal("Can't find a display and a display mode!", -1);
                    return;
                }

                // Search for a best plane we can use
                uint32_t bestPlaneIndex = UINT32_MAX;
                VkDisplayKHR* pDisplays = NULL;
                for(uint32_t i = 0; i < planePropertyCount; i++)
                {
                    uint32_t planeIndex=i;
                    uint32_t displayCount;
                    vkGetDisplayPlaneSupportedDisplaysKHR(physicalDevice, planeIndex, &displayCount, NULL);
                    if (pDisplays)
                    {
                        delete [] pDisplays;
                    }
                    pDisplays = new VkDisplayKHR[displayCount];
                    vkGetDisplayPlaneSupportedDisplaysKHR(physicalDevice, planeIndex, &displayCount, pDisplays);

                    // Find a display that matches the current plane
                    bestPlaneIndex = UINT32_MAX;
                    for(uint32_t j = 0; j < displayCount; j++)
                    {
                        if(display == pDisplays[j])
                        {
                            bestPlaneIndex = i;
                            break;
                        }
                    }
                    if(bestPlaneIndex != UINT32_MAX)
                    {
                        break;
                    }
                }

                if(bestPlaneIndex == UINT32_MAX)
                {
                    vks::tools::exitFatal("Can't find a plane for displaying!", -1);
                    return;
                }

                VkDisplayPlaneCapabilitiesKHR planeCap;
                vkGetDisplayPlaneCapabilitiesKHR(physicalDevice, displayMode, bestPlaneIndex, &planeCap);
                VkDisplayPlaneAlphaFlagBitsKHR alphaMode = (VkDisplayPlaneAlphaFlagBitsKHR)0;

                if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR)
                {
                    alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR;
                }
                else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR)
                {
                    alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR;
                }
                else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR)
                {
                    alphaMode = VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR;
                }
                else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR)
                {
                    alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
                }

                VkDisplaySurfaceCreateInfoKHR surfaceInfo{};
                surfaceInfo.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
                surfaceInfo.pNext = NULL;
                surfaceInfo.flags = 0;
                surfaceInfo.displayMode = displayMode;
                surfaceInfo.planeIndex = bestPlaneIndex;
                surfaceInfo.planeStackIndex = pPlaneProperties[bestPlaneIndex].currentStackIndex;
                surfaceInfo.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
                surfaceInfo.globalAlpha = 1.0;
                surfaceInfo.alphaMode = alphaMode;
                surfaceInfo.imageExtent.width = width;
                surfaceInfo.imageExtent.height = height;

                VkResult result = vkCreateDisplayPlaneSurfaceKHR(instance, &surfaceInfo, NULL, &surface);
                if (result !=VK_SUCCESS) {
                    vks::tools::exitFatal("Failed to create surface!", result);
                }

                delete[] pDisplays;
                delete[] pModeProperties;
                delete[] pDisplayProperties;
                delete[] pPlaneProperties;
            }
        #endif 
        };
    }
}

#endif