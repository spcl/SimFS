#ifndef CLIENTLIB_PNETCDF_PNETCDF_BIND_H_
#define CLIENTLIB_PNETCDF_PNETCDF_BIND_H_

/*
 * Parallel NetCDF version of the NetCDF wrapper.
 * Only collective calls so far.
 * 2017-03-09 ps
 */

#include <stdlib.h>
#include <mpi.h>

#include <pnetcdf.h>

typedef int (*oncmpi_open_t)(MPI_Comm, const char *, int, MPI_Info, int *);
typedef int (*oncmpi_create_t)(MPI_Comm, const char *, int, MPI_Info, int *);
typedef int (*oncmpi_close_t)(int);


typedef int (*oncmpi_get_vara_all_t)(int, int, const MPI_Offset *,
               const MPI_Offset *, void *, MPI_Offset, MPI_Datatype);
typedef int (*oncmpi_get_vara_int_all_t)(int, int, const MPI_Offset *,
               const MPI_Offset *, int *);
typedef int (*oncmpi_get_vara_double_all_t)(int, int, const MPI_Offset *,
               const MPI_Offset *, double *);
typedef int (*oncmpi_get_vara_float_all_t)(int, int, const MPI_Offset *,
               const MPI_Offset *, float *);
typedef int (*oncmpi_get_vara_text_all_t)(int, int, const MPI_Offset *,
               const MPI_Offset *, char *);
// not implemented types at the moment
// schar, short, longlong 


typedef int (*oncmpi_put_vara_all_t)(int, int, const MPI_Offset *,
               const MPI_Offset *, const void *,
               MPI_Offset, MPI_Datatype);
typedef int (*oncmpi_put_vara_int_all_t)(int, int, const MPI_Offset *,
               const MPI_Offset *, const int *);
typedef int (*oncmpi_put_vara_double_all_t)(int, int, const MPI_Offset *,
               const MPI_Offset *, const double *);
typedef int (*oncmpi_put_vara_float_all_t)(int, int, const MPI_Offset *,
               const MPI_Offset *, const float *);
typedef int (*oncmpi_put_vara_text_all_t)(int, int, const MPI_Offset *,
               const MPI_Offset *, const char *);
// not implemented types at the moment
// schar, short, longlong 

#endif  // CLIENTLIB_PNETCDF_PNETCDF_BIND_H_
