

#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>

#include "netcdf_bind.h"
#include "dvl.h"


int nc_close(int ncid){
    onc_close_t original = dlsym(RTLD_NEXT, "nc_close");
    return _dvl_nc_close(ncid, original);
}

int nc_open(char *path, int omode, int * ncidp){
    onc_open_t original = dlsym(RTLD_NEXT, "nc_open");
    return _dvl_nc_open(path, omode, ncidp, original);
}

int nc_create(char *path, int cmode, int * ncidp){
    onc_create_t original = dlsym(RTLD_NEXT, "nc_create");
    return dvl_nc_create(path, cmode, ncidp, original);
}



int nc_get_vara(int ncid, int varid, const size_t start[], const size_t count[], const void * valuesp){
    onc_get_vara_t original = dlsym(RTLD_NEXT, "nc_get_vara");
    
    int res = dvl_nc_get(ncid, varid, start, count, valuesp);
    if (res>=0) return (*original)(res, varid, start, count, valuesp);
    return res;
}

int nc_get_vara_int(int ncid, int varid, const size_t start[], const size_t count[], const int * valuesp){
    onc_get_vara_int_t original = dlsym(RTLD_NEXT, "nc_get_vara_int");
    int res = dvl_nc_get(ncid, varid, start, count, valuesp);
    if (res>=0) return (*original)(res, varid, start, count, valuesp);
    return res;
}

int nc_get_vara_float(int ncid, int varid, const size_t start[], const size_t count[], const float * valuesp){
    onc_get_vara_float_t original = dlsym(RTLD_NEXT, "nc_get_vara_float");
    int res = dvl_nc_get(ncid, varid, start, count, valuesp);
    if (res>=0) return (*original)(res, varid, start, count, valuesp);
    return res;
}

int nc_get_vara_double(int ncid, int varid, const size_t start[], const size_t count[], const double * valuesp){
    onc_get_vara_double_t original = dlsym(RTLD_NEXT, "nc_get_vara_double");
    int res = dvl_nc_get(ncid, varid, start, count, valuesp);
    if (res>=0) return (*original)(res, varid, start, count, valuesp);
    return res;
}

int nc_get_vara_char(int ncid, int varid, const size_t start[], const size_t count[], const char * valuesp){
    onc_get_vara_text_t original = dlsym(RTLD_NEXT, "nc_get_vara_text");
    int res = dvl_nc_get(ncid, varid, start, count, valuesp);
    if (res>=0) return (*original)(res, varid, start, count, valuesp);
    return res;
}



int nc_put_vara(int ncid, int varid, const size_t start[], const size_t count[], const void * valuesp){
    onc_put_vara_t original = dlsym(RTLD_NEXT, "nc_put_vara");
    int res = dvl_nc_put(ncid, varid, start, count, valuesp);
#ifdef __WRITE_NO_DATA_DURING_REDIRECT__
    if (res >= 0) {
        return (*original)(res, varid, start, count, valuesp);
    } else if (res == DVL_PREVENT_WRITING_DATA_DURING_REDIRECT) {
        return NC_NOERR;
    }
#else
    if (res>=0) return (*original)(res, varid, start, count, valuesp);
#endif

    return res;
}

int nc_put_vara_int(int ncid, int varid, const size_t start[], const size_t count[], const int * valuesp){
    onc_put_vara_int_t original = dlsym(RTLD_NEXT, "nc_put_vara_int");
    int res = dvl_nc_put(ncid, varid, start, count, valuesp);
#ifdef __WRITE_NO_DATA_DURING_REDIRECT__
    if (res >= 0) {
        return (*original)(res, varid, start, count, valuesp);
    } else if (res == DVL_PREVENT_WRITING_DATA_DURING_REDIRECT) {
        return NC_NOERR;
    }
#else    
    if (res>=0) return (*original)(res, varid, start, count, valuesp);
#endif
    return res;
}

int nc_put_vara_float(int ncid, int varid, const size_t start[], const size_t count[], const float * valuesp){
    onc_put_vara_float_t original = dlsym(RTLD_NEXT, "nc_put_vara_float");
    int res = dvl_nc_put(ncid, varid, start, count, valuesp);
#ifdef __WRITE_NO_DATA_DURING_REDIRECT__
    if (res >= 0) {
        return (*original)(res, varid, start, count, valuesp);
    } else if (res == DVL_PREVENT_WRITING_DATA_DURING_REDIRECT) {
        return NC_NOERR;
    }
#else     
    if (res>=0) return (*original)(res, varid, start, count, valuesp);
#endif
    return res;
}

int nc_put_vara_double(int ncid, int varid, const size_t start[], const size_t count[], const double * valuesp){
    onc_put_vara_double_t original = dlsym(RTLD_NEXT, "nc_put_vara_double");
    int res = dvl_nc_put(ncid, varid, start, count, valuesp);
#ifdef __WRITE_NO_DATA_DURING_REDIRECT__
    if (res >= 0) {
        return (*original)(res, varid, start, count, valuesp);
    } else if (res == DVL_PREVENT_WRITING_DATA_DURING_REDIRECT) {
        return NC_NOERR;
    }
#else 
    if (res>=0) return (*original)(res, varid, start, count, valuesp);
#endif
    return res;
}

int nc_put_vara_char(int ncid, int varid, const size_t start[], const size_t count[], const char * valuesp){
    onc_put_vara_text_t original = dlsym(RTLD_NEXT, "nc_put_vara_text");
    int res = dvl_nc_put(ncid, varid, start, count, valuesp);
#ifdef __WRITE_NO_DATA_DURING_REDIRECT__
    if (res >= 0) {
        return (*original)(res, varid, start, count, valuesp);
    } else if (res == DVL_PREVENT_WRITING_DATA_DURING_REDIRECT) {
        return NC_NOERR;
    }
#else 
    if (res>=0) return (*original)(res, varid, start, count, valuesp);
#endif
    return res;
}
