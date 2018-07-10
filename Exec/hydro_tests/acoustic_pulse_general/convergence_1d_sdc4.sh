#!/bin/bash

mpiexec -n 8 ./Castro1d.gnu.MPI.ex inputs.64 castro.sdc_order=4 castro.time_integration_method=2 castro.fourth_order=1
mpiexec -n 8 ./Castro1d.gnu.MPI.ex inputs.128 castro.sdc_order=4 castro.time_integration_method=2 castro.fourth_order=1
mpiexec -n 8 ./Castro1d.gnu.MPI.ex inputs.256 castro.sdc_order=4 castro.time_integration_method=2 castro.fourth_order=1
mpiexec -n 8 ./Castro1d.gnu.MPI.ex inputs.512 castro.sdc_order=4 castro.time_integration_method=2 castro.fourth_order=1

RichardsonConvergenceTest1d.gnu.ex coarFile=acoustic_pulse_64_plt00201 mediFile=acoustic_pulse_128_plt00401 fineFile=acoustic_pulse_256_plt00801 mediError=med.out coarError=coar.out > convergence_1d_sdc4.1.out
RichardsonConvergenceTest1d.gnu.ex coarFile=acoustic_pulse_128_plt00401 mediFile=acoustic_pulse_256_plt00801 fineFile=acoustic_pulse_512_plt01601 mediError=med.out coarError=coar.out > convergence_1d_sdc4.2.out

