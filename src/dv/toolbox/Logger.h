#ifndef TOOLBOX_LOGGER_H_
#define TOOLBOX_LOGGER_H_

#include <string>

#define LOG(key, message) { toolbox::Logger::log(key, message, __FILE__, __LINE__); }

#define ERROR "error"
#define INFO "info"

namespace toolbox{

    class Logger{

    public:
        static std::string tolog_;

    public:
        static void setLogKeys(const std::string &tolog);
        static int log(const std::string &key, const std::string &msg, const std::string &file, int line);
    
    private:
        static bool toLog(const std::string &key);
    
    };
}

#endif //TOOLBOX_LOGGER_H_


