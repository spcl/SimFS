#ifndef __DVL_INTERNAL_H__
#define __DVL_INTERNAL_H__

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef __MT__

/**
 * Note on this multi-threading (MT) aware variant of DVLib:
 * This feature allows an association of access trajectories to individual threads (determined
 * by pid_t gettid(void) ) to improve heuristic decisions in prefetching made by DV.
 * Internal shared data structures will be protected using rw locks to limit the performance hit.
 * This functionality is provided in a separate DVLib instance libdvlmt.so that is compatible with libdvl.so.
 * Recommendation: use libdvlmt.so only for clients that need it; use libdvl.so for the simulator.

 * Please note: NetCDF itself is **not** thread safe.
 * It keeps various global states re the open file. Thus, any multi-threaded client application
 * using the C, Fortran or Python interface of NetCDF has to solve that problem (e.g. serializing
 * access to files). Our library does not provide a solution to that.
 */

#include <pthread.h>


#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
// reference for this macro (gettid() is not provided by glibc stdlib)
// https://stackoverflow.com/questions/30680550/c-gettid-was-not-declared-in-this-scope?answertab=votes#tab-top
#endif

#include "ht/uthash.h"


#ifdef __NCMPI__
#include <mpi.h>
#include <pnetcdf.h>
#include "pnetcdf/pnetcdf_bind.h"

    #ifdef __MT__
    #error Multithreaded clients are not supported yet for PNetCDF
    #endif


#elif __HDF5__
#include <H5Fpublic.h>
#include <H5Dpublic.h>
#include <H5Ipublic.h>
#include <H5Opublic.h>
#include <H5Ppublic.h>
#include "hdf5/dvl_hdf5.h"

    #ifdef __MT__
    #error Multithreaded clients are not supported yet for HDF5
    #endif


#else
// default netcdf
#include <netcdf.h>
#include "netcdf_bind.h"
#endif


#define DEBUG 1
#ifdef DEBUG
#define DVLPRINT(format, ...) printf(format,  ##__VA_ARGS__)
#else
#define DVLPRINT 
#endif

#define fatal(str) { fprintf(stderr, str); exit(1); }

/* MAX_FILE_NAME + MAX_VAR_NAME + reserve for offset/length lists <= BUFFER_SIZE (dvl_proxy.h)
   also 2 * MAX_FILE_NAME + 3 <= BUFFER_SIZE (see hello message)
*/
#define MAX_FILE_NAME 2040
#define MAX_VAR_NAME 512
#define MAX_OPEN_FILES 1024

#define MAX_REDIR_FILES 1024

#ifdef __MT__
#define MAX_CLIENT_THREADS 128
#endif

#define DVL_FILE_OPEN 0
#define DVL_FILE_SIM 1

#define DVL_REPLY_FILE_OPEN '0'
#define DVL_REPLY_FILE_SIM '1'
#define DVL_REPLY_RDMA '2'

#define DVL_CREATE_REPLY_ACK '0'
#define DVL_CREATE_REPLY_KILL '1'
#define DVL_CREATE_REPLY_REDIRECT '2'

#define DVL_MSG_HELLO '0'
#define DVL_MSG_FOPEN '1'
#define DVL_MSG_FCLOSE_CLIENT '2'
#define DVL_MSG_VGET '3'
#define DVL_MSG_FCLOSE_SIM '4'
#define DVL_MSG_VPUT '5'
#define DVL_MSG_FCREATE '6'
#define DVL_MSG_FINALIZE '7'

#define DVL_MSG_FCREATE_CHECKPOINT '8'

#define DVL_MSG_EXTENDED_API 'E'

// reserved command letters:
// X -> reserved for stop_dv command
// S -> reserved for dv_status command  

#define DVL_MSG_SEP ":"

#define DVL_ABORT { exit(-1); }


#define ENV_JOBID "DV_JOBID"
#define ENV_DISABLE "DV_DISABLE"


#define DVL_FILETYPE_UNKNOWN 0
#define DVL_FILETYPE_RESULT 1
#define DVL_FILETYPE_CHECKPOINT 2


#ifdef __WRITE_NO_DATA_DURING_REDIRECT__
    #define DVL_PREVENT_WRITING_DATA_DURING_REDIRECT (-10000)
#endif


#ifdef BENCH
    #ifdef __MT__
    #error Multithreaded clients are not supported yet for LibLSB benchmarking
    #endif

    #include "liblsb.h"
    #define DVL_BENCH(ID) { LSB_Set_Rparam_int("ct", dvl.opcount[ID]++); LSB_Check(ID); }

    //workaround until new liblsb available
    //#define DVL_BENCH(ID) { LSB_Set_Rparam_int("ct", dvl.opcount[ID]++); }
