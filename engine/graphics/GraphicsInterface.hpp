#ifndef TRB_GFX_GraphicInterface_H_
#define TRB_GFX_GraphicInterface_H_

namespace trb{
    namespace grfx{

        class GraphicInterface{
            public:
                virtual const std::string getWindowTitle() const = 0;
                virtual ~GraphicInterface(){};
        };
    }
}

#endif