#!/bin/bash

cpath=$1
sed -e "s#__SIMFS_DIR__#$cpath#g" $cpath/dv_config_files/heatequation.dv.in > $cpath/dv_config_files/heatequation.dv 

