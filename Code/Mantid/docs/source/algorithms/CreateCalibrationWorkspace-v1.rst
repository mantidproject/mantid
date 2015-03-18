.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates a calibration workspace to be used with inelastic indirect reductions,
allowing for the correction of relative detector intensities.

Usage
-----

**Example - create calibration workspace for IRIS**

.. include:: ../usagedata-note.txt

.. testcode:: ExCreateCalibrationWorkspaceSimple

   # Create a calibration workspace
   cal_ws = CreateCalibrationWorkspace(InputFiles='IRS26173.raw',
                                       DetectorRange='3,53',
                                       PeakRange='62500,65000',
                                       BackgroundRange='59000,61500')

   print 'Number of spectra: %d' % cal_ws.getNumberHistograms()
   print 'Number of bins: %d' % cal_ws.blocksize()

Output:

.. testoutput:: ExCreateCalibrationWorkspaceSimple

   Number of spectra: 51
   Number of bins: 1

.. categories::
