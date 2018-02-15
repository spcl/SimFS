#define _DEFAULT_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <libgen.h> /* basename */
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __NCMPI__
#include <mpi.h>
#include "pnetcdf/dvl_ncmpi.h"

#elif __HDF5__
#include "hdf5/dvl_hdf5.h"

#else
// default netcdf
#include <netcdf.h>
#endif

#include "dvl.h"
#include "dvl_internal.h"
#include "dvl_proxy.h"


#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#ifdef __MT__
pthread_rwlock_t init_lock = PTHREAD_RWLOCK_INITIALIZER;
volatile int init = 0;
#else
int init = 0;
#endif

dvl_t dvl;

#ifdef __NCMPI__
int mpi_comm_avail=0;
MPI_Comm mpi_comm=0;
int mpi_rank=0;
#endif


int dvl_init(){

#ifdef __MT__
    // we need to check on init again after acquiring the write lock, of course
    if (pthread_rwlock_wrlock(&init_lock) != 0) fatal("dvl_init(): can't get init_lock");
    if (init) {
        pthread_rwlock_unlock(&init_lock);
        return DVL_SUCCESS;
    }
    // release the lock after initialization
#endif

    init = 1;

    char buff[BUFFER_SIZE];

    dvl.is_simulator = (getenv("DV_SIMULATOR")!=NULL && atoi(getenv("DV_SIMULATOR"))==1);  
   
    printf("DVL HELLO! Simulator: %i\n", dvl.is_simulator);


#ifdef BENCH
    char * lsb_name = getenv("LSB_NAME");
    if (lsb_name==NULL) lsb_name = "NA";

    char * client_type = getenv("CLIENT_TYPE");
    if (client_type==NULL) client_type = "NA";

    int slurmrank = 0;
    char * slurmid = getenv("SLURM_PROCID");
    if (slurmid==NULL) slurmid = "NA";
    else slurmrank = atoi(slurmid);
    
    int cache_size=0;
    char * cachesize_str = getenv("DV_FILECACHE_SIZE");
    if (cachesize_str!=NULL) cache_size = atoi(cachesize_str);

    int protected=1;
    char * protected_str = getenv("DV_FILECACHE_PROTECTED_MRUS");
    if (protected_str!=NULL) protected = atoi(protected_str);

    int max_simjobs=0;
    char * max_simjobs_str = getenv("MAX_SIMJOBS");
    if (max_simjobs_str!=NULL) max_simjobs = atoi(max_simjobs_str);

    char buffname[256];
    snprintf(buffname, 256, "%s.%s", lsb_name, slurmid);

    LSB_Init(buffname, 0);
  
    for (int i=0; i<DVL_NC_TOTOPS; i++) dvl.opcount[i] = 0;

    LSB_Set_Rparam_string("client_type", client_type);
    LSB_Set_Rparam_int("max_simjobs", max_simjobs);
    LSB_Set_Rparam_string("name", lsb_name);
    LSB_Set_Rparam_int("slurmrank", slurmrank);
    LSB_Set_Rparam_int("cache_size", cache_size);
    LSB_Set_Rparam_int("protected", protected);


    if (dvl.is_simulator) LSB_Set_Rparam_string("type", "simulator");
    else LSB_Set_Rparam_string("type", "client");
    LSB_Res();
#endif


#if __HDF5__
    dvl_H5_fill_originals_table(&dvl.h5originals);
#endif

    dvl.enabled = getenv(ENV_DISABLE)==NULL;
    dvl.to_terminate=0;

    if (getenv(ENV_JOBID)!=NULL) dvl.jobid = atoi(getenv(ENV_JOBID));    
    else dvl.jobid = -1;
    if (dvl.is_simulator && dvl.jobid==-1) {
        printf("Error. Jobid of simulator is invalid\n");
        exit(-1);
    }
    /* send hello msg */

#ifdef RDMA
    dvl.gni.addr = dvl_get_gni_nic_address(0x0);
#else
    dvl.gni.addr = 0;
#endif


#ifdef __NCMPI__
    // pnetcdf case: only rank 0 communicates with server and then broadcasts the respath to the other ranks, if possible
    // implementation follows the netcdf case below

    if (mpi_comm_avail) {
        MPI_Comm_rank(mpi_comm, &mpi_rank);

        if (0 == mpi_rank) {
            int len = snprintf(buff, BUFFER_SIZE, "%c:%u:%u", DVL_MSG_HELLO, dvl.gni.addr, dvl.jobid);

            dvl_send_message(buff, len, 0);
            dvl_recv_message(buff, BUFFER_SIZE, 1);

            printf("HELLO success for MPI rank 0! Message from DV server: %s; will broadcast\n", buff);

            char * buffptr = buff;

            char * respath = strsep(&buffptr, DVL_MSG_SEP);
            memcpy(dvl.respath, respath, MAX_FILE_NAME);

            char *checkpoint_path = strsep(&buffptr, DVL_MSG_SEP);
            memcpy(dvl.checkpoint_path, checkpoint_path, MAX_FILE_NAME);

            /* Reply is result_path:checkpoint_path:myrank if client OR
             * result_path:checkpoint_path:myrank if sim
             * note: initially planned result_path:checkpoint_path:myrank:client_nic_addr if sim 
             *       is not yet implemented (DV is not yet RDMA ready */
            uint32_t rank = atoi(strsep(&buffptr, DVL_MSG_SEP));
            if (dvl.is_simulator) dvl.gni.myrank = dvl.jobid;
            else dvl.gni.myrank = rank;
        }

        // broadcasts
        MPI_Bcast(dvl.respath, MAX_FILE_NAME, MPI_CHAR, 0, mpi_comm);
        MPI_Bcast(&(dvl.gni.myrank), 1, MPI_UNSIGNED, 0, mpi_comm);

        // output for control
        printf("HELLO from MPI rank %d. Got broadcast respath = %s, gni.myrank = %u\n", mpi_rank, dvl.respath, dvl.gni.myrank);

    } else {
        // fallback: identical to netcdf case
        int len = snprintf(buff, BUFFER_SIZE, "%c:%u:%u", DVL_MSG_HELLO, dvl.gni.addr, dvl.jobid);

        dvl_send_message(buff, len, 0);
        dvl_recv_message(buff, BUFFER_SIZE, 1);

        printf("HELLO success (using fallback)! Mes: %s;\n", buff);

        char * buffptr = buff;

        char * respath = strsep(&buffptr, DVL_MSG_SEP);
        memcpy(dvl.respath, respath, MAX_FILE_NAME);

        char *checkpoint_path = strsep(&buffptr, DVL_MSG_SEP);
        memcpy(dvl.checkpoint_path, checkpoint_path, MAX_FILE_NAME);

        /* Reply is result_path:checkpoint_path:myrank if client OR
         * result_path:checkpoint_path:myrank if sim
         * note: initially planned result_path:checkpoint_path:myrank:client_nic_addr if sim 
         *       is not yet implemented (DV is not yet RDMA ready */
        uint32_t rank = atoi(strsep(&buffptr, DVL_MSG_SEP));
        if (dvl.is_simulator) dvl.gni.myrank = dvl.jobid;
        else dvl.gni.myrank = rank;
    }

#ifdef RDMA
#warning ERROR: NCMPI IS NOT YET READY FOR RDMA
#endif


#else
    // netcdf and hdf5 cases
    int len = snprintf(buff, BUFFER_SIZE, "%c:%u:%u", DVL_MSG_HELLO, dvl.gni.addr, dvl.jobid);
   
    dvl_send_message(buff, len, 0);
    dvl_recv_message(buff, BUFFER_SIZE, 1);

    printf("HELLO success! Mes: %s;\n", buff);

    char * buffptr = buff;

    char * respath = strsep(&buffptr, DVL_MSG_SEP);
    memcpy(dvl.respath, respath, MAX_FILE_NAME);

    char *checkpoint_path = strsep(&buffptr, DVL_MSG_SEP);
    memcpy(dvl.checkpoint_path, checkpoint_path, MAX_FILE_NAME);

    /* Reply is result_path:checkpoint_path:myrank if client OR
     * result_path:checkpoint_path:myrank if sim
     * note: initially planned result_path:checkpoint_path:myrank:client_nic_addr if sim 
     *       is not yet implemented (DV is not yet RDMA ready */
    uint32_t rank = atoi(strsep(&buffptr, DVL_MSG_SEP));
    if (dvl.is_simulator) dvl.gni.myrank = dvl.jobid;
    else dvl.gni.myrank = rank;

#ifdef RDMA
    dvl_gni_init(&dvl.gni); 
    
    printf("GNI rank: %u\n", dvl.gni.myrank);
    if (dvl.is_simulator){
        if (getenv(ENV_GNI_ADDR)!=NULL){
            /* the addr of the client application that caused me */
            uint32_t peer_addr = atoi(getenv(ENV_GNI_ADDR));
            dvl_gni_connect(&dvl.gni, peer_addr); 
        }else{
            printf("Warning: no application nic address received!\n");
        } 
    } 
#endif //RDMA

#endif //__NCMPI__


    atexit(dvl_finalize);


    printf("Init data structures\n");

    /* initialize free file structures */
#ifdef __MT__
    if (pthread_rwlock_init(&dvl.open_files_lock, NULL) != 0) fatal("dvl_init(): can't create rwlock");
#endif    
    for (int i = 0; i < (MAX_OPEN_FILES - 1); i++){
        dvl.open_files[i].trash_next = &(dvl.open_files[i + 1]);
        dvl.open_files[i].id = i;
    }

    dvl.open_files[MAX_OPEN_FILES - 1].trash_next = NULL;
    dvl.free_files = dvl.open_files;
    dvl.open_files_idx = NULL;


    /* initialize redirect file structures */
    for (int i = 0; i < (MAX_REDIR_FILES - 1); i++) {
        dvl.redirected_files[i].trash_next = &(dvl.redirected_files[i + 1]);
        dvl.redirected_files[i].id = i;
    }

    dvl.redirected_files[MAX_REDIR_FILES - 1].trash_next = NULL;
    dvl.free_redirected_files = dvl.redirected_files;
    dvl.open_redirected_files_idx = NULL;

#ifdef __MT__
    /* initialize client tid rank mapping */
    if (pthread_rwlock_init(&dvl.client_tid_rank_mapping_lock, NULL) != 0) fatal("dvl_init(): can't create rwlock");
    for (int i = 0; i < (MAX_CLIENT_THREADS - 1); i++) {
        dvl.client_tid_rank_mapping[i].trash_next = &(dvl.client_tid_rank_mapping[i + 1]);
        dvl.client_tid_rank_mapping[i].id = i;
    }

    dvl.client_tid_rank_mapping[MAX_CLIENT_THREADS - 1].trash_next = NULL;
    dvl.free_client_tid_rank_mappings = dvl.client_tid_rank_mapping;
    dvl.used_client_tid_rank_mappings_idx = NULL;

    /* add the initial rank */
    if (!dvl.is_simulator) {
        int mapid = dvl.free_client_tid_rank_mappings->id;
        dvl.free_client_tid_rank_mappings = dvl.free_client_tid_rank_mappings->trash_next;
        dvl_tid_rank_mapping_t *map = &(dvl.client_tid_rank_mapping[mapid]);

        map->key = gettid();
        map->rank = dvl.gni.myrank;
        HASH_ADD_INT(dvl.used_client_tid_rank_mappings_idx, key, &(dvl.client_tid_rank_mapping[mapid]));
    }
#endif
    
    /* finalized setting: both only on simulator side at the moment */
    dvl.finalized = 0;
    dvl.open_files_count = 0;
    
    printf("DVL initialized!\n");

#ifdef __MT__
    pthread_rwlock_unlock(&init_lock);
#endif

    return DVL_SUCCESS;
}


