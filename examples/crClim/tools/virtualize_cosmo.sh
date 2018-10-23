#!/bin/bash

echo "SimFS context name:"
read context_name

echo "Path to COSMO SimFS context:"
read path

echo "Path to COSMO executable (lm_f90):"
read cosmo_exe

echo "Path to input/ folder (without trailing '/'): "
read sim_input

echo "Path to bc/ folder (without trailing '/'): "
read sim_bc

echo "Path to restarts/ folder (without trailing '/'): "
read restarts

echo "Path to output/ folder (without trailing '/'): "
read results

echo "Path to temporary files directory (leave blank for $SCRATCH/tmp):"
read tmp_dir

if [ -z $tmp_dir ]; then
  tmp_dir=$SCRATCH/tmp
fi

echo "SimFS configuration file template (leave blank for $SIMFS_DIR/dv_config_files/crCLIM/lmf.dv.template):"
read config_file_template

if [ -z $config_file ]; then
  config_file_template=$SIMFS_DIR/dv_config_files/crCLIM/lmf.dv.template
fi


echo "COSMO Job template file: (leave blank for $SIMFS_DIR/templates/crCLIM/lmf_template.job):"
read $cosmo_job_template

if [ -z $cosmo_job_template ]; then
  cosmo_job_template=$SIMFS_DIR/templates/crCLIM/lmf_template.job
fi

echo "Virtualizing COSMO"
echo "Context name: " $context_name
echo "Context path: " $path
echo "COSMO exe: " $cosmo_exe
echo "Path to input/ folder" $sim_input
echo "Path to bc/ folder" $sim_bc
echo "Path to restarts/ folder: " $restarts
echo "Path to output/ folder: " $results
echo "Temporary dir: " $tmp_dir
echo "SimFS template config file: " $config_file_template
echo "COSMO Job template file: " $cosmo_job_template

input_io=$path/INPUT_IO
input_io_template=$path/INPUT_IO_TEMPLATE

input_org=$path/INPUT_ORG
input_org_template=$path/INPUT_ORG_TEMPLATE


mkdir $path/runs > /dev/null 2>/dev/null
mkdir $tmp_dir > /dev/null 2>/dev/null

#sed -e "s?ydir='\([a-zA-Z0-9./_]*\)'?ydir='../../\1'?g" -e "s?ydirini='\([a-zA-Z0-9./_]*\)'?ydirini='../../\1'?g" -e "s?ydirbd='\([a-zA-Z0-9./_]*\)'?ydirbd='../../\1'?g" $input_io > $input_io_template
sed -e "s?ydir='\([a-zA-Z0-9./_]*\)'?ydir='${results}/\1'?g" -e "s?ydirini='\([a-zA-Z0-9./_]*\)'?ydirini='${sim_input}/\1'?g" -e "s?ydirbd='\([a-zA-Z0-9./_]*\)'?ydirbd='${sim_bc}/\1'?g" $input_io > $input_io_template

sed -e "s/hstart[ ]*=[ ]*[0-9]*/hstart=__START__/" -e "s/hstop[ ]*=[ ]*[0-9]*/hstop=__STOP__/" INPUT_ORG $input_org > $input_org_template


cat > $path/job << EOF_JOB
#!/bin/tcsh
#SBATCH --job-name=lm_f
#SBATCH --output=job.out
#SBATCH --nodes=100
#SBATCH --ntasks=106
#SBATCH --ntasks-per-core=1
#SBATCH --cpus-per-task=1
#SBATCH --partition=normal
#SBATCH --time=00:10:00
#SBATCH --account=pr04
#SBATCH --gres=gpu:1
#SBATCH --constraint=gpu
module load daint-gpu
setenv CRAY_CUDA_MPS 1
#SBATCH --gres=gpu:1
#SBATCH --constraint=gpu
module load daint-gpu
setenv CRAY_CUDA_MPS 1

# Initialization
set verbose
set echo

# set environmental parameters
setenv OMP_NUM_THREADS 1
setenv COSMO_NPROC_NODEVICE 4
setenv MALLOC_MMAP_MAX_ 0
setenv MALLOC_TRIM_THRESHOLD_ 536870912
setenv MV2_ENABLE_AFFINITY 0
setenv MV2_USE_CUDA 1
setenv MPICH_RDMA_ENABLED_CUDA 1
setenv MPICH_G2G_PIPELINE 256
setenv CRAY_CUDA_PROXY 0
setenv CUDA_VA_RESERVATION_CHUNK_SIZE 4294967296


#to be able to run it after the June 13, 2018 upgrade
setenv CRAY_TCMALLOC_MEMFS_FORCE 1

\rm -rf YU* OUTPUT fort.* M_* core script_*.grc stdeo_*.grc grc.nqs *.tex

date
# Run LM in case directory
simfs $context_name simulate srun -u $cosmo_exe
date

# remove job tag (if present)
\rm -f .jobid >&/dev/null

# done
EOF_JOB

start_date=$(cat $path/INPUT_ORG| grep ydate_ini | cut -d "'" -f 2)

sed -e "s%?STARTDATE%$start_date%g" -e "s%?CONFIG_PATH%$path%g" -e "s%?RESTART_PATH%$restarts/restarts/%g" -e "s%?RESULT_PATH%$results/output/%g" -e "s%?TMP_PATH%$tmp_dir%g" -e "s%?JOB_TEMPLATE%${cosmo_job_template}%g" $config_file_template > $path/simfs_config.dv

simfs $context_name init $path/simfs_config.dv

