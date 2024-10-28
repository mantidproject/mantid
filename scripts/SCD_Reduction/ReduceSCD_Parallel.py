# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

# File: ReduceSCD_Parallel.py
#
# Version 2.0, modified to work with Mantid's new python interface.
#
# This script will run multiple instances of the script ReduceSCD_OneRun.py
# in parallel, using either local processes or a slurm partition.  After
# using the ReduceSCD_OneRun script to find, index and integrate peaks from
# multiple runs, this script merges the integrated peaks files and re-indexes
# them in a consistent way.  If desired, the indexing can also be changed to a
# specified conventional cell.
# Many intermediate files are generated and saved, so all output is written
# to a specified output_directory.  This output directory must be created
# before running this script, and must be specified in the configuration file.
# The user should first make sure that all parameters are set properly in
# the configuration file for the ReduceSCD_OneRun.py script, and that that
# script will properly reduce one scd run.  Once a single run can be properly
# reduced, set the additional parameters in the configuration file that specify
# how the list of runs will be processed in parallel.
#

#
# _v1: December 3rd 2013. Mads Joergensen
# This version now includes the possibility to use the 1D cylindrical integration method
# and the possibility to load a UB matrix which will be used for integration of the individual
# runs and to index the combined file (Code from Xiapoing).
#

#
# _v2: December 3rd 2013. Mads Joergensen
# Adds the possibility to optimize the loaded UB for each run for a better peak prediction
# It is also possible to find the common UB by using lattice parameters of the first
# run or the loaded matirix instead of the default FFT method
#

import os
import sys
import threading
import time
import ReduceDictionary

sys.path.append("/opt/mantidnightly/bin")
# sys.path.append("/opt/Mantid/bin")

from mantid import apiVersion, FileFinder
from mantid.simpleapi import (
    CombinePeaksWorkspaces,
    CreatePeaksWorkspace,
    FindUBUsingFFT,
    FindUBUsingLatticeParameters,
    IndexPeaks,
    Load,
    LoadEventNexus,
    LoadIsawPeaks,
    LoadIsawUB,
    SaveNexus,
    SaveIsawPeaks,
    SaveIsawUB,
    SelectCellOfType,
)

print("API Version")
print(apiVersion())

start_time = time.time()

# -------------------------------------------------------------------------
# ProcessThread is a simple local class.  Each instance of ProcessThread is
# a thread that starts a command line process to reduce one run.
#


class ProcessThread(threading.Thread):
    command = ""

    def setCommand(self, command=""):
        self.command = command

    def run(self):
        print("STARTING PROCESS: " + self.command)
        os.system(self.command)


# -------------------------------------------------------------------------


#
# Get the config file name from the command line
#
if len(sys.argv) < 2:
    print("You MUST give the config file name on the command line")
    exit(0)

config_files = sys.argv[1:]

#
# Load the parameter names and values from the specified configuration file
# into a dictionary and set all the required parameters from the dictionary.
#

params_dictionary = ReduceDictionary.LoadDictionary(*config_files)

exp_name = params_dictionary["exp_name"]
output_directory = params_dictionary["output_directory"]
output_nexus = params_dictionary.get("output_nexus", False)
reduce_one_run_script = params_dictionary["reduce_one_run_script"]
slurm_queue_name = params_dictionary["slurm_queue_name"]
max_processes = int(params_dictionary["max_processes"])
min_d = params_dictionary["min_d"]
max_d = params_dictionary["max_d"]
tolerance = params_dictionary["tolerance"]
cell_type = params_dictionary["cell_type"]
centering = params_dictionary["centering"]
allow_perm = params_dictionary["allow_perm"]
run_nums = params_dictionary["run_nums"]
data_directory = params_dictionary["data_directory"]

use_cylindrical_integration = params_dictionary["use_cylindrical_integration"]
instrument_name = params_dictionary["instrument_name"]

read_UB = params_dictionary["read_UB"]
UB_filename = params_dictionary["UB_filename"]
UseFirstLattice = params_dictionary["UseFirstLattice"]
num_peaks_to_find = params_dictionary["num_peaks_to_find"]

# determine what python executable to launch new jobs with
python = sys.executable
if python is None:  # not all platforms define this variable
    python = "python"

#
# Make the list of separate process commands.  If a slurm queue name
# was specified, run the processes using slurm, otherwise just use
# multiple processes on the local machine.
#
procList = []
index = 0
for r_num in run_nums:
    procList.append(ProcessThread())
    cmd = "%s %s %s %s" % (python, reduce_one_run_script, " ".join(config_files), str(r_num))
    if slurm_queue_name is not None:
        console_file = output_directory + "/" + str(r_num) + "_output.txt"
        cmd = "srun -p " + slurm_queue_name + " --cpus-per-task=3 -J ReduceSCD_Parallel.py -o " + console_file + " " + cmd
    procList[index].setCommand(cmd)
    index = index + 1

