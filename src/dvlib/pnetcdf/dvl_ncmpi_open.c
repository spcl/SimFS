/*
 * Parallel NetCDF version.
 * -> currently, a copy of the dvl_nc_open.c body with adjustments to ncmpi
 * -> needed adjustments:
 *    - onc_open -> oncmpi_open with adjusted argument list
 *      note: comm is before path.
 *    - add comm and rank to file descriptor data structure before hash calculation
 *    - only rank 0 sends message -> add check there
 *    note: each node/rank keeps its own dvl data structure for lookup purpose
 *
 *    - we need to duplicate the MPI_Info structure into the descriptor, too
 *      since dvl_ncmpi_get may open a file
 *
 * 2017-03-14 / 2017-03-19 ps
 */

#include <mpi.h>

#ifndef __NCMPI__
#define __NCMPI__
#endif

#include "../dvl.h"
#include "../dvl_internal.h"
#include "../dvl_proxy.h"

#include "pnetcdf_bind.h"

int _dvl_ncmpi_open(MPI_Comm comm, const char *opath, int omode, MPI_Info info, int *ncidp, oncmpi_open_t oncmpi_open) {
    char buff[BUFFER_SIZE];        
    char npath[MAX_FILE_NAME];

    DVL_CHECK_WITH_COMM(DVL_NC_OPEN_ID, comm);

    dvl.ncmpiopen = oncmpi_open;

    printf("open!! %s\n", opath);

    /* Check if this file is relevant to us */
    char * path = is_result_file(opath, npath);
    if (path==NULL) return (*oncmpi_open)(comm, opath, omode, info, ncidp);


    if (!dvl.is_simulator) {

        printf("DVL OPEN!\n");

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
#ifdef BENCH
        //LSB_Set_Rparam_str("restart", simstart);
        LSB_Res();
#endif

        /* send request to the dvl */
        // only rank 0 sends message
        int mpi_rank;
        MPI_Comm_rank(comm, &mpi_rank);
        if (0 == mpi_rank) {

            int msgsize = BUFFER_SIZE;

            MAKE_MESSAGE(buff, msgsize, "%c:%s:%i:file=%s;gni_addr=%u", DVL_MSG_FOPEN, path, dvl.gni.myrank, path, dvl.gni.addr);

            if (msgsize<0) return DVL_ERROR;
            dvl_send_message(buff, msgsize, 0);
    
            /* recv response */
            dvl_recv_message(buff, BUFFER_SIZE, 1);
        }

        int res; /* it has to be a valid ncid*/

        if (buff[0]==DVL_REPLY_FILE_SIM){
            printf("File is not avail. Opening meta: %s\n", buff+1);
            res = (*oncmpi_open)(comm, buff+1, omode, info, ncidp); 
            if (res!=NC_NOERR){
                printf("Error opening metadata file\n");
                DVL_ABORT;
            }
            dfile->state = DVL_FILE_SIM;     
        }else{
            res = (*oncmpi_open)(comm, opath, omode, info, ncidp);
            dfile->state = DVL_FILE_OPEN;
        }

        dfile->ncid = *ncidp;
        dfile->key = *ncidp;
        dfile->omode = omode;
        memcpy(dfile->path, path, pathlen + 1);

        // add comm and rank & duplicate info
        dfile->comm = comm;
        dfile->rank = mpi_rank;
        MPI_Info_dup(info, &(dfile->info));


        HASH_ADD_INT(dvl.open_files_idx, key, &(dvl.open_files[fileid]));
         
        DVLPRINT("DVL_NC_OPEN: %s; fileid: %i; idx cnt: %u; res: %i; key: %i\n", path, fileid, HASH_COUNT(dvl.open_files_idx), res, *ncidp); 

        return res; 
    }else{ /* we don't care about the simulator */
        /*int msgsize = BUFFER_SIZE;
        MAKE_MESSAGE(buff, msgsize, "%c:%s", DVL_MSG_FOPEN_SIM, path);
        if (msgsize<0) return DVL_ERROR;

        dvl_send_message(buff, msgsize, 1);
        */
        printf("SIM OPEN!\n");
        return (*oncmpi_open)(comm, opath, omode, info, ncidp);
    }
}
