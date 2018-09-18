#ifndef TRB_GFX_Engine_H_
#define TRB_GFX_Engine_H_

#include "graphics/GraphicsManager.hpp"
#include "graphics/Graphics.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


namespace trb{

    class Engine{
        public:
            Engine()
                : graphics( trb::grfx::Graphics::create(enableValidationLayers) )
            {};
            ~Engine(){};

        private:
            grfx::GraphicsManager graphics;

    };
}

#endif
