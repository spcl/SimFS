/*
 * Parallel NetCDF version.
 *
 * -> currently, a copy of the dvl_nc_get.c body with adjustments to ncmpi
 *    only client side implemented for DVL
 *
 * -> needed adjustments:
 *    - id -> ncid
 *    - remember start and count are of MPI_Offset[] data type
 *    - *valuesp *ip
 *    - bufcount and buftype are given as MPI counts and types
 *      and do not need to be determined by nc_inq lookups
 *      note: nothing is currently done with this information
 *      nor with the looked up information in dvl_nc_get
 *      -> this may change in the future.
 *
 * -> note: since each MPI rank can/will ask for different data regions
 *    all ranks send their message individually and not just rank 0
 *
 * 2017-03-09 / 2017-03-19 ps
 */

#include <assert.h>
#include <mpi.h>

#ifndef __NCMPI__
#define __NCMPI__
#endif

#include "../dvl.h"
#include "../dvl_internal.h"
#include "../dvl_proxy.h"

int dvl_ncmpi_get(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, void *ip, MPI_Offset bufcount,
               MPI_Datatype buftype) {

    char buff[BUFFER_SIZE];        
	
    DVL_CHECK(DVL_NC_GET_ID);   

    if (dvl.is_simulator) return ncid;


    dvl_file_t * dfile = NULL;
    HASH_FIND_INT(dvl.open_files_idx, &ncid, dfile);
    if (dfile==NULL) return ncid;

    // nc_inq lookups removed
    // note: the variables were not used at the moment
    // check later with code updates

    /* if the file is already open, just get the data */
    if (dfile->state == DVL_FILE_OPEN){
        return dfile->ncid;
    }

    /* ask the dvl for this data, communicate the rank. 
       It can reply with AVAIL or SIMULATING */
    int msgsize = BUFFER_SIZE;
    MAKE_MESSAGE(buff, msgsize, "%c:%s:%i:%i:", DVL_MSG_VGET, dfile->path, varid, dvl.gni.myrank);
    if (msgsize<0) return DVL_ERROR;

    dvl_send_message(buff, msgsize, 0);
    
    /* recv response */
    dvl_recv_message(buff, BUFFER_SIZE, 1);
   
    if (buff[0] == DVL_REPLY_FILE_OPEN){
        /* if AVAIL just open the file and read from it */
        assert(dfile->state==DVL_FILE_SIM);
        dfile->state = DVL_FILE_OPEN;
    
        snprintf(buff, BUFFER_SIZE, "%s%s", dvl.respath, dfile->path);        
        dvl.ncmpiopen(dfile->comm, buff, dfile->omode, dfile->info, &(dfile->ncid));

        printf("DVL said the file is avail: opening %s\n", buff);
        return dfile->ncid;

    }else if (buff[0] == DVL_REPLY_RDMA){
        /* make RDMA get */

        return -1; /* do not call the original get */
    }

    assert(1!=1);
    return ncid; /* shouldn't be reached */
}
