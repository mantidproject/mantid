.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithms is to convert an MDEventWorkspace in Q-sample coordinate 
to HKL coordindate for a reactor-based four-circle single crystal diffractometer.
Meanwhile, the algorithm is able to export the MDEvents to file.

Outline of algorithm
####################

1. Convert MDEvent's coordinate system

2. Export MDEvent in Q-sample coordinate and HKL coordinate.


Inputs
######

**InputWorkspace** is an MDEventWorkspace... 

**PeakWorkspace** is an optional input as in many cases especially after calculating UB matrix, ... 

**UBMatrix**


Outputs
#######

The output is an MDEventWorkspace that... .. 

MDEvent
+++++++

Each MDEvent in output MDEventWorkspace contain 
* *Kx*
* *Ky*
* *Kz*
* Signal
* Error
* Detector ID
* Run Number

Compare with ConvertMD
======================

... ...


Usage
-----

**Example - Convert detector counts of an HB3A's measurement to HKL.**

.. testcode:: ExConvertHB3AToHKL

  # Create input table workspaces for experiment information and virtual instrument parameters
  CollectHB3AExperimentInfo(ExperimentNumber='355', ScanList='11', PtLists='-1,11', 
      DataDirectory='',
      OutputWorkspace='ExpInfoTable', DetectorTableWorkspace='VirtualInstrumentTable')

  # Convert to MDWorkspace
  ConvertCWSDExpToMomentum(InputWorkspace='ExpInfoTable', DetectorTableWorkspace='VirtualInstrumentTable', 
      OutputWorkspace='QSampleMD', SourcePosition='0,0,2', SamplePosition='0,0,0', PixelDimension='1,2,2,3,3,4,3,3', 
      Directory='')

  # Find peak in the MDEventWorkspace
  FindPeaksMD(InputWorkspace='QSampleMD', DensityThresholdFactor=0.10000000000000001, 
      OutputWorkspace='PeakTable')
  
  # Examine
  mdws = mtd['QSampleMD']
  print 'Output MDEventWorkspace has %d events.'%(mdws.getNEvents())
  peakws = mtd['PeakTable']
  print  'There are %d peaks found in output MDWorkspace'%(peakws.getNumberPeaks())
  peak = peakws.getPeak(0)
  qsample = peak.getQSampleFrame()
  print 'In Q-sample frame, center of peak 0 is at (%.5f, %.5f, %.5f) at detector with ID %d'%(
      qsample.X(), qsample.Y(), qsample.Z(), peak.getDetectorID())
    
.. testcleanup::  ExConvertHB3AToHKL

  DeleteWorkspace(Workspace='QSampleMD')
  DeleteWorkspace(Workspace='ExpInfoTable')
  DeleteWorkspace(Workspace='VirtualInstrumentTable')
  DeleteWorkspace(Workspace='PeakTable')

Output:

.. testoutput:: ExConvertHB3AToHKL

  Output MDEventWorkspace has 397 events.
  There are 1 peaks found in output MDWorkspace
  In Q-sample frame, center of peak 0 is at (-6.95467, -0.06937, 8.14106) at detector with ID 29072

.. categories::

.. sourcelink::