#ifdef __MT__
/** additionally adds tid->rank association to the hashmap if not known yet.
 */
uint32_t dvl_get_current_rank() {
    if (dvl.is_simulator) {
        return dvl.gni.myrank;
    }

    pid_t tid = gettid();
    if (pthread_rwlock_rdlock(&dvl.client_tid_rank_mapping_lock) != 0) fatal("dvl_get_current_rank(): can't get rdlock");
    dvl_tid_rank_mapping_t *e;
    HASH_FIND_INT(dvl.used_client_tid_rank_mappings_idx, &tid, e);

    if (e != NULL) {
        uint32_t rank = e->rank;
        pthread_rwlock_unlock(&dvl.client_tid_rank_mapping_lock);
        return rank;
    }

    // send fresh hello message and get new rank for unknown tid
    pthread_rwlock_unlock(&dvl.client_tid_rank_mapping_lock);

    char buff[BUFFER_SIZE];
    int len = snprintf(buff, BUFFER_SIZE, "%c:%u:%u", DVL_MSG_HELLO, dvl.gni.addr, dvl.jobid);
    dvl_send_message(buff, len, 0);
    dvl_recv_message(buff, BUFFER_SIZE, 1);
    char *buffptr = buff;
    char *respath = strsep(&buffptr, DVL_MSG_SEP);
    // memcpy(dvl.respath, respath, MAX_FILE_NAME);
    uint32_t rank = atoi(strsep(&buffptr, DVL_MSG_SEP));

    if (pthread_rwlock_wrlock(&dvl.client_tid_rank_mapping_lock) != 0) fatal("dvl_get_current_rank(): can't get wrlock");
    int mapid = dvl.free_client_tid_rank_mappings->id;
    dvl.free_client_tid_rank_mappings = dvl.free_client_tid_rank_mappings->trash_next;
    dvl_tid_rank_mapping_t *map = &(dvl.client_tid_rank_mapping[mapid]);
    map->key = tid;
    map->rank = rank;
    HASH_ADD_INT(dvl.used_client_tid_rank_mappings_idx, key, &(dvl.client_tid_rank_mapping[mapid]));
    pthread_rwlock_unlock(&dvl.client_tid_rank_mapping_lock);
    return rank;    
}
#endif

