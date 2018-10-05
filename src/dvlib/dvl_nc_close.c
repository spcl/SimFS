#define _DEFAULT_SOURCE

#include "dvl.h"
#include "dvl_internal.h"
#include "dvl_proxy.h"

/*
int _dvl_nc_close(int id, onc_close_t onc_close); 


int dvl_nc_close(int ncid){
    return _dvl_nc_close(ncid, nc_close); 
}*/

int _dvl_nc_close(int id, onc_close_t onc_close){
    char buff[BUFFER_SIZE];
    char cpath[MAX_FILE_NAME];
    char *cpath_ptr = cpath; // to allow optional redirection

    char npath[MAX_FILE_NAME];
    char * path;
    size_t pathlen;
    int bsize = BUFFER_SIZE;

#ifdef __MT__
    uint32_t mt_rank = 0;
#endif
    DVL_CHECK(DVL_NC_CLOSE_ID);

    int toclose = id;

    dvl_file_t * dfile = NULL;

    if (dvl.is_simulator) {
        // simulator

        if (dvl.finalized) {
            fprintf(stderr, "Warning: this simulation is suppose to terminate but it's trying to close a file.\n");
            return NC_EBADID;
        }

    } else {
        // client

#ifdef __MT__
        if (pthread_rwlock_rdlock(&dvl.open_files_lock) != 0) fatal("_dvl_nc_close(): can't get rdlock");
#endif
        HASH_FIND_INT(dvl.open_files_idx, &id, dfile);
        if (dfile == NULL) {
            fprintf(stderr, "Warning: client is closing a file (id: %i) that was not open by DVLIB. Trying to close it with netcdf (hint: opening/creating function may have not been intercepted!).\n", id);
#ifdef __MT__
            pthread_rwlock_unlock(&dvl.open_files_lock);
#endif
            return (*onc_close)(id);     
        }


        if (dfile->state == DVL_FILE_SIM) {
            /* corner case: closing file while still being simulated. Here the 
            * key is the ncid of the metadata file */
            toclose = id;
        } else {
            toclose = dfile->ncid;
        }
#ifdef __MT__
        pthread_rwlock_unlock(&dvl.open_files_lock);
#endif        

    }

    nc_inq_path(toclose, &pathlen, NULL);
    if (pathlen >= MAX_FILE_NAME){
        printf("Not enough space to build the PUT message\n");
        DVL_ABORT;
    }
    nc_inq_path(toclose, NULL, cpath);

    int toclose_res = (*onc_close)(toclose);

    if (dvl.is_simulator) {
        dvl.open_files_count--;
    }


    // for simulators and in case of redirected files, the path used for messaging with DV
    // must be retrieved from the hashmap. The path recovered by nc_inq_path() refers to redirected file
    // opath/cpath is needed, too, to get file_size later
    dvl_redirected_file_t *rfile = NULL;
    path = NULL;
    int is_redirected = 0;
    int file_type = DVL_FILETYPE_UNKNOWN;
    if (dvl.is_simulator) {
        
        HASH_FIND_INT(dvl.open_redirected_files_idx, &id, rfile);

        if (rfile != NULL) {
            is_redirected = 1;
            cpath_ptr = rfile->opath;
            path = rfile->path;
            file_type = rfile->file_type;
        }
    }

    if (path == NULL) {
        path = is_result_file(cpath, npath);
        if (path != NULL) {
            file_type = DVL_FILETYPE_RESULT;
        }
    }

    // note: we are not sending close messages for closed checkpoint files
    // but redirected checkpoint files have to be removed from the hashmap
    // and in this case, path is already set.

    if (path == NULL) {
        return toclose_res;
    }

    if (dvl.is_simulator) {
        //loc = dvl_srv_mark_complete(&dvl, buff);

        // messages are only sent for result files but not for checkpoint files
        if (file_type == DVL_FILETYPE_RESULT) {
            int64_t size = file_size(cpath_ptr);
            if (size < 0) {
                DVLPRINT("DVL_NC_CLOSE: could not determine file size of %s", cpath_ptr);
                size = 0;
            }

            MAKE_MESSAGE(buff, bsize, "%c:%i:%s:%li", DVL_MSG_FCLOSE_SIM, dvl.gni.myrank, path, size);

            if (bsize<0) return DVL_ERROR;
        
            dvl_send_message(buff, bsize, 1);
        }

        // free data structures for redirected files (both, results and checkpoints)
        if (is_redirected) {
            dvl.redirected_files[rfile->id].trash_next = dvl.free_redirected_files;
            dvl.free_redirected_files = &(dvl.redirected_files[rfile->id]);
            HASH_DEL(dvl.open_redirected_files_idx, &(dvl.redirected_files[rfile->id]));
        }

    } else {

#ifdef __MT__
        if (pthread_rwlock_wrlock(&dvl.open_files_lock) != 0) fatal("_dvl_nc_close(): can't get wrlock");
        dvl_file_t *dfile_check = NULL;
        HASH_FIND_INT(dvl.open_files_idx, &id, dfile_check);
        if (dfile != dfile_check) {
            fprintf(stderr, "ERROR client calling _dvl_nc_close(): dfile structure removed by other thread before sending message (i.e. 2 close calls for the same file)");
            pthread_rwlock_unlock(&dvl.open_files_lock);
            return toclose_res;
        }
#endif
        if (dfile->state == DVL_FILE_OPEN){ /* we are the client and an actual file is open (not meta) */

            MAKE_MESSAGE(buff, bsize, "%c:%s", DVL_MSG_FCLOSE_CLIENT, path);

            DVLPRINT("DVL_NC_CLOSE: closing %s (ncid: %i)\n", path, toclose);

            dvl_send_message(buff, bsize, 1);
            
            if (dfile->meta_toclose != -1){
                (*onc_close)(dfile->meta_toclose);
            }
        }

        printf("freeing data strutures id: %i\n", dfile->id);
        /* free the file descriptor */
        dvl.open_files[dfile->id].trash_next = dvl.free_files;
        dvl.free_files = &(dvl.open_files[dfile->id]);
        HASH_DEL(dvl.open_files_idx, &(dvl.open_files[dfile->id]));
#ifdef __MT__
        pthread_rwlock_unlock(&dvl.open_files_lock);
#endif        
    }


    printf("to close: %i\n", toclose);

    return toclose_res;

}
