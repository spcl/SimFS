#!/bin/bash

cpath=$1
sed -e "s#__SIMFS_DIR__#$cpath#g" $cpath/dv_config_files/heatequation.dv.in > $cpath/dv_config_files/heatequation.dv 
sed -e "s#__SIMFS_DIR__#$cpath#g" $cpath/examples/heatequation/simulator/job_template.sh.in > $cpath/examples/heatequation/simulator/job_template.sh 

mkdir -p $cpath/examples/heatequation/restarts
mkdir -p $cpath/examples/heatequation/output

