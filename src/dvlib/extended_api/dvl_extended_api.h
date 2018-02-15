/**
 * stubs that translate arguments into the defined message format for DV.
 * note: all functions are synchronous. Thus, functions with void return type
 * are also waiting for a reply from DV before returning back to the caller.
 *
 * message format:
 * E:appid:function_nr:n_arguments:argument1:argument2:argument3:...
 *
 * return message:
 * a number that can be translated to an int64_t
 *
 * v0.2 2017-07-23 Pirmin Schmid
 */


#ifndef CLIENTLIB_EXTENDED_API_DVL_EXTENDED_API_H_
#define CLIENTLIB_EXTENDED_API_DVL_EXTENDED_API_H_

#include <stdint.h>

#define EXTENDED_API_ERROR_RETURN_VALUE (-1)

/**
  * access a pre-defined key-value store in DV
  * set: returns 0 if ok or -1 in case of error
  * get: returns value if found or default otherwise
  */
int64_t sdavi_set_info(const char *key, int64_t value);
int64_t sdavi_get_info(const char *key, int64_t default_value);


// return: 0 ok, error otherwise
int64_t sdavi_request_range(const char *begin, const char *end, int64_t stride);


// return: 0 ok, error otherwise
int64_t sdavi_request_multiple_ranges(const char **begin, const char **end, int64_t *strides, int64_t length);


/**
 * returns value
 *  0 -> not available / not scheduled
 *  1 -> simulation for file is scheduled
 *  2 -> file is being produced in running simulation
 *  3 -> file exists
 */
int64_t sdavi_test_file(const char *path);


/**
 * return value:
 *  0 -> OK
 * -1 -> unspecified error that prevents normal action (fallback)
 * -2 -> cache full preventing additional jobs to be launched at the moment (wait can help)
 */
int64_t sdavi_status();


#endif // CLIENTLIB_EXTENDED_API_DVL_EXTENDED_API_H_
