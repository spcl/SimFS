/*
 * Parallel NetCDF version.
 *
 * -> currently, a copy of the dvl_nc_open.c body with adjustments to ncmpi
 * -> needed adjustments:
 *    - onc_create -> oncmpi_create with adjusted argument list
 *    - create and insert a file descriptor into the hashmap similar to open to have comm and rank available at closing time
 *    - only rank 0 is sending the message
 *      although all ranks keep the data 
 *
 *    - we need to duplicate the MPI_Info structure into the descriptor, too
 *      since dvl_ncmpi_get may open a file
 *
 * note: create is only caught on the simulator side at the moment
 * as in _dvl_nc_create()
 *
 * 2017-03-09 / 2017-03-19 ps
 */

#include <mpi.h>

#ifndef __NCMPI__
#define __NCMPI__
#endif

#include "../dvl.h"
#include "../dvl_internal.h"
#include "../dvl_proxy.h"

#include "pnetcdf_bind.h"

int _dvl_ncmpi_create(MPI_Comm comm, const char *opath, int cmode, MPI_Info info, int *ncidp, oncmpi_create_t oncmpi_create) {
    char buff[BUFFER_SIZE];        
    char npath[MAX_FILE_NAME];

    DVL_CHECK_WITH_COMM(DVL_NC_CREATE_ID, comm);

    /* Check if this file is relevant to us */
    char * path = is_result_file(opath, npath);
    if (path==NULL) return (*oncmpi_create)(comm, opath, cmode, info, ncidp);
 

    printf("CREATING!\n");   
    if (dvl.is_simulator){
    	// prepare file descriptor -- similar to open
    	/* Check if we have file descriptors available */
        if (dvl.free_files==NULL){
            printf("Error: too many open files!\n");
            DVL_ABORT;
        }        

        size_t pathlen = strlen(path);
        if (pathlen >= MAX_FILE_NAME){
            printf("Error: file name too long: %s\n", path);
            DVL_ABORT;
        }
    
        /* get the next free file descriptor */
        int fileid = dvl.free_files->id;
        dvl.free_files = dvl.free_files->trash_next;

        dvl_file_t * dfile = &(dvl.open_files[fileid]);

        // only rank 0 sends message
        int mpi_rank;
        MPI_Comm_rank(comm, &mpi_rank);
        if (0 == mpi_rank) {
        	// code from nc_create
        	int msgsize = BUFFER_SIZE;
        	MAKE_MESSAGE(buff, msgsize, "%c:%i:%s", DVL_MSG_FCREATE, dvl.gni.myrank, path);
        	if (msgsize<0) return DVL_ERROR;
        	printf("CREATING MESSAGE!\n");   

            dvl_send_message(buff, msgsize, 0);

            // fix for race condition
            // simulator result file creation is now synchronous with DV
            dvl_recv_message(buff, BUFFER_SIZE, 1);
            // nothing needs to be done with the message at the moment.
        }

        // complete file descriptor and insert into hashmap
        dfile->ncid = *ncidp;
        dfile->key = *ncidp;
        dfile->omode = cmode;
        memcpy(dfile->path, path, pathlen + 1);

        // add comm and rank & duplicate info
        dfile->comm = comm;
        dfile->rank = mpi_rank;
        MPI_Info_dup(info, &(dfile->info));

        HASH_ADD_INT(dvl.open_files_idx, key, &(dvl.open_files[fileid]));
    }

    return (*oncmpi_create)(comm, opath, cmode, info, ncidp);
}
