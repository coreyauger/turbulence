VULKAN_SDK_PATH = /home/suroot/projects/vulkan/1.1.77.0/x86_64

CC=g++
VPATH=engine:engine/graphics:engine/graphics/vulkan/
INCLUDES=-Iengine -Iengine/graphics -Iengine/graphics/vulkan/
CFLAGS = -std=c++11 -I$(VULKAN_SDK_PATH)/include $(INCLUDES) -Wall -g
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan

EXECUTABLE=turbulence
OBJ=main.o VulkanGraphics.o

# FIXME: not sure wtf .. but i seem to need this extra obj list
OO=main.o VulkanGraphics.o

turbulence: ${OBJ}
	$(CC) $(CFLAGS) $(OO) -o $@ $(OBJS) $(LDFLAGS)
	
clean:
	-rm -f *.o core *.core

.cpp.o:
	$(CC) $(CFLAGS) -c $<	

