#include <iostream>

namespace trb{
    class LogManager
    {
        private:
            static LogManager* instance;
            LogManager(){};
            ~LogManager(){};

        public:
            static LogManager* getInstance(){
                if (!instance){
                    instance = new LogManager();
                }
                return instance;
            }

            void log(const char* msg){
                std::cout<< msg << std::endl;
            }
    };

    LogManager* LogManager::instance = 0;

}