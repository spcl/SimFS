/* declares all dvl functions provided by pnetcdf / ncmpi implementation.
   2017-04-27 Pirmin Schmid


   note: this interface is currently outdated. It does not yet implement
   - simulator stop/finalize
   - simulator file redirection for result and checkpoint files
   - client side multithreaded clients

   thus, it is not compatible with DV server versions > 0.9.14.
   Implementation was postponed because there is no app at the moment that allows testing.

   compare with netcdf and hdf5 implementations where to add the required checks.

   2017-09-20 Pirmin Schmid
*/

#ifndef CLIENTLIB_PNETCDF_DVL_NCMPI_
#define CLIENTLIB_PNETCDF_DVL_NCMPI_

#include <mpi.h>

#include "pnetcdf_bind.h"

int dvl_ncmpi_open(MPI_Comm comm, const char *opath, int omode, MPI_Info info, int *ncidp);
int _dvl_ncmpi_open(MPI_Comm comm, const char *opath, int omode, MPI_Info info, int *ncidp, oncmpi_open_t oncmpi_open);

int dvl_ncmpi_create(MPI_Comm comm, const char *opath, int cmode, MPI_Info info, int *ncidp);
int _dvl_ncmpi_create(MPI_Comm comm, const char *opath, int cmode, MPI_Info info, int *ncidp, oncmpi_create_t oncmpi_create);

int dvl_ncmpi_close(int ncid);
int _dvl_ncmpi_close(int ncid, oncmpi_close_t oncmpi_close);

int dvl_ncmpi_get(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, void *ip, MPI_Offset bufcount,
               MPI_Datatype buftype);

int dvl_ncmpi_put(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, const void *op,
               MPI_Offset bufcount, MPI_Datatype buftype);

#endif // CLIENTLIB_PNETCDF_DVL_NCMPI_

