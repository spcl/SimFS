/*
 * Parallel NetCDF version of the NetCDF wrapper.
 * Only collective calls so far.
 * 2017-03-09 ps
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>

#ifndef __NCMPI__
#define __NCMPI__
#endif

#include "pnetcdf_bind.h"
#include "../dvl.h"


// file
int ncmpi_open(MPI_Comm comm, const char *path, int omode, MPI_Info info, int *ncidp) {
    oncmpi_open_t original = dlsym(RTLD_NEXT, "ncmpi_open");
    return _dvl_ncmpi_open(comm, path, omode, info, ncidp, original);
}

int ncmpi_create(MPI_Comm comm, const char *path, int cmode, MPI_Info info, int *ncidp) {
    oncmpi_create_t original = dlsym(RTLD_NEXT, "ncmpi_create");
    return _dvl_ncmpi_create(comm, path, cmode, info, ncidp, original);
}

int ncmpi_close(int ncid) {
    printf("CLOSE CALLED");
    oncmpi_close_t original = dlsym(RTLD_NEXT, "ncmpi_close");
    return _dvl_ncmpi_close(ncid, original);
}


// get
int ncmpi_get_vara_all(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, void *ip, MPI_Offset bufcount,
               MPI_Datatype buftype) {
    oncmpi_get_vara_all_t original = dlsym(RTLD_NEXT, "ncmpi_get_vara_all");
    
    int res = dvl_ncmpi_get(ncid, varid, start, count, ip, bufcount, buftype);
    if (res>=0) return (*original)(res, varid, start, count, ip, bufcount, buftype);
    return res;
}

int ncmpi_get_vara_int_all(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, int *ip) {
    oncmpi_get_vara_int_all_t original = dlsym(RTLD_NEXT, "ncmpi_get_vara_int_all");
    int res = dvl_ncmpi_get(ncid, varid, start, count, ip, -1, MPI_INT);
    if (res>=0) return (*original)(res, varid, start, count, ip);
    return res;
}

int ncmpi_get_vara_double_all(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, double *ip) {
    oncmpi_get_vara_double_all_t original = dlsym(RTLD_NEXT, "ncmpi_get_vara_double_all");
    int res = dvl_ncmpi_get(ncid, varid, start, count, ip, -1, MPI_DOUBLE);
    if (res>=0) return (*original)(res, varid, start, count, ip);
    return res;
}

int ncmpi_get_vara_float_all(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, float *ip) {
    oncmpi_get_vara_float_all_t original = dlsym(RTLD_NEXT, "ncmpi_get_vara_float_all");
    int res = dvl_ncmpi_get(ncid, varid, start, count, ip, -1, MPI_FLOAT);
    if (res>=0) return (*original)(res, varid, start, count, ip);
    return res;
}

int ncmpi_get_vara_text_all(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, char *ip) {
    oncmpi_get_vara_text_all_t original = dlsym(RTLD_NEXT, "ncmpi_get_vara_text_all");
    int res = dvl_ncmpi_get(ncid, varid, start, count, ip, -1, MPI_CHAR);
    if (res>=0) return (*original)(res, varid, start, count, ip);
    return res;
}


// put
int ncmpi_put_vara_all(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, const void *op,
               MPI_Offset bufcount, MPI_Datatype buftype) {
    oncmpi_put_vara_all_t original = dlsym(RTLD_NEXT, "ncmpi_put_vara_all");
    int res = dvl_ncmpi_put(ncid, varid, start, count, op, bufcount, buftype);
    if (res>=0) return (*original)(res, varid, start, count, op, bufcount, buftype);
    return res;
}

int ncmpi_put_vara_int(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, const int *op) {
    oncmpi_put_vara_int_all_t original = dlsym(RTLD_NEXT, "ncmpi_put_vara_int");
    int res = dvl_ncmpi_put(ncid, varid, start, count, op, -1, MPI_INT);
    if (res>=0) return (*original)(res, varid, start, count, op);
    return res;
}

int ncmpi_put_vara_double_all(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, const double *op) {
    oncmpi_put_vara_double_all_t original = dlsym(RTLD_NEXT, "ncmpi_put_vara_double_all");
    int res = dvl_ncmpi_put(ncid, varid, start, count, op, -1, MPI_DOUBLE);
    if (res>=0) return (*original)(res, varid, start, count, op);
    return res;
}

int ncmpi_put_vara_float_all(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, const float *op) {
    oncmpi_put_vara_float_all_t original = dlsym(RTLD_NEXT, "ncmpi_put_vara_float_all");
    int res = dvl_ncmpi_put(ncid, varid, start, count, op, -1, MPI_FLOAT);
    if (res>=0) return (*original)(res, varid, start, count, op);
    return res;
}

int ncmpi_put_vara_text_all(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, const char *op) {
    oncmpi_put_vara_text_all_t original = dlsym(RTLD_NEXT, "ncmpi_put_vara_text_all");
    int res = dvl_ncmpi_put(ncid, varid, start, count, op, -1, MPI_CHAR);
    if (res>=0) return (*original)(res, varid, start, count, op);
    return res;
}

