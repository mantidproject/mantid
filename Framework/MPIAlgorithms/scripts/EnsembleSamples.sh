#!/bin/bash
#PBS -l nodes=12:ppn=4
#PBS -l walltime=30:00
#PBS -N NOMAD_Reduction

let first=5418
let last=5423
let can=5426
let van=5428

let total_tasks=$(wc -l $PBS_NODEFILE | awk '{print $1}')
export OMP_NUM_THREADS=192/$total_tasks
cd $PBS_O_WORKDIR
export LD_LIBRARY_PATH=/shared/openmpi/gcc/lib:/home/vel/Mantid/Code/mpi-build/lib:/home/tr9/mantid-deps 
export PYTHONPATH=/home/vel/Mantid/Code/mpi-build/bin:/home/tr9/mantid-deps/site-packages:/home/vel/Mantid/Code/Mantid/Framework/PythonInterface/plugins/algorithms

let tasks_per_job=$total_tasks/2
let second_start=$tasks_per_job+1
rm NOM_$can.nxs  NOM_$van.nxs
sed -n 1,${tasks_per_job}'p' < $PBS_NODEFILE >nodefilev
sed -n ${second_start},${total_tasks}'p' < $PBS_NODEFILE >nodefilec
time /shared/openmpi/gcc/bin/mpirun -machinefile nodefilev -np $tasks_per_job -x OMP_NUM_THREADS -x LD_LIBRARY_PATH -x PYTHONPATH python NOM1.py 0 -1 $van  >& jobv.out &
time /shared/openmpi/gcc/bin/mpirun -machinefile nodefilec -np $tasks_per_job -x OMP_NUM_THREADS -x LD_LIBRARY_PATH -x PYTHONPATH python NOM1.py 0 $can -1  >& jobc.out &
wait

let num_jobs=$last-$first+1
let tasks_per_job=$total_tasks/$num_jobs
for i in $(seq $first $last)
do
    #write hostfile for i-th job to use
    let lstart=($i-$first)*${tasks_per_job}+1
    let lend=${lstart}+${tasks_per_job}-1
    sed -n ${lstart},${lend}'p' < $PBS_NODEFILE >nodefile$i

    time /shared/openmpi/gcc/bin/mpirun -machinefile nodefile$i -np $tasks_per_job -x OMP_NUM_THREADS -x LD_LIBRARY_PATH -x PYTHONPATH python NOM1.py $i $can $van  >& job$i.out &

done

wait

