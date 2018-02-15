
#ifndef __NETCDF_BIND_H__
#define __NETCDF_BIND_H__

#include <stdlib.h>

typedef int (*onc_open_t)(const char *, int, int *);
typedef int (*onc_create_t)(const char *, int, int *);
typedef int (*onc_close_t)(int);

typedef int (*onc_put_vara_t)(int, int, const size_t[], const size_t[], const void *);
typedef int (*onc_put_vara_int_t)(int, int, const size_t[], const size_t[], const int *);
typedef int (*onc_put_vara_double_t)(int, int, const size_t[], const size_t[], const double *);
typedef int (*onc_put_vara_float_t)(int, int, const size_t[], const size_t[], const float *);
typedef int (*onc_put_vara_text_t)(int, int, const size_t[], const size_t[], const char *);

typedef int (*onc_get_vara_t)(int, int, const size_t[], const size_t[], const void *);
typedef int (*onc_get_vara_int_t)(int, int, const size_t[], const size_t[], const int *);
typedef int (*onc_get_vara_double_t)(int, int, const size_t[], const size_t[], const double *);
typedef int (*onc_get_vara_float_t)(int, int, const size_t[], const size_t[], const float *);
typedef int (*onc_get_vara_text_t)(int, int, const size_t[], const size_t[], const char *);






#endif /* __NETCDF_BIND_H__ */
