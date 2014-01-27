# File: ReduceSCD_Parallel.py
#
# Version 2.0, modified to work with Mantid's new python interface.
#
# This script will run multiple instances of the script ReduceOneSCD_Run.py
# in parallel, using either local processes or a slurm partition.  After
# using the ReduceOneSCD_Run script to find, index and integrate peaks from
# multiple runs, this script merges the integrated peaks files and re-indexes
# them in a consistent way.  If desired, the indexing can also be changed to a
# specified conventional cell.
# Many intermediate files are generated and saved, so all output is written 
# to a specified output_directory.  This output directory must be created
# before running this script, and must be specified in the configuration file.
# The user should first make sure that all parameters are set properly in
# the configuration file for the ReduceOneSCD_Run.py script, and that that 
# script will properly reduce one scd run.  Once a single run can be properly
# reduced, set the additional parameters in the configuration file that specify 
# how the the list of runs will be processed in parallel. 
#
import os
import sys
import threading
import time
import ReduceDictionary

sys.path.append("/opt/mantidnightly/bin")
#sys.path.append("/opt/Mantid/bin")

from mantid.simpleapi import *

print "API Version"
print apiVersion()

start_time = time.time()

# -------------------------------------------------------------------------
# ProcessThread is a simple local class.  Each instance of ProcessThread is 
# a thread that starts a command line process to reduce one run.
#
class ProcessThread ( threading.Thread ):
   command = ""

   def setCommand( self, command="" ):
      self.command = command

   def run ( self ):
      print 'STARTING PROCESS: ' + self.command
      os.system( self.command )

# -------------------------------------------------------------------------

#
# Get the config file name from the command line
#
if (len(sys.argv) < 2):
  print "You MUST give the config file name on the command line"
  exit(0)

config_file_name = sys.argv[1]

#
# Load the parameter names and values from the specified configuration file 
# into a dictionary and set all the required parameters from the dictionary.
#

params_dictionary = ReduceDictionary.LoadDictionary( config_file_name )

exp_name              = params_dictionary.get('exp_name', "SAPPHIRE_JUNE_SPHERE")
output_directory      = params_dictionary.get('output_directory', "/SNS/users/eu7/SCRIPT_TEST")
reduce_one_run_script = params_dictionary.get('reduce_one_run_script', "ReduceOneSCD_Run.py")
slurm_queue_name      = params_dictionary.get('slurm_queue_name', None) 
max_processes         = int(params_dictionary.get('max_processes', "8"))
min_d                 = params_dictionary.get('min_d', "4")
max_d                 = params_dictionary.get('max_d', "8")
tolerance             = params_dictionary.get('tolerance', "0.12")
cell_type             = params_dictionary.get('cell_type', "Rhombohedral") 
centering             = params_dictionary.get('centering', "R")
run_nums              = params_dictionary.get('run_nums', "5637:5644")

# determine what python executable to launch new jobs with
python = sys.executable
if python is None: # not all platforms define this variable
   python = 'python'

#
# Make the list of separate process commands.  If a slurm queue name
# was specified, run the processes using slurm, otherwise just use
# multiple processes on the local machine.
#
list=[]
index = 0
for r_num in run_nums:
  list.append( ProcessThread() )
  cmd = '%s %s %s %s' % (python, reduce_one_run_script, config_file_name, str(r_num))
  if slurm_queue_name is not None:
    console_file = output_directory + "/" + str(r_num) + "_output.txt"
    cmd =  'srun -p ' + slurm_queue_name + \
           ' --cpus-per-task=3 -J ReduceSCD_Parallel.py -o ' + console_file + ' ' + cmd
  list[index].setCommand( cmd )
  index = index + 1

#
# Now create and start a thread for each command to run the commands in parallel, 
# starting up to max_processes simultaneously.  
#
all_done = False
active_list=[]
while not all_done:
  if ( len(list) > 0 and len(active_list) < max_processes ):
    thread = list[0]
    list.remove(thread)
    active_list.append( thread ) 
    thread.start()
  time.sleep(2)
  for thread in active_list:
    if not thread.isAlive():
      active_list.remove( thread )
  if len(list) == 0 and len(active_list) == 0 :
    all_done = True

print "\n**************************************************************************************"
print   "************** Completed Individual Runs, Starting to Combine Results ****************"
print   "**************************************************************************************\n"

#
# First combine all of the integrated files, by reading the separate files and
# appending them to a combined output file.
#
niggli_name = output_directory + "/" + exp_name + "_Niggli"
niggli_integrate_file = niggli_name + ".integrate"
niggli_matrix_file = niggli_name + ".mat"

first_time = True
for r_num in run_nums:
  one_run_file = output_directory + '/' + str(r_num) + '_Niggli.integrate'
  peaks_ws = LoadIsawPeaks( Filename=one_run_file )
  if first_time:
    SaveIsawPeaks( InputWorkspace=peaks_ws, AppendFile=False, Filename=niggli_integrate_file )
    first_time = False
  else:
    SaveIsawPeaks( InputWorkspace=peaks_ws, AppendFile=True, Filename=niggli_integrate_file )

#
# Load the combined file and re-index all of the peaks together. 
# Save them back to the combined Niggli file
#
peaks_ws = LoadIsawPeaks( Filename=niggli_integrate_file )
FindUBUsingFFT( PeaksWorkspace=peaks_ws, MinD=min_d, MaxD=max_d, Tolerance=tolerance )
IndexPeaks( PeaksWorkspace=peaks_ws, Tolerance=tolerance )
SaveIsawPeaks( InputWorkspace=peaks_ws, AppendFile=False, Filename=niggli_integrate_file )
SaveIsawUB( InputWorkspace=peaks_ws, Filename=niggli_matrix_file )

#
# If requested, also switch to the specified conventional cell and save the
# corresponding matrix and integrate file
#
if (not cell_type is None) and (not centering is None) :
  conv_name = output_directory + "/" + exp_name + "_" + cell_type + "_" + centering
  conventional_integrate_file = conv_name + ".integrate"
  conventional_matrix_file = conv_name + ".mat"

  SelectCellOfType( PeaksWorkspace=peaks_ws, CellType=cell_type, Centering=centering,
                    Apply=True, Tolerance=tolerance )
  SaveIsawPeaks( InputWorkspace=peaks_ws, AppendFile=False, Filename=conventional_integrate_file )
  SaveIsawUB( InputWorkspace=peaks_ws, Filename=conventional_matrix_file )

end_time = time.time()

print "\n**************************************************************************************"
print   "****************************** DONE PROCESSING ALL RUNS ******************************"
print   "**************************************************************************************\n"

print 'Total time:   ' + str(end_time - start_time) + ' sec'
print 'Connfig file: ' + config_file_name 
print 'Script file:  ' + reduce_one_run_script + '\n'
print
