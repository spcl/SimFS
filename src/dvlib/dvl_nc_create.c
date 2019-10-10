#define _GNU_SOURCE
#include <string.h>

#include <assert.h>
#include <signal.h>

#include "dvl.h"
#include "dvl_internal.h"
#include "dvl_proxy.h"

/*
int dvl_nc_open(char *path, int omode, int * ncidp){
    return _dvl_nc_open(path, omode, ncidp, nc_open);
}
*/

int dvl_nc_create(char * opath, int omode, int * ncidp, onc_create_t onc_ocreate){

    char buff[BUFFER_SIZE];        
    char npath[MAX_FILE_NAME];

    int file_type = DVL_FILETYPE_UNKNOWN;

#ifdef __MT__
    uint32_t mt_rank = 0;
#endif
    DVL_CHECK(DVL_NC_CREATE_ID);

    printf("CREATING!\n");   

    /* Check if this file is relevant to us */
    char * path = is_result_file(opath, npath);
    if (path == NULL) {
        path = is_checkpoint_file(opath, npath);
        if (path == NULL) {
            return (*onc_ocreate)(opath, omode, ncidp);
        } else {
            file_type = DVL_FILETYPE_CHECKPOINT;
        }
    } else {
        file_type = DVL_FILETYPE_RESULT;
    }
 

    
    int redirected = 0;
    char *create_path = opath;

    if (dvl.is_simulator){

        if (dvl.finalized){
            fprintf(stderr, "Warning: this simulation is suppose to terminate but it's trying to create %s.\n", opath);
            return NC_ENOMEM;
        }


        if (dvl.to_terminate && dvl.open_files_count==0) {
            dvl_finalize(); // will set dvl.finalized=1
            raise(SIGINT);
            //return proper error code. Alternative: redirect to /tmp/ or somewhere else.
            return NC_ENOMEM;
        }

        int msgsize = BUFFER_SIZE;

        if (file_type == DVL_FILETYPE_RESULT) {
            // result message as before
            MAKE_MESSAGE(buff, msgsize, "%c:%i:%s", DVL_MSG_FCREATE, dvl.gni.myrank, path);
        } else if (file_type == DVL_FILETYPE_CHECKPOINT) {
            // new checkpoint message
            MAKE_MESSAGE(buff, msgsize, "%c:%i:%s", DVL_MSG_FCREATE_CHECKPOINT, dvl.gni.myrank, path);
        } else {
            fatal("ERROR: unknown file type\n");
        }

        if (msgsize<0) return DVL_ERROR;
        printf("CREATING MESSAGE!\n");   

        dvl_send_message(buff, msgsize, 0);

        // fix for race condition
        // simulator result file creation is now synchronous with DV
        dvl_recv_message(buff, BUFFER_SIZE, 1);

        if (buff[0] == DVL_CREATE_REPLY_KILL) {
            //need MACROs, etc..
            //FIXME: need reference counting. raise() should be called only when there are no other open files open.           
            dvl.to_terminate = 1;

            if (dvl.open_files_count == 0) {
                dvl_finalize();
                raise(SIGINT);
                //return proper error code. Alternative: redirect to /tmp/ or somewhere else.
                return NC_ENOMEM;
            }
        } else {
            dvl.open_files_count++;

            if (buff[0] == DVL_CREATE_REPLY_REDIRECT) {
                // at least a small security check that the path is a /0 terminated string
                char *redir_path = &buff[2];
                size_t max_len = BUFFER_SIZE - 4;
                size_t redir_len = strnlen(redir_path, max_len);
                if (redir_len < max_len) {
                    printf("CREATE of %s is redirected to %s\n", opath, redir_path);
                    create_path = redir_path;
                    redirected = 1;                    
                } else {
                    printf("ERROR: redir path was longer than available buffer size. Check buffer size and/or networking protocol. Using original path that will overwrite the existing file\n");
                }
            }
        }
    }

    int retval = (*onc_ocreate)(create_path, omode, ncidp);

    if (retval != NC_NOERR) {
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

    rfile->key = *ncidp;

    size_t pathlen = strlen(opath);
    if (pathlen >= MAX_FILE_NAME){
        printf("Error: file name too long: %s\n", opath);
        DVL_ABORT;
    }
    memcpy(rfile->opath, opath, pathlen + 1);

    pathlen = strlen(path);
    memcpy(rfile->path, path, pathlen + 1);

    rfile->file_type = file_type;

    HASH_ADD_INT(dvl.open_redirected_files_idx, key, &(dvl.redirected_files[redir_fileid]));

    return retval;
}
