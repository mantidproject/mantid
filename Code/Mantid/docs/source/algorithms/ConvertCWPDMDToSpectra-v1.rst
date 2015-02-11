.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

... ...


Inputs
######

... ...

Outputs
#######

... ...


Sample Logs
###########

... ...


Workflow
--------

... ...


Usage
-----

**Example - reduce a SPICE file for HB2A to Fullprof file:**

.. testcode:: ExReduceHB2AToFullprof

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

  # Convert from real-space MD to Fullprof data
  ConvertCWPDMDToSpectra(
        InputWorkspace = 'Exp0231DataMD',
        InputMonitorWorkspace = 'Exp0231MonitorMD',
        OutputWorkspace = 'Exp0231Reduced')

  # output
  datamdws = mtd["Exp0231DataMD"]
  print "Number of events = %d" % (datamdws.getNEvents())

.. testcleanup::  ExReduceHB2AToFullprof

  DeleteWorkspace('Exp0231DataTable')
  DeleteWorkspace('Exp0231ParentWS')
  DeleteWorkspace('Exp0231DataMD')
  DeleteWorkspace('Exp0231MonitorMD')

Output:

.. testoutput:: ExReduceHB2AToFullprof

  Number of events = 2684

.. categories::
