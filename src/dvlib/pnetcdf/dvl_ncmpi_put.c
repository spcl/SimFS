/*
 * Parallel NetCDF version.
 *
 * -> currently, a copy of the dvl_nc_put.c body with adjustments to ncmpi
 *    only simulator side implemented for DVL
 *
 * -> needed adjustments:
 *    - id -> ncid
 *    - remember start and count are of MPI_Offset[] data type
 *    - *valuesp *op
 *    - bufcount and buftype are given as MPI counts and types
 *      and do not need to be determined by nc_inq lookups
 *      note: nothing is currently done with this information
 *      nor with the looked up information in dvl_nc_get
 *      -> this may change in the future.
 *
 *    - ncmpi_inq_path() does not exist in pnetcdf v1.7
 *      -> path is used from file descriptor struct
 *      -> this needs a lookup in the hashtable
 *
 *    - ncmpi_inq_var() is used (exists in pnetcdf v1.7)
 *      for getting the dimensions of the variable
 *
 *    - stringify_size_array -> stringify_MPI_Offset_array
 *
 * -> note: since each MPI rank can/will store into different data regions
 *    all ranks send their messages individually and not just rank 0
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


void stringify_MPI_Offset_array(char * buff, size_t bsize, const MPI_Offset * arr, size_t asize){
    int off = 0;
    int res;
    for (int i=0; i<asize; i++){
        res = snprintf(buff + off, bsize-off, "%llu,", arr[i]);

        //printf("stringify: ndims: %lu, res: %i, write: %lu; buff: %s\n", asize, res, arr[i], buff);
        if (res>bsize-off) { printf("BUFFER TO SMALL!\n"); DVL_ABORT; }
        off += res;
    }
    printf("\n");
    buff[off-1] = '\0';
}


int dvl_ncmpi_put(int ncid, int varid, const MPI_Offset *start,
               const MPI_Offset *count, const void *op,
               MPI_Offset bufcount, MPI_Datatype buftype) {
#ifdef SEND_PUT_MESSAGE
    char buff[BUFFER_SIZE];
    char pathbuff[MAX_FILE_NAME];
    char npath[MAX_FILE_NAME];
    char countstr[BUFFER_SIZE];
    char startstr[BUFFER_SIZE];

    int bsize = BUFFER_SIZE;

    countstr[0] = '\0';
    startstr[0] = '\0';

    DVL_CHECK(DVL_NC_PUT_ID);

	dvl_file_t * dfile = NULL;

    if (dvl.is_simulator) {
        size_t pathlen;
        int ndims;

        //uint64_t cache_addr = dvl_cache_put(ncid, varid, start, count, op);

        // dimensions lookup
        ncmpi_inq_var(ncid, varid, NULL, NULL, &ndims, NULL, NULL);

        // path lookup through dvl hashmap instead of nc_inq_path
        HASH_FIND_INT(dvl.open_files_idx, &ncid, dfile);
        if (NULL == dfile) {
            // file unknown -> default function will handle it
            return ncid;
        }

        pathlen = strlen(dfile->path);
        if (pathlen >= MAX_FILE_NAME){
            printf("Not enough space to build the PUT message\n");
            DVL_ABORT;
        }
        memcpy(pathbuff, dfile->path, pathlen + 1);

        char * path = is_result_file(pathbuff, npath);
        if (path==NULL) return ncid;

        stringify_MPI_Offset_array(startstr, BUFFER_SIZE, start, ndims);
        stringify_MPI_Offset_array(countstr, BUFFER_SIZE, count, ndims);

        MAKE_MESSAGE(buff, bsize, "%c:%i:%i:%s:%i:%s:%s", DVL_MSG_VPUT, dvl.gni.myrank, varid, path, ndims, startstr, countstr);
        if (bsize<0) return DVL_ERROR;
    
        //printf("sending: %s\n", buff);
        dvl_send_message(buff, bsize, 1);
        
        return ncid;
    }
    
    printf("put from client applications is not supported yet\n");
#endif

    return ncid;
}
