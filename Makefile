VULKAN_SDK_PATH = ./libs

CC=g++
VPATH=engine:engine/graphics:engine/graphics/vulkan/
INCLUDES=-Iexternal/ -Iengine -Iengine/graphics -Iengine/graphics/vulkan/ 
CFLAGS = -std=c++11 -I$(VULKAN_SDK_PATH)/include $(INCLUDES) -Wall -g
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib -lvulkan -lxcb

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

