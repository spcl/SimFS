#!/bin/bash

module switch PrgEnv-cray PrgEnv-gnu 2> /dev/null

current_path=$(pwd)
code_source_path=$current_path/src/dv

if [ "$1" == "install" ]; then
    cp $current_path/build/bin/simfs /usr/bin/
    exit 0
fi

build_path=$current_path/build
workspace_path=$current_path/workspace
dvlib_path=$current_path/build/lib/
target_path=$current_path/build/bin

mkdir -p $target_path
mkdir -p $build_path
mkdir -p $workspace_path


if [ -z "$LUA_PATH" ]; then
    LUA_PATH=$current_path/lua
fi

export LUA_LIB_PATH=$LUA_PATH/lib
export LUA_INCLUDE_PATH=$LUA_PATH/include


echo "Compiling SimFS ($code_source_path -> $build_path)"
cd $build_path
cmake -DSIMFS_INSTALL_PATH=$target_path -DSIMFS_WORKSPACE_PATH=$workspace_path -DSIMFS_DVLIB_PATH=$dvlib_path $code_source_path
make -j 8 simfs
#make -j 8 stop_dv
#make -j 8 dv_status
make -j 8 check_dv_config_file


cd $current_path
#
echo "installing binaries to $target_path"
cp $build_path/simfs $target_path/
#cp $build_path/stop_dv $target_path/
#cp $build_path/dv_status $target_path/
cp $build_path/check_dv_config_file $target_path/

echo "Installing examples"
bash dv_config_files/configure.sh $current_path





echo "Done!"



