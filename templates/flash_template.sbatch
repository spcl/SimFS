#!/bin/bash -l
#SBATCH --job-name="flash_pnetcdf_$__APPID__"
#SBATCH --time=00:05:00
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --ntasks-per-node=4
#SBATCH --constraint=mc
#SBATCH --output=sbatch_out/flash_pnetcdf_$__APPID__.%j.o
#SBATCH --error=sbatch_out/flash_pnetcdf_$__APPID__.%j.e

# this template file currently needs these substitutions
# __APPID__ -> number
# __PARFILE__ -> parameter file with path

# starts a basic flash simulation
# organization for given $$SCRATCH scratch folder
# $$SCRATCH/run/ contains flash binary config file name.par, flash outputs and also this batch script
# $$SCRATCH/run/sbatch_out/ will contain the stdio/stderr outputs

#======START======
module switch PrgEnv-cray PrgEnv-gnu
module load cray-parallel-netcdf

echo "SLURN_NODES: $$SLURM_NODES"
echo "SLURM_NTASKS: $$SLURM_NTASKS"
echo "SLURM_CPUS_PER_TASK: $$SLURM_CPUS_PER_TASK"
echo "SLURM_NTASKS_PER_NODE: $$SLURM_NTASKS_PER_NODE"

export DVL_SIMULATOR=1

srun -C mc -n $$SLURM_NTASKS --ntasks-per-node $$SLURM_NTASKS_PER_NODE ./flash4 $__PARFILE__
#======END======
