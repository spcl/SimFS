/* hdf5 create following the netcdf version
 * v1.2 2017-09-20 Pirmin Schmid
*/

#ifndef __HDF5__
#define __HDF5__
#endif

#define _GNU_SOURCE
#include <string.h>

#include <signal.h>

#include "../dvl.h"
#include "../dvl_internal.h"
#include "../dvl_proxy.h"

#include "dvl_hdf5.h"

hid_t H5Fcreate(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id) {
    char buff[BUFFER_SIZE];        
    char npath[MAX_FILE_NAME];

    int file_type = DVL_FILETYPE_UNKNOWN;

    DVL_CHECK(DVL_NC_CREATE_ID);

    /* Check if this file is relevant to us */
    char * path = is_result_file(name, npath);
    if (path == NULL) {
        path = is_checkpoint_file(name, npath);
        if (path == NULL) {
            return dvl.h5originals.h5fcreate(name, flags, fcpl_id, fapl_id);
        } else {
            file_type = DVL_FILETYPE_CHECKPOINT;
        }
    } else {
        file_type = DVL_FILETYPE_RESULT;
    }
 
    printf("CREATING!\n");

    int redirected = 0;
    char *create_path = (char *) name;
    // explicit cast to express awareness of "discards ‘const’ qualifier from pointer target type"
    // we assure in the code below that data in *name is not modified
    // create_path is only use to allow redirection if needd (see below)

    if (dvl.is_simulator) {

        if (dvl.finalized){
            fprintf(stderr, "Warning: this simulation is suppose to terminate but it's trying to create %s.\n", name);
            return HDF5_FAIL;
        }

        if (dvl.to_terminate && dvl.open_files_count==0) {
            dvl_finalize(); // will set dvl.finalized=1
            raise(SIGINT);
            //return proper error code. Alternative: redirect to /tmp/ or somewhere else.
            return HDF5_FAIL;
        }

        int msgsize = BUFFER_SIZE;

        if (file_type == DVL_FILETYPE_RESULT) {
            // result message as before
            MAKE_MESSAGE(buff, msgsize, "%c:%i:%s", DVL_MSG_FCREATE, dvl.gni.myrank, path);
        } else if (file_type == DVL_FILETYPE_CHECKPOINT) {
            // new checkpoint message
            MAKE_MESSAGE(buff, msgsize, "%c:%i:%s", DVL_MSG_FCREATE_CHECKPOINT, dvl.gni.myrank, path);
        }

        if (msgsize < 0) {
            return DVL_ERROR;
        }
        printf("CREATING MESSAGE!\n");   

        dvl_send_message(buff, msgsize, 0);

        // fix for race condition
        // simulator result file creation is now synchronous with DV
        dvl_recv_message(buff, BUFFER_SIZE, 1);

        printf("received message: %s\n", buff);

        if (buff[0] == DVL_CREATE_REPLY_KILL) {
            //need MACROs, etc..
            //FIXME: need reference counting. raise() should be called only when there are no other open files open.           
            dvl.to_terminate = 1;

            if (dvl.open_files_count == 0) {
                dvl_finalize();
                raise(SIGINT);
                //return proper error code. Alternative: redirect to /tmp/ or somewhere else.
                return HDF5_FAIL;
            }
        } else {
            dvl.open_files_count++;

            if (buff[0] == DVL_CREATE_REPLY_REDIRECT) {
                // at least a small security check that the path is a /0 terminated string
                char *redir_path = &buff[2];
                size_t max_len = BUFFER_SIZE - 4;
                size_t redir_len = strnlen(redir_path, max_len);
                if (redir_len < max_len) {
                    printf("CREATE of %s is redirected to %s\n", name, redir_path);
                    create_path = redir_path;
                    redirected = 1;                    
                } else {
                    printf("ERROR: redir path was longer than available buffer size. Check buffer size and/or networking protocol. Using original path that will overwrite the existing file\n");
                }
            }
        }

    } else {
        // we do not care about files created on client side
    }

    hid_t retval = dvl.h5originals.h5fcreate(create_path, flags, fcpl_id, fapl_id);

    if (retval < 0) {
        // error
        return retval;
    }

    if (!dvl.is_simulator) {
        return retval;
    }

    if(!redirected) {
        return retval;
    }

    // handle redirect hash map for simulator: result files and checkpoint files
    int redir_fileid = dvl.free_redirected_files->id;
    dvl.free_redirected_files = dvl.free_redirected_files->trash_next;
    dvl_redirected_file_t *rfile = &(dvl.redirected_files[redir_fileid]);

    rfile->key = retval;

    size_t pathlen = strlen(name);
    if (pathlen >= MAX_FILE_NAME){
        printf("Error: file name too long: %s\n", name);
        DVL_ABORT;
    }
    memcpy(rfile->opath, name, pathlen + 1);

    pathlen = strlen(path);
    memcpy(rfile->path, path, pathlen + 1);

    rfile->file_type = file_type;

    HASH_ADD_INT(dvl.open_redirected_files_idx, key, &(dvl.redirected_files[redir_fileid]));

    return retval;
}
