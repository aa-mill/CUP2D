#!/usr/bin/bash
#SBATCH -n 1024
#SBATCH -t 0-5:00
#SBATCH -p shared
#SBATCH --mem-per-cpu 750

module load gcc
module load openmpi
module load hdf5
module load python
module load gsl

export MPICXX=mpic++
export HDF5_ROOT=/n/sw/helmod-rocky8/apps/MPI/gcc/13.2.0-fasrc01/openmpi/4.1.5-fasrc03/hdf5/1.14.2-fasrc01

cd ../makefiles
make -j4
cd ../launch
./launchDiskAMR.sh 25k