#
# Now create and start a thread for each command to run the commands in parallel,
# starting up to max_processes simultaneously.
#
all_done = False
active_list = []
while not all_done:
    if len(procList) > 0 and len(active_list) < max_processes:
        thread = procList[0]
        procList.remove(thread)
        active_list.append(thread)
        thread.start()
    time.sleep(2)
    for thread in active_list:
        if not thread.is_alive():
            active_list.remove(thread)
    if len(procList) == 0 and len(active_list) == 0:
        all_done = True

print("\n**************************************************************************************")
print("************** Completed Individual Runs, Starting to Combine Results ****************")
print("**************************************************************************************\n")

#
# First combine all of the integrated files, by reading the separate files and
# appending them to a combined output file.
#
niggli_name = output_directory + "/" + exp_name + "_Niggli"
if output_nexus:
    niggli_integrate_file = niggli_name + ".nxs"
else:
    niggli_integrate_file = niggli_name + ".integrate"
niggli_matrix_file = niggli_name + ".mat"

first_time = True

if output_nexus:
    # Only need this for instrument for peaks_total
    short_filename = "%s_%s" % (instrument_name, str(run_nums[0]))
    if data_directory is not None:
        full_name = data_directory + "/" + short_filename + ".nxs.h5"
        if not os.path.exists(full_name):
            full_name = data_directory + "/" + short_filename + "_event.nxs"
    else:
        candidates = FileFinder.findRuns(short_filename)
        full_name = ""
        for item in candidates:
            if os.path.exists(item):
                full_name = str(item)

        if not full_name.endswith("nxs") and not full_name.endswith("h5"):
            print("Exiting since the data_directory was not specified and")
            print("findnexus failed for event NeXus file: " + instrument_name + " " + str(run_nums[0]))
            exit(0)

    #
    # Load the first data file to find instrument
    #
    wksp = LoadEventNexus(Filename=full_name, FilterByTofMin=0, FilterByTofMax=0)
    peaks_total = CreatePeaksWorkspace(NumberOfPeaks=0, InstrumentWorkspace=wksp)

if not use_cylindrical_integration:
    for r_num in run_nums:
        if output_nexus:
            one_run_file = output_directory + "/" + str(r_num) + "_Niggli.nxs"
            peaks_ws = Load(Filename=one_run_file)
        else:
            one_run_file = output_directory + "/" + str(r_num) + "_Niggli.integrate"
            peaks_ws = LoadIsawPeaks(Filename=one_run_file)
        if first_time:
            if UseFirstLattice and not read_UB:
                # Find a UB (using FFT) for the first run to use in the FindUBUsingLatticeParameters
                FindUBUsingFFT(PeaksWorkspace=peaks_ws, MinD=min_d, MaxD=max_d, Tolerance=tolerance)
                uc_a = peaks_ws.sample().getOrientedLattice().a()
                uc_b = peaks_ws.sample().getOrientedLattice().b()
                uc_c = peaks_ws.sample().getOrientedLattice().c()
                uc_alpha = peaks_ws.sample().getOrientedLattice().alpha()
                uc_beta = peaks_ws.sample().getOrientedLattice().beta()
                uc_gamma = peaks_ws.sample().getOrientedLattice().gamma()
            if output_nexus:
                peaks_total = CombinePeaksWorkspaces(LHSWorkspace=peaks_total, RHSWorkspace=peaks_ws)
                SaveNexus(InputWorkspace=peaks_ws, Filename=niggli_integrate_file)
            else:
                SaveIsawPeaks(InputWorkspace=peaks_ws, AppendFile=False, Filename=niggli_integrate_file)
            first_time = False
        else:
            if output_nexus:
                peaks_total = CombinePeaksWorkspaces(LHSWorkspace=peaks_total, RHSWorkspace=peaks_ws)
                SaveNexus(InputWorkspace=peaks_total, Filename=niggli_integrate_file)
            else:
                SaveIsawPeaks(InputWorkspace=peaks_ws, AppendFile=True, Filename=niggli_integrate_file)

    #
    # Load the combined file and re-index all of the peaks together.
    # Save them back to the combined Niggli file (Or selected UB file if in use...)
    #
    if output_nexus:
        peaks_ws = Load(Filename=niggli_integrate_file)
    else:
        peaks_ws = LoadIsawPeaks(Filename=niggli_integrate_file)

    #
    # Find a Niggli UB matrix that indexes the peaks in this run
    # Load UB instead of Using FFT
    # Index peaks using UB from UB of initial orientation run/or combined runs from first iteration of crystal orientation refinement
    if read_UB:
        LoadIsawUB(InputWorkspace=peaks_ws, Filename=UB_filename)
        if UseFirstLattice:
            # Find UB using lattice parameters from the specified file
            uc_a = peaks_ws.sample().getOrientedLattice().a()
            uc_b = peaks_ws.sample().getOrientedLattice().b()
            uc_c = peaks_ws.sample().getOrientedLattice().c()
            uc_alpha = peaks_ws.sample().getOrientedLattice().alpha()
            uc_beta = peaks_ws.sample().getOrientedLattice().beta()
            uc_gamma = peaks_ws.sample().getOrientedLattice().gamma()
            FindUBUsingLatticeParameters(
                PeaksWorkspace=peaks_ws,
                a=uc_a,
                b=uc_b,
                c=uc_c,
                alpha=uc_alpha,
                beta=uc_beta,
                gamma=uc_gamma,
                NumInitial=num_peaks_to_find,
                Tolerance=tolerance,
            )
        # OptimizeCrystalPlacement(PeaksWorkspace=peaks_ws,ModifiedPeaksWorkspace=peaks_ws,
        #                         FitInfoTable='CrystalPlacement_info',MaxIndexingError=tolerance)
    elif UseFirstLattice and not read_UB:
        # Find UB using lattice parameters using the FFT results from first run if no UB file is specified
        FindUBUsingLatticeParameters(
            PeaksWorkspace=peaks_ws,
            a=uc_a,
            b=uc_b,
            c=uc_c,
            alpha=uc_alpha,
            beta=uc_beta,
            gamma=uc_gamma,
            NumInitial=num_peaks_to_find,
            Tolerance=tolerance,
        )
    else:
        FindUBUsingFFT(PeaksWorkspace=peaks_ws, MinD=min_d, MaxD=max_d, Tolerance=tolerance)

    IndexPeaks(PeaksWorkspace=peaks_ws, Tolerance=tolerance)
    if output_nexus:
        SaveNexus(InputWorkspace=peaks_ws, Filename=niggli_integrate_file)
    else:
        SaveIsawPeaks(InputWorkspace=peaks_ws, AppendFile=False, Filename=niggli_integrate_file)
    SaveIsawUB(InputWorkspace=peaks_ws, Filename=niggli_matrix_file)

