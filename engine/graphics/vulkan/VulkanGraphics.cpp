#include "VulkanGraphics.hpp"

#include "../../LogManager.hpp"

//////
// Validation Layer Callbacks
const std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

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


void trb::grfx::VulkanGraphics::initVulkan(){
    createInstance();
    setupDebugCallback();
    vulkanDevice.init(instance);
}

void trb::grfx::VulkanGraphics::createInstance(){
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
        vk::ApplicationInfo appInfo;
        // TODO: We will need to grab the name from an Application Manager or pass it in here.
        appInfo.pApplicationName = "Turbulence";        
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.pEngineName = "Turbulence";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.apiVersion = VK_API_VERSION_1_0;


        std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

	// Enable surface extensions depending on os
#if defined(_WIN32)
	    instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
	    instanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(_DIRECT2DISPLAY)
	    instanceExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	    instanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	    instanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
	    instanceExtensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
	    instanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif        

        vk::InstanceCreateInfo createInfo;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.pNext = NULL;
        createInfo.pApplicationInfo = &appInfo;
        if (instanceExtensions.size() > 0){
            if (settings.validation){
                instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            }
            createInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
            createInfo.ppEnabledExtensionNames = instanceExtensions.data();
        }
        if (settings.validation || enableValidationLayers){
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }else{
            createInfo.enabledLayerCount = 0;
        }

        if (vk::createInstance(&createInfo, nullptr, &instance) != vk::Result::eSuccess ) {
            throw std::runtime_error("failed to create instance!");
        }
}


bool trb::grfx::VulkanGraphics::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }
    return true;
}


 void trb::grfx::VulkanGraphics::setupDebugCallback() {
    if (!enableValidationLayers) return;
    // using C style syntax for this call only.  Could not get C++ to work :(
    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = debugCallback;

    if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug callback!");
    }
}




#if defined(VK_USE_PLATFORM_XCB_KHR)

static inline xcb_intern_atom_reply_t* intern_atom_helper(xcb_connection_t *conn, bool only_if_exists, const char *str)
{
	xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, only_if_exists, strlen(str), str);
	return xcb_intern_atom_reply(conn, cookie, NULL);
}

// Set up a window using XCB and request event types
xcb_window_t trb::grfx::VulkanGraphics::setupWindow(){
    std::cout<<"setupWindow xcb" << std::endl;
	LogManager::getInstance()->log( "setupWindow xcb", LogManager::Level::eDebug);

	uint32_t value_mask, value_list[32];

	window = xcb_generate_id(connection);    
	value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	value_list[0] = screen->black_pixel;
	value_list[1] =
		XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EVENT_MASK_KEY_PRESS |
		XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		XCB_EVENT_MASK_POINTER_MOTION |
		XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_BUTTON_RELEASE;

	if (settings.fullscreen)
	{
		width = destWidth = screen->width_in_pixels;
		height = destHeight = screen->height_in_pixels;
	}

    LogManager::getInstance()->log( "xcb_create_window", LogManager::Level::eDebug);
	xcb_create_window(connection,
		XCB_COPY_FROM_PARENT,
		window, screen->root,
		0, 0, width, height, 0,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual,
		value_mask, value_list);

	/* Magic code that will send notification when window is destroyed */
	xcb_intern_atom_reply_t* reply = intern_atom_helper(connection, true, "WM_PROTOCOLS");
	atom_wm_delete_window = intern_atom_helper(connection, false, "WM_DELETE_WINDOW");

    std::cout<<"xcb_change_property" << std::endl;
	xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
		window, (*reply).atom, 4, 32, 1,
		&(*atom_wm_delete_window).atom);

	std::string windowTitle = getWindowTitle();
	xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
		window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
		title.size(), windowTitle.c_str());
	free(reply);

	if (settings.fullscreen){
		xcb_intern_atom_reply_t *atom_wm_state = intern_atom_helper(connection, false, "_NET_WM_STATE");
		xcb_intern_atom_reply_t *atom_wm_fullscreen = intern_atom_helper(connection, false, "_NET_WM_STATE_FULLSCREEN");
		xcb_change_property(connection,
				XCB_PROP_MODE_REPLACE,
				window, atom_wm_state->atom,
				XCB_ATOM_ATOM, 32, 1,
				&(atom_wm_fullscreen->atom));
		free(atom_wm_fullscreen);
		free(atom_wm_state);
	}	

	LogManager::getInstance()->log( "xcb_map_window", LogManager::Level::eDebug);
	xcb_map_window(connection, window);

	return(window);
}

