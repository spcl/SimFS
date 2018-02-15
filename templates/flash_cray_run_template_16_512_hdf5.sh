#!/bin/bash

# REMEMBER: NO outputs here except the DVL_JOBID at the end

# hdf version for the simulator
module unload cray-hdf5
module load cray-hdf5/1.8.16
export LD_PRELOAD=/path/lib/libdvlh8.so
export SIMULATOR_DIR=$$FLASH_RUN_DIR

export DVL_SIMULATOR=1
export DVL_JOBID=$jobid
export DVL_GNI_ADDR=$gni_addr

export LSB_OUTFILE="flash_simulator"

# note: we need to cd to the simulator dir (see realpath() use within clientlib)
cd $$SIMULATOR_DIR

# Run flash in case directory

#real
srun -u -n 512 -N 16 --ntasks-per-node 32 -C mc $$SIMULATOR_DIR/flash4 -par_file $__PARFILE__ >$$SDAVI_DIR/logs/flash.$${DVL_JOBID}.out 2>$$SDAVI_DIR/logs/flash.$${DVL_JOBID}.err &

# signal end of job startup
echo -n $$DVL_JOBID

# done
