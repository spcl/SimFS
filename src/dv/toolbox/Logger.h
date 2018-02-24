#ifndef TOOLBOX_LOGGER_H_
#define TOOLBOX_LOGGER_H_

#include <string>

#define LOG(key, level, message) { toolbox::Logger::log(key, level, message, __FILE__, __LINE__); }

#define ERROR 0
#define INFO 1
#define SIM_HANDLER 2
#define CLI_HANDLER 3
#define CACHE 4
#define WARNING 5

#define KEYNAMES "ERROR", "INFO", "SIMULATOR", "CLIENT", "CACHE", "WARNING"


namespace toolbox{

    class Logger{

    public:
        static std::string tolog_;
        static const char * log_key_names_[];
        static int level_;        

    public:
        static void setLogKeys(const std::string &tolog, int level);
        static int log(int key, int level, const std::string &msg, const std::string &file, int line);
    
    private:
        static bool toLog(int key);

    };
}

#endif //TOOLBOX_LOGGER_H_


