.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

TODO

Usage
-----
**Example - create calibration workspace for IRIS**

.. include:: ../usagedata-note.txt 

.. testcode:: ExCreateCalibrationWorkspaceSimple

   import os

   # Create a calibration workspace
   cal_ws = CreateCalibrationWorkspace(InputFiles='IRS38633.raw', DetectorRange='3,53', PeakRange='62500,65000', BackgroundRange='59000,61500')

   # Save the workspace to a NeXus file
   calib_file = 'iris_calibration.nxs'
   SaveNexus(InputWorkspace=cal_ws, Filename=calib_file)

   # Check the output file
   print "File Exists:", os.path.exists(calib_file)

Output:

.. testoutput:: ExCreateCalibrationWorkspaceSimple

   File Exists: True
   
.. testcleanup:: ExCreateCalibrationWorkspaceSimple

   os.remove(calib_file)

   
.. categories::
