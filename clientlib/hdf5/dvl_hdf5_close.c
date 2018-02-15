/* hdf5 close following the netcdf version
 *
 * note: HDF5 has a particular functionality re close.
 * files can be closed by close but also by other means with reference count going to 0
 * and only if refcount is actually == 0, the file will close.
 *
 * v1.1 2017-04-27 Pirmin Schmid
*/

#ifndef __HDF5__
#define __HDF5__
#endif

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "../dvl.h"
#include "../dvl_internal.h"
#include "../dvl_proxy.h"

#include "dvl_hdf5.h"


herr_t H5Fclose(hid_t file_id) {
    DVL_CHECK_BENCH_ID_FOLLOWS;
    // note: actual close is not yet decided. Thus: the bench ID part follows later
    // to avoid false logging data
    //DVL_CHECK(DVL_NC_CLOSE_ID);

    return dvl_hdf5_handle_file_close(file_id, CLOSE_BY_H5FCLOSE);
}

// note: global variable dvl must be initialized
static const char dvl_hdf5_call_original_close_error_msg[] = "dvl_hdf5_handle_file_close() is calling a close function that is not yet defined.\n";
static inline dvl_hdf5_herr_or_int_t dvl_hdf5_call_original_close(hid_t file_id, dvl_hdf5_file_close_t type) {
    switch (type) {
        case CLOSE_BY_H5FCLOSE:
            // int will map to herr_t
            return dvl.h5originals.h5fclose(file_id);
            break;

        case CLOSE_BY_H5IDEC_REF:
            // int is ok.
            return dvl.h5originals.h5idec_ref(file_id);
            break;

        default:
            fprintf(stderr, dvl_hdf5_call_original_close_error_msg);
            DVL_ABORT;
    }
}

