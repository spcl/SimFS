/*
 * Parallel NetCDF version.
 * -> currently, a copy of the dvl_nc_close.c body with adjustments to ncmpi
 * -> needed adjustments:
 *    - id -> ncid
 *    - onc_close -> oncmpi_close
 *
 *    - also the simulator side offers the lookup for the file descriptor
 *      -> use the descriptor to find comm and rank; only rank 0 sends the message
 *         all ranks handle their hash table
 *
 *    - nc_inq_path -> ncmpi_inq_path (needs pnetcdf >= v1.8.0)
 *      since only v1.7 is installed -> use the path from the retrieved file descriptor
 *
 *    - we need to delete the prepared copy of the MPI_Info structure (see open and create)
 *
 * 2017-03-14 / 2017-03-19 ps
 */

#ifndef __NCMPI__
#define __NCMPI__
#endif

#include "../dvl.h"
#include "../dvl_internal.h"
#include "../dvl_proxy.h"

#include "pnetcdf_bind.h"

int _dvl_ncmpi_close(int ncid, oncmpi_close_t oncmpi_close) {
    char buff[BUFFER_SIZE];
    char cpath[MAX_FILE_NAME];
    char npath[MAX_FILE_NAME];
    char * path;
    int bsize = BUFFER_SIZE;

    DVL_CHECK(DVL_NC_CLOSE_ID);

    int toclose = ncid;

    dvl_file_t * dfile = NULL;

    // lookup for both, simulator and not simulator
    if (!dvl.is_simulator){
        HASH_FIND_INT(dvl.open_files_idx, &ncid, dfile);
        if (dfile->state == DVL_FILE_SIM) {
            /* corner case: closing file while still being simulated. Here the 
            * key is the ncid of the metadata file */
            toclose = ncid;
            //ncid = dfile->ncid; 
        }else{
            toclose = dfile->ncid;
        }
    } else {
        // simulator lookup added. note: we cannot be sure whether there is a
        // descriptor in the hash table since we are tracking create but not open on this side.
        HASH_FIND_INT(dvl.open_files_idx, &ncid, dfile);
        if (NULL == dfile) {
            // must have been an opened restart file -> just close it
            return oncmpi_close(ncid);
        }

        // we have a tracked file here
        toclose = dfile->ncid;
    }

    // get cpath from dfile->path instead of nc_inq_path() lookup
    memcpy(cpath, dfile->path, strlen(dfile->path) + 1);
    DVLPRINT("DVL_NCMPI_CLOSE: cpath %s\n", cpath);

    // and we can delete the duplicated MPI_Info already here
    if (MPI_INFO_NULL != dfile->info) {
        MPI_Info_free(&(dfile->info));
    }

    int toclose_res = oncmpi_close(toclose);

    path = is_result_file(cpath, npath);
    if (path==NULL) return toclose_res;

    DVLPRINT("DVL_NCMPI_CLOSE: recognizes as result file\n");

    if (dvl.is_simulator){
        //loc = dvl_srv_mark_complete(&dvl, buff);

        // added check for rank 0
        if (0 == dfile->rank) {
            printf("DVL_NCMPI_CLOSE: rank 0; sending message\n");

            int64_t size = file_size(cpath);
            if (size < 0) {
                DVLPRINT("DVL_NC_CLOSE: could not determine file size of %s", cpath);
                size = 0;
            }

            MAKE_MESSAGE(buff, bsize, "%c:%i:%s:%li", DVL_MSG_FCLOSE_SIM, dvl.gni.myrank, path, size);
            if (bsize<0) return DVL_ERROR;
            
            DVLPRINT("DVL_NCMPI_CLOSE: simulator has closed %s (ncid: %i, gni_rank %i)\n", path, toclose, dvl.gni.myrank);

            dvl_send_message(buff, bsize, 1);
        }

        // added cleanup equally as for !is_simulator
        printf("freeing data strutures id: %i\n", dfile->id);
        /* free the file descriptor */
        dvl.open_files[dfile->id].trash_next = dvl.free_files;
        dvl.free_files=&(dvl.open_files[dfile->id]);
        HASH_DEL(dvl.open_files_idx, &(dvl.open_files[dfile->id]));
    } else {
        if (dfile->state == DVL_FILE_OPEN){ /* we are the client and an actual file is open (not meta) */
            // added check for rank 0
            if (0 == dfile->rank) {
                printf("DVL_NCMPI_CLOSE: rank 0; sending message\n");
                MAKE_MESSAGE(buff, bsize, "%c:%s", DVL_MSG_FCLOSE_CLIENT, path);

                DVLPRINT("DVL_NCMPI_CLOSE: client has closed %s (ncid: %i)\n", path, toclose);

                dvl_send_message(buff, bsize, 1);
            }
        }

        printf("freeing data strutures id: %i\n", dfile->id);
        /* free the file descriptor */
        dvl.open_files[dfile->id].trash_next = dvl.free_files;
        dvl.free_files=&(dvl.open_files[dfile->id]);
        HASH_DEL(dvl.open_files_idx, &(dvl.open_files[dfile->id]));
    }

    printf("to close: %i\n", toclose);

    return toclose_res;
}
