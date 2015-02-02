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

Temporary MD File
#################

** TODO : write how the MD file is build and read in **


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

  LoadSpiceAscii(Filename="HB2A_exp0231_scan0001.dat", 
        IntegerSampleLogNames="Sum of Counts, scan, mode, experiment_number",
        FloatSampleLogNames="samplemosaic, preset_value, Full Width Half-Maximum, Center of Mass", 
        OutputWorkspace="HB2A_0231_0001_Data", 
        RunInfoWorkspace="HB2A_0231_Info")
  datatbws = mtd['HB2A_0231_0001_Data'] 
  infows = mtd['HB2A_0231_Info']

  LoadHFIRPDData()


.. testcleanup:: ExLoadHB2ADataToMD

  DeleteWorkspace(infows)
  DeleteWorkspace(datatbws)

Output:

.. testoutput:: ExLoadHB2ADataToMD

  Number of measuring points = 61
  Number of columns in data workspace = 70
  Number of run information = 34
  Sum of Counts = 1944923
  Center of Mass = 9.00076 +/- 0.00921

.. categories::