#else
    #define DVL_BENCH(ID) 
#endif

//BENCH is what we used for collecting the data for the paper. For production
//now give the option to collect some "simpler" profiling info
#ifdef PROFILE 
    #ifdef __MT__
    #error Multithreaded clients are not supported yet for LibLSB profiling
    #endif

    #include "liblsb.h"
    #define DVL_PROFILE(ID, op_name, file_name) { \
        LSB_Set_Rparam_string("op_name", op_name); \
        LSB_Set_Rparam_int("op_count", dvl.opcount[ID]++); \
        LSB_Check(ID); }
#else
    #define DVL_PROFILE(ID, op_name, file_name) {}
#endif


#ifdef __HDF5__
#define DVL_CHECK_BENCH_ID_FOLLOWS { if (!init) { dvl_init();}; if (!dvl.enabled) {return 0;} }
#endif


#ifdef __MT__
// MT variants sets a local variable mt_rank that can be used later in the functions
// but it must be have been declared before
#define DVL_CHECK(ID) { \
    if (pthread_rwlock_rdlock(&init_lock) != 0) fatal("DVL_CHECK(): can't get init_lock"); \
    if (!init) { pthread_rwlock_unlock(&init_lock); dvl_init(); }; \
    pthread_rwlock_unlock(&init_lock); \
    mt_rank = dvl_get_current_rank(); \
    DVL_BENCH(ID); \
    if (!dvl.enabled) {return 0;} \
}

#define DVL_CHECK_WITHOUT_BENCH { \
    if (pthread_rwlock_rdlock(&init_lock) != 0) fatal("DVL_CHECK_WITHOUT_BENCH(): can't get init_lock"); \
    if (!init) { pthread_rwlock_unlock(&init_lock); dvl_init(); }; \
    pthread_rwlock_unlock(&init_lock); \
    mt_rank = dvl_get_current_rank(); \
    if (!dvl.enabled) {return 0;} \
}

#else
#define DVL_CHECK(ID) { \
    if (!init) { dvl_init();}; \
    DVL_BENCH(ID); \
    if (!dvl.enabled) {return 0;} \
}

#define DVL_CHECK_WITHOUT_BENCH { \
    if (!init) { dvl_init();}; \
    if (!dvl.enabled) {return 0;} \
}
#endif


#ifdef __NCMPI__
#define DVL_CHECK_WITH_COMM(ID, COMM) { mpi_comm_avail=1; mpi_comm=COMM; DVL_CHECK(ID); } 
#endif

#define DVL_NC_TOTOPS 5
#define DVL_NC_OPEN_ID 0
#define DVL_NC_CREATE_ID 1
#define DVL_NC_PUT_ID 2
#define DVL_NC_GET_ID 3
#define DVL_NC_CLOSE_ID 4

/* note: RDMA is activated by compiler flag -DRDMA at the moment. See compile_daint.sh
#ifdef __NCMPI__
#warning NOTE: RDMA NOT YET IMPLEMENTED FOR NCMPI. DEACTIVATED
#else
// default
#define RDMA
#endif
*/


// include for RDMA needed in both cases: with and without
#define ENV_GNI_ADDR "DV_APP_GNI_ADDR"
#include "dvl_rdma.h"


/* note re storage location of MPI_Comm and rank:
   In general, a typical application may only open one comm and thus have one rank,
   which would indicate storing it in dvl_t. However, to be more generic to any
   kind of client/simulator application, comm and rank are stored separately
   for each file.

   If we realized that an individual node may open the same file multiple times in
   parallel using different comm modes and ranks, we would have to get more complex
   by using a list or better hash table to store the association comm -> rank.
*/

/* the organization for HDF5 is a bit more complicated for the client side (with meta files)
   since both fid values must be kept in the hash map, for the meta file and the actual file
   (for files that start with a meta file).
   Both must be kept in the list until closing of the file since any of the available close
   methods (close, decrement) may be called with either of these ids.
   See implementation for details.
   The server side (without meta files) is straight forward. At the moment, server side
   files are not even stored into the hashmap (except in NCMPI mode).
*/

/* see also additional association of these problems with the MT aware variant
*/


