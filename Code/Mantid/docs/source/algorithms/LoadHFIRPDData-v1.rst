.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is to load HFIR Spice powder diffraction data to MDWorkspaces. 
HB2A and HB2B are the two instruments supported by this algorithm. 


Format of SPICE data file
#########################

There are two parts of content in SPICE data file. 
One is the run information, in which line that starts with #. 

In most cases, the run information is give as 

# run info name = run info value

The other is the experimental data.  
The first item is an integer as the index of experimental data point. 

How to combine measurements
###########################

Powder diffractometers rotates its detectors to achieve the coverage
and resolution. 
Hence the output of of a run should include all the measurements. 

MDEventWorkspac is the best solution to combine all the measurements
to a single workspace and 
keep all the information for future reduction. 


Output Worskpaces
#################

Two MDEventWorkspaces will be output from this algorithm. 
One MDEventWorkspace stores the detectors' counts;
and the other one stores the monitor counts. 


Apply MDWorkspaces to HFIR powder diffractometer experiment
-----------------------------------------------------------

Run
###

In an HFIR powder diffractometer scan, one measurement is made at a certain rotational angle. 
Such one meaurement constains 44 detectors' counts, instrument setup (such as rotation angle) and
sample environment logs.  
As it is translated to MDWorkspace, such a measurement constitutes a {\it run}. 


Essential logs for building MDWorkspace
#######################################

There are a few sample logs that are essential to create the MDWorkspaces for the HFIR powder diffractometers.
The algorithm allows the user to specify these logs

* Rotation angle to determine the :math:`2\theta` for detector 0. The default is '2theta';
* Prefix of the logs for detectors.  The default is 'anode'.  In present HB2A's SPICE file, the detectors are labelled as 'anode1', 'anode2', and etc;
* Monitor counts for each measurement.  The default is 'monitor';
* Duration of each measurement.  The default is 'time'. 


Sample Logs
###########

Sample logs will be written to the ExperimentInfo for each run.  
There are N+1 ExperimentInfo in the MDWorkspace that stores
the detectors' counts.  

The first N ExperimentInfo are for the N measurements.  
The sample logs's value measured of that data point will be recorded in the corresponding 
ExperimentInfo.  
Hence each of these N ExperimentInfo will contains a set of sample logs, each of which
contains only one log entry. 

The last one, i.e., ExperimentInfo[N], contains the combined sample logs from all the runs. 
Hence for an experiment with N runs. 


Temporary MD File
#################

*** TODO : write how the MD file is build and read in **


Workflow
--------

The 2 input workspaces of algorithm LoadHFIRPDData are the output of 
algorithm LoadSpiceAscii. 
Therefore, in order to load an HB2A or HB2B data from a SPICE file, 
LoadSpiceAscii should be called first. 


Usage
-----

**Example - load a SPICE .dat file for HB2A:**

.. testcode:: ExLoadHB2ADataToMD

  # create table workspace and parent log workspace
  LoadSpiceAscii(Filename='HB2A_exp0231_scan0001.dat', 
        IntegerSampleLogNames="Sum of Counts, scan, mode, experiment_number",
        FloatSampleLogNames="samplemosaic, preset_value, Full Width Half-Maximum, Center of Mass", 
        DateAndTimeLog='date,MM/DD/YYYY,time,HH:MM:SS AM', 
        OutputWorkspace='Exp0231DataTable', 
        RunInfoWorkspace='Exp0231ParentWS')

  # load for HB2A 
  LoadHFIRPDData(InputWorkspace='Exp0231DataTable', 
        ParentWorkspace='Exp0231ParentWS', 
        OutputWorkspace='Exp0231DataMD', 
        OutputMonitorWorkspace='Exp0231MonitorMD')

  # output
  datamdws = mtd["Exp0231DataMD"]
  print "Number of events = %d" % (datamdws.getNEvents())

.. testcleanup:: ExLoadHB2ADataToMD

  DeleteWorkspace('Exp0231DataTable')
  DeleteWorkspace('Exp0231ParentWS')
  DeleteWorkspace('Exp0231DataMD')
  DeleteWorkspace('Exp0231MonitorMD')

Output:

.. testoutput:: ExLoadHB2ADataToMD

  Number of events = 2684

.. categories::