// Initialize XCB connection
void trb::grfx::VulkanGraphics::initxcbConnection()
{ 
    std::cout<< "initxcbConnection" << std::endl;
	const xcb_setup_t *setup;
	xcb_screen_iterator_t iter;
	int scr;

	connection = xcb_connect(NULL, &scr);
	if (!connection) {
		std::cerr <<  "Could not find a compatible Vulkan ICD!" << std::endl;
		exit(1);
	}

	setup = xcb_get_setup(connection);
	iter = xcb_setup_roots_iterator(setup);
	while (scr-- > 0)
		xcb_screen_next(&iter);
	screen = iter.data;
}

void trb::grfx::VulkanGraphics::handleEvent(const xcb_generic_event_t *event)
{
	/*switch (event->response_type & 0x7f)
	{
	case XCB_CLIENT_MESSAGE:
		if ((*(xcb_client_message_event_t*)event).data.data32[0] ==
			(*atom_wm_delete_window).atom) {
			quit = true;
		}
		break;
	case XCB_MOTION_NOTIFY:
	{
		xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
		handleMouseMove((int32_t)motion->event_x, (int32_t)motion->event_y);
		break;
	}
	break;
	case XCB_BUTTON_PRESS:
	{
		xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
		if (press->detail == XCB_BUTTON_INDEX_1)
			mouseButtons.left = true;
		if (press->detail == XCB_BUTTON_INDEX_2)
			mouseButtons.middle = true;
		if (press->detail == XCB_BUTTON_INDEX_3)
			mouseButtons.right = true;
	}
	break;
	case XCB_BUTTON_RELEASE:
	{
		xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
		if (press->detail == XCB_BUTTON_INDEX_1)
			mouseButtons.left = false;
		if (press->detail == XCB_BUTTON_INDEX_2)
			mouseButtons.middle = false;
		if (press->detail == XCB_BUTTON_INDEX_3)
			mouseButtons.right = false;
	}
	break;
	case XCB_KEY_PRESS:
	{
		const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
		switch (keyEvent->detail)
		{
			case KEY_W:
				camera.keys.up = true;
				break;
			case KEY_S:
				camera.keys.down = true;
				break;
			case KEY_A:
				camera.keys.left = true;
				break;
			case KEY_D:
				camera.keys.right = true;
				break;
			case KEY_P:
				paused = !paused;
				break;
			case KEY_F1:
				if (settings.overlay) {
					settings.overlay = !settings.overlay;
				}
				break;				
		}
	}
	break;	
	case XCB_KEY_RELEASE:
	{
		const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
		switch (keyEvent->detail)
		{
			case KEY_W:
				camera.keys.up = false;
				break;
			case KEY_S:
				camera.keys.down = false;
				break;
			case KEY_A:
				camera.keys.left = false;
				break;
			case KEY_D:
				camera.keys.right = false;
				break;			
			case KEY_ESCAPE:
				quit = true;
				break;
		}
		keyPressed(keyEvent->detail);
	}
	break;
	case XCB_DESTROY_NOTIFY:
		quit = true;
		break;
	case XCB_CONFIGURE_NOTIFY:
	{
		const xcb_configure_notify_event_t *cfgEvent = (const xcb_configure_notify_event_t *)event;
		if ((prepared) && ((cfgEvent->width != width) || (cfgEvent->height != height)))
		{
				destWidth = cfgEvent->width;
				destHeight = cfgEvent->height;
				if ((destWidth > 0) && (destHeight > 0))
				{
					windowResize();
				}
		}
	}
	break;
	default:
		break;
	}
    */
}

#endif

