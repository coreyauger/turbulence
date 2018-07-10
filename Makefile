N_SDK_PATH = /home/suroot/projects/vulkan/1.1.77.0/x86_64

CFLAGS = -std=c++11 -I$(VULKAN_SDK_PATH)/include
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan

turbulence: main.cpp
	g++ $(CFLAGS) -o turbulence main.cpp $(LDFLAGS)

.PHONY: test clean

test: turbulence
	LD_LIBRARY_PATH=$(VULKAN_SDK_PATH)/lib VK_LAYER_PATH=$(VULKAN_SDK_PATH)/etc/explicit_layer.d ./VulkanTest

clean:
	rm -f turbulence