void dvl_finalize(){
    char buff[BUFFER_SIZE]; 
    int msgsize = BUFFER_SIZE;

    if (dvl.finalized) return;    
    dvl.finalized = 1;

#ifdef __NCMPI__
    // also here: test whether it is possible to send only one message (see init)
    if (dvl.is_simulator){
        if (mpi_comm_avail) {
            // regular case: note MPI itself is no longer available at this time.
            // use same rank as during initialization (valid for having one node answering)
            if (0 == mpi_rank) {
                MAKE_MESSAGE(buff, msgsize, "%c:%i", DVL_MSG_FINALIZE, dvl.jobid);
                if (msgsize>0) dvl_send_message(buff, msgsize, 1);
            }
        } else {
            // fallback: all
            MAKE_MESSAGE(buff, msgsize, "%c:%i", DVL_MSG_FINALIZE, dvl.jobid);
            if (msgsize>0) dvl_send_message(buff, msgsize, 1);
        }
    }

#else
    // default
    if (dvl.is_simulator){
        MAKE_MESSAGE(buff, msgsize, "%c:%i", DVL_MSG_FINALIZE, dvl.jobid);
        if (msgsize>0) dvl_send_message(buff, msgsize, 1);        
    }
#endif


#ifdef BENCH
    LSB_Finalize();
#endif
    //return DVL_SUCCESS;
}


