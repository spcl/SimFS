#ifndef TOOLBOX_LOGGER_H_
#define TOOLBOX_LOGGER_H_

#include <string>
#include <unordered_map>

#define LOG(key, level, message) { toolbox::Logger::log(key, level, message, __FILE__, __LINE__); }

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


#define LOG_SIMPLE 0
#define LOG_FULL 1

namespace toolbox{

    class Logger{

    private:
        struct logkey{
            const char * name;
            int format;
            int level;
            const char * color;
        };

    private:
        //static std::string tolog_;
        //static const char * log_key_names_[];
        //static int level_;        
        static std::unordered_map<int, struct logkey> keymap_;
        static bool log_all_;

    public:
        //static void setLogKeys(const std::string &tolog, int level);
        static void setLogKey(int key, int level, const char * name, int format=LOG_SIMPLE, const char * color=ANSI_COLOR_RESET);
        static void setLogKeyLevel(int key, int level);
        static int log(int key, int level, const std::string &msg, const std::string &file, int line);
        static void logAll(bool logall);

        

    };
}

#endif //TOOLBOX_LOGGER_H_


