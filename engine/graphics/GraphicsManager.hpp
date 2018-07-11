#include "GraphicsInterface.hpp"

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
        };
    }
}