#
# If requested, also switch to the specified conventional cell and save the
# corresponding matrix and integrate file
#
if not use_cylindrical_integration:
    if (cell_type is not None) and (centering is not None):
        conv_name = output_directory + "/" + exp_name + "_" + cell_type + "_" + centering
        if output_nexus:
            conventional_integrate_file = conv_name + ".nxs"
        else:
            conventional_integrate_file = conv_name + ".integrate"
        conventional_matrix_file = conv_name + ".mat"

        SelectCellOfType(
            PeaksWorkspace=peaks_ws, CellType=cell_type, Centering=centering, AllowPermutations=allow_perm, Apply=True, Tolerance=tolerance
        )
        if output_nexus:
            SaveNexus(InputWorkspace=peaks_ws, Filename=conventional_integrate_file)
        else:
            SaveIsawPeaks(InputWorkspace=peaks_ws, AppendFile=False, Filename=conventional_integrate_file)
        SaveIsawUB(InputWorkspace=peaks_ws, Filename=conventional_matrix_file)

if use_cylindrical_integration:
    if (cell_type is not None) or (centering is not None):
        print("WARNING: Cylindrical profiles are NOT transformed!!!")
    # Combine *.profiles files
    filename = output_directory + "/" + exp_name + ".profiles"
    outputFile = open(filename, "w")

    # Read and write the first run profile file with header.
    r_num = run_nums[0]
    filename = output_directory + "/" + instrument_name + "_" + r_num + ".profiles"
    inputFile = open(filename, "r")
    file_all_lines = inputFile.read()
    outputFile.write(file_all_lines)
    inputFile.close()
    os.remove(filename)

    # Read and write the rest of the runs without the header.
    for r_num in run_nums[1:]:
        filename = output_directory + "/" + instrument_name + "_" + r_num + ".profiles"
        inputFile = open(filename, "r")
        for line in inputFile:
            if line[0] == "0":
                break
        outputFile.write(line)
        for line in inputFile:
            outputFile.write(line)
        inputFile.close()
        os.remove(filename)

    # Remove *.integrate file(s) ONLY USED FOR CYLINDRICAL INTEGRATION!
    for integrateFile in os.listdir(output_directory):
        if integrateFile.endswith(".integrate"):
            os.remove(integrateFile)

end_time = time.time()

print("\n**************************************************************************************")
print("****************************** DONE PROCESSING ALL RUNS ******************************")
print("**************************************************************************************\n")

print("Total time:   " + str(end_time - start_time) + " sec")
print("Config file: " + ", ".join(config_files))
print("Script file:  " + reduce_one_run_script + "\n")
print()
