#ifndef TRB_GFX_VulkanGraphics_H_
#define TRB_GFX_VulkanGraphics_H_

#include "../GraphicsInterface.hpp"

namespace trb{
    namespace grfx{
        
        class VulkanGraphics : public GraphicInterface{
            private:
                VulkanGraphics(){}
            public:                

                static VulkanGraphics* create(){
                    VulkanGraphics *graphics = new VulkanGraphics();
                    // do the init.
                    return graphics;
                }

        };
    }
}

#endif