/*
* Basic camera class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "vulkan/VulkanGraphics.hpp"

namespace trb{
    namespace grfx{
        class Graphics : public VulkanGraphics
        {
        private:
            Graphics(bool enableValidationLayers) : VulkanGraphics(enableValidationLayers){

            }
        public:
           static VulkanGraphics* create(bool enableValidationLayers){
                Graphics *graphics = new Graphics(enableValidationLayers);
                graphics->init();
                return graphics;
            }

            void render(){
                
            }
        };
    }
}
