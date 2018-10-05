#ifndef __DVL_H__
#define __DVL_H__

#define DVL_SUCCESS 0
#define DVL_ERROR -1

#define TYPE_RESTART 0
#define TYPE_RESULT 1

/* We disable the PUT messages because we can run out of ephemeral 
   ports if the simulator is too fast (all the connections are in 
   TIME_WAIT). 
   TODO: The dvlib<-->dv communications need to go on a single, 
   persistent, TCP connection. 
*/
//#define SEND_PUT_MESSAGE

#include <stdint.h>

#ifdef __FLASH__
    #define is_result_file is_result_file_FLASH
	#define is_checkpoint_file is_checkpoint_file_FLASH
#else
    // COSMO and default
    #define is_result_file is_result_file_COSMO
	#define is_checkpoint_file is_checkpoint_file_COSMO
#endif

// shared
int dvl_init(void);
void dvl_finalize(void);
void dvl_sig_finalize(int signo);

#ifdef __MT__
uint32_t dvl_get_current_rank(void);
#endif

// extended API
#include "extended_api/dvl_extended_api.h"

#ifdef __NCMPI__
#include "pnetcdf/dvl_ncmpi.h"

#elif __HDF5__
#include "hdf5/dvl_hdf5.h"

#else
// default netcdf
#include "netcdf_bind.h"

int dvl_nc_open(char *path, int omode, int * ncidp);
int _dvl_nc_open(char *path, int omode, int * ncidp, onc_open_t onc_open);
int dvl_nc_create(char *path, int omode, int * ncidp, onc_create_t onc_create);

int dvl_nc_close(int ncid);
int _dvl_nc_close(int ncid, onc_close_t onc_close);

int dvl_nc_get(int ncid, int varid, const size_t start[], const size_t count[], const void * valuesp);

int dvl_nc_put(int id, int varid, const size_t start[], const size_t count[], const void * valuesp);
#endif

#endif /* __DVL_H__ */
