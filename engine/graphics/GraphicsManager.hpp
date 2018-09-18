#include "GraphicsInterface.hpp"

#include <iostream>

namespace trb{
    namespace grfx{
        
        class GraphicsManager : public GraphicInterface{
            private:
                GraphicInterface* handle;
            
            public:
                GraphicsManager(GraphicInterface* handle) : handle(handle) { }  
                ~GraphicsManager(){ delete handle; }

                const std::string getWindowTitle() const{
                    return handle->getWindowTitle();
                }

                void renderLoop(){
                    std::cout<<"GraphicsManager::renderLoop" << std::endl;
                    std::cout << "handle: " << handle << std::endl;
                    handle->renderLoop();
                }
        };
    }
}