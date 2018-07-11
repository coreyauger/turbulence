
#ifndef TRB_GFX_LogManager_H_
#define TRB_GFX_LogManager_H_

#include <iostream>

namespace trb{
    class LogManager
    {
        public:
            enum Level{
                eDebug = 0,
                eWarn = 1,
                eInfo = 2,
                eError = 3
            };

        private:
            static LogManager* instance;
            
            Level logLevel; 
            LogManager(){};
            ~LogManager(){};

        public:
            static LogManager* getInstance(){
                if (!instance){
                    instance = new LogManager();
                    instance->setLevel( Level::eDebug );
                }
                return instance;
            }
            void setLevel( Level level ){ logLevel = level; }

            void log(const char* msg, Level level){
                if( level < logLevel)
                    std::cout<< msg << std::endl;
            }
    };

    LogManager* LogManager::instance = 0;

}

#endif