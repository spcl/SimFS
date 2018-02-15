#define _DEFAULT_SOURCE

/**
  * see header file for detailed explanations
  */

#include "dvl_extended_api.h"

#include <inttypes.h>
#include <stdio.h>

#include "../dvl.h"
#include "../dvl_internal.h"
#include "../dvl_proxy.h"

// these values must match with the constants defined in
// DV ExtendedApiMessageHandler

#define API_SET_INFO 1
#define API_GET_INFO 2
#define API_REQUEST_RANGE 3
#define API_TEST_FILE 4
#define API_STATUS 5


int64_t sdavi_set_info(const char *key, int64_t value) {
#ifdef __MT__
    uint32_t mt_rank = 0;
#endif
    DVL_CHECK_WITHOUT_BENCH;

    char buff[BUFFER_SIZE];
    int msgsize = BUFFER_SIZE;
#ifdef __MT__
    MAKE_MESSAGE(buff, msgsize, "%c:%i:%i:%i:%s:%" PRId64, DVL_MSG_EXTENDED_API, mt_rank, API_SET_INFO, 2, key, value);
#else
    MAKE_MESSAGE(buff, msgsize, "%c:%i:%i:%i:%s:%" PRId64, DVL_MSG_EXTENDED_API, dvl.gni.myrank, API_SET_INFO, 2, key, value);
#endif
    if (msgsize < 0) {
        fprintf(stderr, "sdavi_set_info(): could not build message: %s, %" PRId64 ".\n", key, value);
        return EXTENDED_API_ERROR_RETURN_VALUE; 
    }
    dvl_send_message(buff, msgsize, 0);
    dvl_recv_message(buff, BUFFER_SIZE, 1);
    int64_t retval;
    int count = sscanf(buff, "%" PRId64, &retval);
    if (count != 1) {
        buff[BUFFER_SIZE-1] = 0;
        fprintf(stderr, "sdavi_set_info(): invalid return value: %s.\n", buff);
        return EXTENDED_API_ERROR_RETURN_VALUE; 
    }
    return retval;
}


int64_t sdavi_get_info(const char *key, int64_t default_value) {
#ifdef __MT__
    uint32_t mt_rank = 0;
#endif
    DVL_CHECK_WITHOUT_BENCH;
    
    char buff[BUFFER_SIZE];
    int msgsize = BUFFER_SIZE;
#ifdef __MT__
    MAKE_MESSAGE(buff, msgsize, "%c:%i:%i:%i:%s:%" PRId64, DVL_MSG_EXTENDED_API, mt_rank, API_GET_INFO, 2,
        key, default_value);
#else
    MAKE_MESSAGE(buff, msgsize, "%c:%i:%i:%i:%s:%" PRId64, DVL_MSG_EXTENDED_API, dvl.gni.myrank, API_GET_INFO, 2,
        key, default_value);
#endif
    if (msgsize < 0) {
        fprintf(stderr, "sdavi_get_info(): could not build message: %s.\n", key);
        return EXTENDED_API_ERROR_RETURN_VALUE; 
    } 
    dvl_send_message(buff, msgsize, 0);
    dvl_recv_message(buff, BUFFER_SIZE, 1);
    int64_t retval;
    int count = sscanf(buff, "%" PRId64, &retval);
    if (count != 1) {
        buff[BUFFER_SIZE-1] = 0;
        fprintf(stderr, "sdavi_get_info(): invalid return value: %s.\n", buff);
        return EXTENDED_API_ERROR_RETURN_VALUE; 
    }
    return retval;
}


#define REQUEST_RANGE_FLAG_FIRST 1
#define REQUEST_RANGE_FLAG_LAST 2

static int64_t sdavi_request_range_internal(uint32_t rank, int flag, const char *begin, const char *end, int64_t stride) {    
    char buff[BUFFER_SIZE];
    int msgsize = BUFFER_SIZE;
    MAKE_MESSAGE(buff, msgsize, "%c:%i:%i:%i:%i:%s:%s:%" PRId64,
        DVL_MSG_EXTENDED_API, rank, API_REQUEST_RANGE, 4,
        flag, begin, end, stride);
    if (msgsize < 0) {
        fprintf(stderr, "sdavi_request_range_internal(): could not build message: %s, %s, %" PRId64 ".\n", begin, end, stride);
        return EXTENDED_API_ERROR_RETURN_VALUE; 
    }  
    dvl_send_message(buff, msgsize, 0);
    dvl_recv_message(buff, BUFFER_SIZE, 1);
    int64_t retval;
    int count = sscanf(buff, "%" PRId64, &retval);
    if (count != 1) {
        buff[BUFFER_SIZE-1] = 0;
        fprintf(stderr, "sdavi_request_range_internal(): invalid return value: %s.\n", buff);
        return EXTENDED_API_ERROR_RETURN_VALUE; 
    }
    return retval;
}


