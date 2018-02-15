#ifndef __DVL_PROXY_H__
#define __DVL_PROXY_H__

#include "dvl_internal.h"

//int dvl_srv_lock(dvl_t * dvl, const char * restart, const char * datafile, float simstart, float simstop);
//int dvl_srv_unlock(dvl_t * dvl, const char * datafile);

//int dvl_srv_mark_complete(dvl_t * dvl, const char * datafile);


#define BUFFER_SIZE 4096


int dvl_send_message(char * buff, int size, int disconnect);
int dvl_recv_message(char * buff, int size, int disconnect);


#define MAKE_MESSAGE(buff, size, format, ...) \
{ \
    size=snprintf(buff, size, format, __VA_ARGS__); \
    if (size>BUFFER_SIZE) { \
        DVLPRINT("Send buffer is too small\n"); \
        size=-1; \
    } \
} \




#endif /* __DVL_PROXY_H__ */