// note: global variable dvl must be initialized
dvl_hdf5_herr_or_int_t dvl_hdf5_handle_file_close(hid_t file_id, dvl_hdf5_file_close_t type) {
    char cpath[MAX_FILE_NAME];
    char *cpath_ptr = cpath;

    hid_t toclose = file_id;

    dvl_file_t *dfile = NULL;

    if (dvl.is_simulator) {

        // check here matches the code flow of the netcdf implementation
        if (dvl.finalized) {
            fprintf(stderr, "Warning: this simulation is suppose to terminate but it's trying to close a file.\n");
            return HDF5_FAIL;
        }

        // simulator side is not kept in a hash map at the moment since there is no
        // special meta file handling -> file_id is always correct
        dvl_H5_filename_associated_with_id(toclose, cpath, MAX_FILE_NAME);
    } else {
        // use lookup on client side
        HASH_FIND_INT(dvl.open_files_idx, &file_id, dfile);

        if (dfile == NULL) {
            fprintf(stderr, "dvl_hdf5_handle_file_close(): Could not find file id %li in hash map.\n", (long) file_id);
            return dvl_hdf5_call_original_close(toclose, type);
        }

        if (dfile->state == DVL_FILE_SIM) {
            /* corner case: closing file while still being simulated. Here the 
            * key is the ncid of the metadata file */
            toclose = file_id;
        } else {
            toclose = dfile->fid_to_use;
            // note: thanks to the abstraction layer, this works from potentially both hash map entries:
            // actual file id entry and meta file entry
        }

        // no name lookup here, we have to use the name stored in the hashmap
        // and extend it with the given respath to get a normalized path again
        snprintf(cpath, MAX_FILE_NAME, "%s%s", dvl.respath, dfile->path);
    }

    // note: only if the refCount gets to 0, the file will actually close
    int refCount = H5Iget_ref(toclose);

    if (dvl.is_simulator && 1 <= refCount) {
        // use flush when we are really closing to avoid problems
        // see http://stackoverflow.com/questions/31287744/corrupt-files-when-creating-hdf5-files-without-closing-them-h5py
        H5Fflush(toclose, H5F_SCOPE_GLOBAL);
    }

    // call of original function
    int toclose_res = dvl_hdf5_call_original_close(toclose, type);

    // adjust refCount
    if (0 <= toclose_res) {
        refCount--;
    }

    DVLPRINT("dvl_hdf5_handle_file_close: fid %li, name %s, refcount %i %s\n", (long) toclose, cpath,
        refCount, refCount == 0 ? ", sending close message to DVL server" : "");

    // we do not care yet if refcount is still > 0
    if (0 < refCount) {
        return toclose_res;
    }


    //--- this is now the part that matches NetCDF code flow -------------------
    DVL_BENCH(DVL_NC_CLOSE_ID);

    char buff[BUFFER_SIZE];
    char npath[MAX_FILE_NAME];
    int bsize = BUFFER_SIZE;

    // actual DVL handling
    if (dvl.is_simulator){
        // messages only sent for result files
        // redirect check and handling hashmap for result- and checkpoint files
        // 1) lookup in redirect hashmap 2) if not found: check whether file is result file
        dvl.open_files_count--;
        int file_type = DVL_FILETYPE_UNKNOWN;
        char *path = NULL;
        dvl_redirected_file_t *rfile = NULL;
        int is_redirected = 0;

        HASH_FIND_INT(dvl.open_redirected_files_idx, &toclose, rfile);

        if (rfile != NULL) {
            is_redirected = 1;
            cpath_ptr = rfile->opath;
            path = rfile->path;
            file_type = rfile->file_type;
        } else {
            path = is_result_file(cpath, npath);
            if (path != NULL) {
                file_type = DVL_FILETYPE_RESULT;
            }
        }

        if (path == NULL) {
            return toclose_res;
        }        

        if (file_type == DVL_FILETYPE_RESULT) {
            int64_t size = file_size(cpath_ptr); // returning file size of the original file if redirected
            if (size < 0) {
                DVLPRINT("DVL_NC_CLOSE: could not determine file size of %s", cpath_ptr);
                size = 0;
            }

            MAKE_MESSAGE(buff, bsize, "%c:%i:%s:%li", DVL_MSG_FCLOSE_SIM, dvl.gni.myrank, path, size);

            if (bsize < 0) {
                fprintf(stderr, "dvl_hdf5_handle_file_close(): ERROR while building the message.\n");
                return toclose_res; // this should not block the entire simulator, actual file close was OK.
            }
        
            dvl_send_message(buff, bsize, 1);
            // no reply expected
        }

        if (is_redirected) {
            dvl.redirected_files[rfile->id].trash_next = dvl.free_redirected_files;
            dvl.free_redirected_files = &(dvl.redirected_files[rfile->id]);
            HASH_DEL(dvl.open_redirected_files_idx, &(dvl.redirected_files[rfile->id]));
        }

    } else {
        // we only send messages for result files
        char *path = is_result_file(cpath, npath);
        if (path == NULL) {
            return toclose_res;
        }

        // to distinguish whether the file was open, we need to look at both hashmap entries
        // additionally, we have to close also the meta file if it was opened
        // note: the same closing procedure is used as is generally used by the application
        int is_open = dfile->state == DVL_FILE_OPEN;

        if (dfile->is_meta) {
            DVLPRINT("closing the meta file.\n");
            dvl_hdf5_call_original_close(dfile->key, type);
        }

        hid_t other_key = dfile->other_key;
        dvl_file_t *other_dfile = NULL;

        if (0 <= other_key) {
            HASH_FIND_INT(dvl.open_files_idx, &other_key, other_dfile);

            if (other_dfile != NULL) {
                // it's a file meta-file combo
                is_open = is_open || (other_dfile->state == DVL_FILE_OPEN);
                if (other_dfile->is_meta) {
                    DVLPRINT("closing the meta file.\n");
                    dvl_hdf5_call_original_close(dfile->key, type);
                }
            }
        }

        if (is_open) {
            /* we are the client and an actual file is open (not meta) */
            MAKE_MESSAGE(buff, bsize, "%c:%s", DVL_MSG_FCLOSE_CLIENT, path);

            DVLPRINT("DVL_HDF5_CLOSE: closing %s\n", path);

            if (bsize < 0) {
                DVLPRINT("dvl_hdf5_handle_file_close(): ERROR while building the message.\n");
                fprintf(stderr, "dvl_hdf5_handle_file_close(): ERROR while building the message.\n");
                return toclose_res; // this should not block the entire client, actual file close was OK.
            }

            dvl_send_message(buff, bsize, 1);

            // no reply is expected
        }

        DVLPRINT("freeing data strutures fid: %li, id: %i, name: %s, is_meta: %d\n",
            (long) dfile->key, dfile->id, dfile->path, dfile->is_meta);
        /* free the file descriptor */
        dvl.open_files[dfile->id].trash_next = dvl.free_files;
        dvl.free_files=&(dvl.open_files[dfile->id]);
        HASH_DEL(dvl.open_files_idx, &(dvl.open_files[dfile->id]));

        if (other_dfile != NULL) {
            DVLPRINT("freeing 2nd associated data strutures fid: %li, id: %i, name: %s, is_meta: %d\n",
                (long) other_dfile->key, other_dfile->id, other_dfile->path, other_dfile->is_meta);
            /* free the file descriptor */
            dvl.open_files[other_dfile->id].trash_next = dvl.free_files;
            dvl.free_files=&(dvl.open_files[other_dfile->id]);
            HASH_DEL(dvl.open_files_idx, &(dvl.open_files[other_dfile->id]));
        }
    }

    DVLPRINT("to close: %li\n", (long) toclose);
    return toclose_res;
}