typedef struct dvl_file {
#ifdef __HDF5__
    hid_t key;        // key == fid
    hid_t other_key;  // actual fid for is_meta; meta fid for !is_meta
    hid_t fid_to_use; // represents the unified abstraction layer; this may change from key to other_fid as it is appropriate
    unsigned flags;
    hid_t access_plist;
    int is_meta;
#else
    int ncid;
    int meta_toclose;
    int key;
    int omode;
#endif

    int state;
    int id;
    char path[MAX_FILE_NAME];
    struct dvl_file * trash_next;

#ifdef __NCMPI__
    MPI_Comm comm;
    int rank;
    MPI_Info info; // will hold a duplicate of the info -> thus needs to be cleared in close()
#endif
    
    UT_hash_handle hh;
} dvl_file_t;


typedef struct dvl_redirected_file {
#ifdef __HDF5__
    hid_t key;      // key == fid (note: v1.8 and v1.10 have hid_t of different sizes; thus 2 different libdvlh wrapper libraries)
#else
    int key;        // key == ncid
#endif

    int id;
    char opath[MAX_FILE_NAME];
    char path[MAX_FILE_NAME];
    int file_type;
    struct dvl_redirected_file *trash_next;

#ifdef __NCMPI__
    // TODO: check whether needed here, too, or data in dvl_file_t sufficient
    MPI_Comm comm;
    int rank;
    MPI_Info info; // will hold a duplicate of the info -> thus needs to be cleared in close()
#endif

    UT_hash_handle hh;
} dvl_redirected_file_t;


#ifdef __MT__
typedef struct dvl_tid_rank_mapping {
    pid_t key;      // key == tid
    uint32_t rank;  // see dvl.gni.myrank; may have to be replaced by a full gni_conn_t if RDMA gets activated
    int id;
    struct dvl_tid_rank_mapping *trash_next;
    UT_hash_handle hh;
} dvl_tid_rank_mapping_t;
#endif


typedef struct _dvl {
    // no global lock needed for dvl data structure
    // - most is read-only after init (which is lock protected)
    // - finalized: only used by simulator -> no locks at the moment
    // - open_files_count: only used by simulator -> no locks at the moment
    // - open_files and client_tid_rank_mapping have rw locks
    // - redirected_files is only on simulator side -> no locking there at the moment
    // - opcount: BENCH is not (yet) supported -> will need locking, too then
    // - gni: only read only at the moment; RDMA is not active at the moment
    //
    // all these things must be checked again later!

    // read only after init
    uint32_t jobid;
    uint8_t is_simulator;
    uint8_t enabled;

    // only used on simulator side at the moment
    uint8_t to_terminate; 
    uint8_t finalized;
    uint32_t open_files_count;
    
    // hashmap to handle .meta files (mainly client side, but for pnetcdf also simulator side)
#ifdef __MT__
    pthread_rwlock_t open_files_lock;
#endif
    dvl_file_t open_files[MAX_OPEN_FILES];
    dvl_file_t * free_files;
    dvl_file_t * open_files_idx;

    // hashmap to handle redirected files (see simulator side)
    // no lock here since only client side is considering multiple threads at the moment
    dvl_redirected_file_t redirected_files[MAX_REDIR_FILES];
    dvl_redirected_file_t * free_redirected_files;
    dvl_redirected_file_t * open_redirected_files_idx;

#ifdef __MT__
    // hashmap to handle client tid to rank mapping
    pthread_rwlock_t client_tid_rank_mapping_lock;
    dvl_tid_rank_mapping_t client_tid_rank_mapping[MAX_CLIENT_THREADS];
    dvl_tid_rank_mapping_t * free_client_tid_rank_mappings;
    dvl_tid_rank_mapping_t * used_client_tid_rank_mappings_idx;
#endif

    // base path for result files
    char respath[MAX_FILE_NAME];

    // the same for checkpoint files
    char checkpoint_path[MAX_FILE_NAME];

#if defined(BENCH) || defined(PROFILE)
    uint32_t opcount[DVL_NC_TOTOPS];
#endif

    // is now needed in both cases: RDMA and without
    gni_conn_t gni;

#ifdef __NCMPI__
    oncmpi_open_t ncmpiopen; // deliberately a separete name since signature differs
#elif __HDF5__
    dvl_hdf5_originals_t h5originals;
#else
    onc_open_t ncopen;
#endif
} dvl_t;


#ifdef __MT__
extern pthread_rwlock_t init_lock;
extern volatile int init;
#else
extern int init;
#endif


extern dvl_t dvl;

#ifdef __NCMPI__
extern int mpi_comm_avail;
extern MPI_Comm mpi_comm;
extern int mpi_rank;
#endif

/**
 * uses stat() to determine file size
 * returns file size in bytes, or -1 in case of error reported by stat()
 */
int64_t file_size(const char *path);

char * is_result_file(const char * path, char * npath);

char * is_checkpoint_file(const char * path, char * npath);

#endif /* __DVL_INTERNAL_H__ */
