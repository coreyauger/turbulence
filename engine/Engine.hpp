#ifndef TRB_GFX_Engine_H_
#define TRB_GFX_Engine_H_

#include "graphics/GraphicsManager.hpp"
#include "graphics/vulkan/VulkanGraphics.hpp"

namespace trb{

    class Engine{
        public:
            Engine()
                : graphics( trb::grfx::VulkanGraphics::create() )
            {};
            ~Engine(){};

        private:
            grfx::GraphicsManager graphics;

    };
}

#endif