int64_t file_size(const char *path) {
    struct stat buffer;
    int ret = stat(path, &buffer);
    if (ret != 0) {
        return -1;
    }

    return (int64_t)buffer.st_size;
}

//--- COSMO specific part ------------------------------------------------------

char * is_result_file_COSMO(const char * path, char * npath){
    //char abspath[MAX_FILE_NAME];

    realpath(path, npath);

    DVLPRINT("path: %s; npath: %s; dvl.respath: %s\n", path, npath, dvl.respath);
    size_t rplen = strlen(dvl.respath);  
    
    DVLPRINT("is_result_file ok: %lu %lu %i\n", strlen(npath), rplen, strncmp(npath, dvl.respath, rplen));


    if (strlen(npath) >= rplen && !strncmp(npath, dvl.respath, rplen)){
        return npath + rplen; /* relapath return null-termianted string */
    }
    DVLPRINT("returning NULL!!!\n");

    return NULL; 
}


char * is_checkpoint_file_COSMO(const char * path, char * npath){
    //char abspath[MAX_FILE_NAME];

    realpath(path, npath);

    DVLPRINT("path: %s; npath: %s; dvl.checkpoint_path: %s\n", path, npath, dvl.checkpoint_path);
    size_t rplen = strlen(dvl.checkpoint_path);  
    
    DVLPRINT("is_checkpoint_file ok: %lu %lu %i\n", strlen(npath), rplen, strncmp(npath, dvl.checkpoint_path, rplen));


    if (strlen(npath) >= rplen && !strncmp(npath, dvl.checkpoint_path, rplen)){
        return npath + rplen; /* relapath return null-termianted string */
    }
    DVLPRINT("returning NULL!!!\n");

    return NULL; 
}



