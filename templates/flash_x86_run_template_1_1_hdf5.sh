#!/bin/bash

# REMEMBER: NO outputs here except the DV_JOBID at the end

# hdf version for the simulator
export LD_LIBRARY_PATH=$$SDAVI_HDF5_1_8_PATH/lib:$$LD_LIBRARY_PATH
export LD_PRELOAD=$$DV_LIB_DIR/libdvlh8.so
export SIMULATOR_DIR=$$FLASH_RUN_DIR

export DV_SIMULATOR=1
export DV_JOBID=$jobid
export DV_GNI_ADDR=$gni_addr

export LSB_OUTFILE="flash_simulator"

# note: we need to cd to the simulator dir (see realpath() use within clientlib)
cd $$SIMULATOR_DIR

# Run flash in case directory

#real
$$SIMULATOR_DIR/flash4 -par_file $__PARFILE__ >$$SDAVI_DIR/logs/flash.$${DV_JOBID}.out 2>$$SDAVI_DIR/logs/flash.$${DV_JOBID}.err &

# signal end of job startup
echo -n $$DV_JOBID

# done
