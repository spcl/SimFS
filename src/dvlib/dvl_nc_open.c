#define _DEFAULT_SOURCE

#include "dvl.h"
#include "dvl_internal.h"
#include "dvl_proxy.h"

/*
int dvl_nc_open(char *path, int omode, int * ncidp){
    return _dvl_nc_open(path, omode, ncidp, nc_open);
}
*/

int _dvl_nc_open(char * opath, int omode, int * ncidp, onc_open_t onc_open){

    char buff[BUFFER_SIZE];        
    char npath[MAX_FILE_NAME];

#ifdef __MT__
    uint32_t mt_rank = 0;
#endif
    DVL_CHECK(DVL_NC_OPEN_ID);


    dvl.ncopen = onc_open;


    //printf("open!! %s\n", opath);

    /* Check if this file is relevant to us */
    char * path = is_result_file(opath, npath);
    if (path==NULL) return (*onc_open)(opath, omode, ncidp);


    if (!dvl.is_simulator) {

        //printf("DVL OPEN!\n");

#ifdef __MT__
        if (pthread_rwlock_wrlock(&dvl.open_files_lock) != 0) fatal("_dvl_nc_open(): can't get wrlock");
#endif
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
        memcpy(dfile->path, path, pathlen + 1);

#ifdef __MT__
        pthread_rwlock_unlock(&dvl.open_files_lock);
        // avoid having the data structure locked over the entire communication time with DV
#endif


#ifdef BENCH
        //LSB_Set_Rparam_str("restart", simstart);
        //LSB_Res();
#endif

        /* send request to the dvl */
        int msgsize = BUFFER_SIZE;

#ifdef __MT__
        MAKE_MESSAGE(buff, msgsize, "%c:%s:%i:file=%s;gni_addr=%u", DVL_MSG_FOPEN, path, mt_rank, path, dvl.gni.addr);
#else
        MAKE_MESSAGE(buff, msgsize, "%c:%s:%i:file=%s;gni_addr=%u", DVL_MSG_FOPEN, path, dvl.gni.myrank, path, dvl.gni.addr);
#endif


        if (msgsize<0) return DVL_ERROR;
        dvl_send_message(buff, msgsize, 0);
    
        /* recv response */
        dvl_recv_message(buff, BUFFER_SIZE, 1);

        int res; /* it has to be a valid ncid*/
        int is_meta=0;

        if (buff[0]==DVL_REPLY_FILE_SIM){
            printf("[DVLIB] File is not avail. Opening meta: %s\n", buff+1);
            res = (*onc_open)(buff+1, omode, ncidp); 
            if (res!=NC_NOERR){
                printf("Fake metadata file not found --> waiting for real data\n");
        
                /*send fake get message to get the notification */
                MAKE_MESSAGE(buff, msgsize, "%c:%s:%i:%i:", DVL_MSG_VGET, dfile->path, 0, dvl.gni.myrank);

                //printf("sending (%i): %s\n", msgsize, buff);
                if (msgsize<0) return DVL_ERROR;
                

                dvl_send_message(buff, msgsize, 0);
            
                /* wait for notification */
                dvl_recv_message(buff, BUFFER_SIZE, 1);
            }else{
                is_meta=1;
                dfile->state = DVL_FILE_SIM;
                dfile->meta_toclose = *ncidp; 
            }
        }

        if (!is_meta){
            res = (*onc_open)(opath, omode, ncidp);
            dfile->state = DVL_FILE_OPEN;
            dfile->meta_toclose = -1;
        }

        dfile->ncid = *ncidp;
        dfile->key = *ncidp;
        dfile->omode = omode;


#ifdef __MT__
        if (pthread_rwlock_wrlock(&dvl.open_files_lock) != 0) fatal("_dvl_nc_open(): can't get wrlock");
#endif

        HASH_ADD_INT(dvl.open_files_idx, key, &(dvl.open_files[fileid]));
         
        DVLPRINT("[DVLIB] DVL_NC_OPEN: %s; fileid: %i; idx cnt: %u; res: %i; key: %i\n", path, fileid, HASH_COUNT(dvl.open_files_idx), res, *ncidp); 

#ifdef __MT__
        pthread_rwlock_unlock(&dvl.open_files_lock);
#endif
       

        return res; 
    }else{ /* we don't care about the simulator */
        /*int msgsize = BUFFER_SIZE;
        MAKE_MESSAGE(buff, msgsize, "%c:%s", DVL_MSG_FOPEN_SIM, path);
        if (msgsize<0) return DVL_ERROR;

        dvl_send_message(buff, msgsize, 1);
        */
        printf("SIM OPEN!\n");
        return (*onc_open)(opath, omode, ncidp);
    }
}