//--- FLASH Sedov specific part ------------------------------------------------

// additionally checks for proper suffix
char * is_result_file_FLASH(const char * path, char * npath){
    //char abspath[MAX_FILE_NAME];

    realpath(path, npath);

    DVLPRINT("path: %s; npath: %s; dvl.respath: %s\n", path, npath, dvl.respath);
    size_t rplen = strlen(dvl.respath);  

    // additional check for FLASH (note: basename may modify the input, thus a copy first)
    char name[MAX_FILE_NAME + 1];
    size_t namelen = strlen(npath);
    if (MAX_FILE_NAME <= namelen) {
        printf("name too long\n");
        return NULL;
    }
    memcpy(&name, npath, namelen + 1);
    name[MAX_FILE_NAME] = '\0';
    char *bn = basename(name);
    size_t bn_len = strlen(bn);

    // hardcoded check at the moment for suffix _plt_cnt_XXXX with XXXX being a 4 digit integer
    // and exclusion of infix _forced_
    char *rp = strstr(bn, "_plt_cnt_");
    char *fp = strstr(bn, "_forced_");
    if (NULL == rp) {
        DVLPRINT("not a result file\n");
        return NULL;
    }

    if (13 != bn_len - (rp - bn)) {
        DVLPRINT("not a result file\n");
        return NULL;
    }

    if (NULL != fp) {
        DVLPRINT("not a result file\n");
        return NULL;
    }

    // back to common code path
    DVLPRINT("is_result_file ok: %lu %lu %i\n", strlen(npath), rplen, strncmp(npath, dvl.respath, rplen));

    if (strlen(npath) >= rplen && !strncmp(npath, dvl.respath, rplen)){
        return npath + rplen; /* relapath return null-termianted string */
    }
    DVLPRINT("returning NULL!!!\n");

    return NULL; 
}


// additionally checks for proper suffix
char * is_checkpoint_file_FLASH(const char * path, char * npath){
    //char abspath[MAX_FILE_NAME];

    realpath(path, npath);

    DVLPRINT("path: %s; npath: %s; dvl.checkpoint_path: %s\n", path, npath, dvl.checkpoint_path);
    size_t rplen = strlen(dvl.respath);  

    // additional check for FLASH (note: basename may modify the input, thus a copy first)
    char name[MAX_FILE_NAME + 1];
    size_t namelen = strlen(npath);
    if (MAX_FILE_NAME <= namelen) {
        printf("name too long\n");
        return NULL;
    }
    memcpy(&name, npath, namelen + 1);
    name[MAX_FILE_NAME] = '\0';
    char *bn = basename(name);
    size_t bn_len = strlen(bn);

    // hardcoded check at the moment for suffix     _chk_XXXX with XXXX being a 4 digit integer
    char *cp = strstr(bn, "_chk_");
    if (NULL == cp) {
        DVLPRINT("not a checkpoint file\n");
        return NULL;
    }

    if (9 != bn_len - (cp - bn)) {
        DVLPRINT("not a checkpoint file\n");
        return NULL;
    }

    // back to common code path
    DVLPRINT("is_checkpoint_file ok: %lu %lu %i\n", strlen(npath), rplen, strncmp(npath, dvl.checkpoint_path, rplen));

    if (strlen(npath) >= rplen && !strncmp(npath, dvl.checkpoint_path, rplen)){
        return npath + rplen; /* relapath return null-termianted string */
    }
    DVLPRINT("returning NULL!!!\n");

    return NULL; 
}
