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


Sample Logs
###########

** TODO : Add this section about how the algorithm treats **
FIXME *** TODO *** : Do not make a long list of time series property for run 1.
BUT assign the relevant sample values to each ExperimentInfo of each run
Only make the properties in the ParentWorkspace to Sample log of run 0???

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
