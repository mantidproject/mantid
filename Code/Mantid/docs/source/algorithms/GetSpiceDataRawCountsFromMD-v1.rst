.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is to export raw experimental data from MDEventWorkspaces,
to which a SPICE data file is loaded. 

It provides 3 modes, *Pt.*, *Detector* and *Sample Log* for various types
of experimental data to export. 

Limitation
##########

This algorithm fully supports the SPICE data of powder diffractometers. 
Some features may not work with other SPICE data. 


Inputs
######

Two input MDEventWorkspaces that are required. 

**InputWorkspace** is an MDEventWorkspace that stores detectors counts and sample logs. 
Each run in this MDEventWorkspace corresponds to an individual measurement point in experiment run. 
Each run has its own instrument object. 

The other input MDEventWorkspace, i.e., **MonitorWorkspace** contains the monitor counts of each measurement point.  
The signal value of each MDEvent in this workspace is the monitor counts
corresponding to an individual detector. 

These two MDEventWorkspace should have the same number of runs and same number of MDEvent.  


Outputs
#######

One single-spectrum MatrixWorkspace is the output of this algorithm. 

Mode *Pt.*
++++++++++

The x-values are the :math:`2\theta` positions of all detectors in a specific experiment measuring point (i.e., *Pt.* and
run number). 
The y-values are the raw counts or normalized intensities (by monitor counts)
of all those detectors of the same *Pt.*.

Mode *Detector*
+++++++++++++++

The x-values can be either :math:`2\theta` positions of all detectors or any sample log's values for all experiment measuring 
points (i.e., *Pt.*). 
The y-values are the raw counts or normalized intensities of a specified detector among all *Pt.*. 

Mode *Sample Log*
+++++++++++++++++

The x-values can be any sample log's values of all experiment measuring points (i.e., *Pt.*).
The y-values are the values of a specified sample log among all *Pt.*.  


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
  ConvertSpiceDataToRealSpace(InputWorkspace='Exp0231DataTable', 
        RunInfoWorkspace='Exp0231ParentWS', 
        OutputWorkspace='Exp0231DataMD', 
        OutputMonitorWorkspace='Exp0231MonitorMD')


  # Get raw counts of DetID = 20 out
  ws = GetSpiceDataRawCountsFromMD(InputWorkspace='Exp0231DataMD', 
      MonitorWorkspace='Exp0231MonitorMD', 
      OutputWorkspace='Det20Counts', DetectorID=20)
    
  # output
  for i in [3, 9, 44, 60]:
      print "X[%d] = %.5f, Y[%d] = %.5f" % (i, ws.readX(0)[i], i, ws.readY(0)[i])


.. testcleanup:: ExLoadHB2ADataToMD

  DeleteWorkspace('Exp0231DataTable')
  DeleteWorkspace('Exp0231ParentWS')
  DeleteWorkspace('Exp0231DataMD')
  DeleteWorkspace('Exp0231MonitorMD')
  DeleteWorkspace('Det20Counts')

Output:

.. testoutput:: ExLoadHB2ADataToMD

  X[3] = 57.53600, Y[3] = 0.00281
  X[9] = 58.13600, Y[9] = 0.00354
  X[44] = 61.63600, Y[44] = 0.00315
  X[60] = 63.23600, Y[60] = 0.00325
  
.. categories::