int64_t sdavi_request_range(const char *begin, const char *end, int64_t stride) {
    uint32_t mt_rank = 0;
    DVL_CHECK_WITHOUT_BENCH;
#ifndef __MT__
    mt_rank = dvl.gni.myrank;
#endif

    return sdavi_request_range_internal(mt_rank, REQUEST_RANGE_FLAG_FIRST | REQUEST_RANGE_FLAG_LAST, begin, end, stride);
}


int64_t sdavi_request_multiple_ranges(const char **begin, const char **end, int64_t *strides, int64_t length) {
    uint32_t mt_rank = 0;
    DVL_CHECK_WITHOUT_BENCH;
#ifndef __MT__
    mt_rank = dvl.gni.myrank;
#endif
    
    // handle special cases
    if (length < 0) {
        fprintf(stderr, "sdavi_request_multiple_ranges(): could not build message for %" PRId64 " range definitions (negative length).\n", length);
        return EXTENDED_API_ERROR_RETURN_VALUE;
    }

    if (length == 0) {
        // empty
        return 0;
    }

    if (length == 1) {
        return sdavi_request_range_internal(mt_rank, REQUEST_RANGE_FLAG_FIRST | REQUEST_RANGE_FLAG_LAST, begin[0], end[0], strides[0]);
    }

    // regular case
    int64_t retval = 0;

    // send first
    retval = sdavi_request_range_internal(mt_rank, REQUEST_RANGE_FLAG_FIRST, begin[0], end[0], strides[0]);
    if (retval < 0) {
        return retval;
    }

    // send body
    int64_t limit = length - 1;
    for (int64_t i = 1; i < limit; i++) {
        retval = sdavi_request_range_internal(mt_rank, 0, begin[i], end[i], strides[i]);
        if (retval < 0) {
            return retval;
        }
    }

    // send last
    return sdavi_request_range_internal(mt_rank, REQUEST_RANGE_FLAG_LAST, begin[limit], end[limit], strides[limit]);
}


int64_t sdavi_test_file(const char *path) {
#ifdef __MT__
    uint32_t mt_rank = 0;
#endif
    DVL_CHECK_WITHOUT_BENCH;
    
    char buff[BUFFER_SIZE];
    int msgsize = BUFFER_SIZE;
#ifdef __MT__
    MAKE_MESSAGE(buff, msgsize, "%c:%i:%i:%i:%s", DVL_MSG_EXTENDED_API, mt_rank, API_TEST_FILE, 1, path);
#else
    MAKE_MESSAGE(buff, msgsize, "%c:%i:%i:%i:%s", DVL_MSG_EXTENDED_API, dvl.gni.myrank, API_TEST_FILE, 1, path);
#endif
    if (msgsize < 0) {
        fprintf(stderr, "sdavi_test_file(): could not build message: %s.\n", path);
        return EXTENDED_API_ERROR_RETURN_VALUE; 
    }  
    dvl_send_message(buff, msgsize, 0);
    dvl_recv_message(buff, BUFFER_SIZE, 1);
    int64_t retval;
    int count = sscanf(buff, "%" PRId64, &retval);
    if (count != 1) {
        buff[BUFFER_SIZE-1] = 0;
        fprintf(stderr, "sdavi_test_file(): invalid return value: %s.\n", buff);
        return EXTENDED_API_ERROR_RETURN_VALUE; 
    }
    return retval;
}

int64_t sdavi_status() {
#ifdef __MT__
    uint32_t mt_rank = 0;
#endif
    DVL_CHECK_WITHOUT_BENCH;
    
    char buff[BUFFER_SIZE];
    int msgsize = BUFFER_SIZE;
#ifdef __MT__
    MAKE_MESSAGE(buff, msgsize, "%c:%i:%i:%i", DVL_MSG_EXTENDED_API, mt_rank, API_STATUS, 0);
#else
    MAKE_MESSAGE(buff, msgsize, "%c:%i:%i:%i", DVL_MSG_EXTENDED_API, dvl.gni.myrank, API_STATUS, 0);
#endif
    if (msgsize < 0) {
        fprintf(stderr, "sdavi_status(): could not build message.\n");
        return EXTENDED_API_ERROR_RETURN_VALUE; 
    }   
    dvl_send_message(buff, msgsize, 0);
    dvl_recv_message(buff, BUFFER_SIZE, 1);
    int64_t retval;
    int count = sscanf(buff, "%" PRId64, &retval);
    if (count != 1) {
        buff[BUFFER_SIZE-1] = 0;
        fprintf(stderr, "sdavi_status(): invalid return value: %s.\n", buff);
        return EXTENDED_API_ERROR_RETURN_VALUE; 
    }
    return retval;
}
