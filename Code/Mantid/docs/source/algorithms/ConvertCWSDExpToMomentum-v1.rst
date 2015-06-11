.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithms is to convert an experiment done on reactor-based four-circle instrument
(such as HFIR HB3A) to a MDEventWorkspace with each MDEvent in momentum space. 


In this algorithm's name, ConvertCWSDToMomentum, *CW* stands for constant wave 
(reactor-source instrument); *SD* stands for single crystal diffraction.

This algorithm takes ??? as inputs.

Futhermore, the unit of the output matrix workspace can be converted to 
momentum transfer (:math:`Q`). 


Outline of algorithm
####################

1. Create output workspace.
 * Build a virtual instrument, requiring  
   - position of source
   - position of sample
   - detector ID, position, detector size of pixels
2. Read in data via table workspace
 * From each row, (1) file name and (2) starting detector ID are read in.  
 * Detector position in (virtual) instrument of MDEventWorkspace is compared with the position in MatrixWorkspace 
 * Momentum is calcualted by goniometry values


Input Workspaces
################

Two TableWorkspaces, which contain experiment information, are required. 

**InputWorkspace** is a TableWorkspace containing the data files names to be loaded for the experiment. 
It is required to have 4 columns.  
They are *Scan*, *Pt*, *Filename* and *StartDetID* respectively. 

A typical HB3A experiment consists of multiple scans, each of which contains multiple measurement point (i.e., Pt). 
*FileName* is the XML data file for 2D detector information for a specific Scan/Pt pair. 
*StartDetID* is the starting detector ID of a specific Scan/Pt mapped to the virtual instrument. 

**DetectorTableWorkspace** is a TableWorkspace that list the parameters of all detector pixels belonged 
to the virtual instrument. 
The parameters include detector ID in virtual instrument, detector's position in cartesian coordinate,
and detector's original detector ID. 


Outputs
#######

The output is an MDEventWorkspace that stores the data of an HB3A experiment.  
Every non-zero count recorded by detector is converted to an MDEvent in the 
output workspace. 


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



Combine Experiment Into One MDEventWorkspace
--------------------------------------------

One typical HB3A (reactor-based four-circle diffractometer) experiment consists of 
a set of scans, each of which contains multiple experiment point (labeled as *Pt.* in SPICE). 

Each experiment point is independent to the others. 
They can have various detector positions, goniometer setup and even sample environment setup.

In order to integrate them into an organized data structure, i.e., *MDEventWorkspace*, 
a virtual instrument is built in the algorithm. 

Virtual instrument
==================

A virtual instrument is built in the algorithm. 
In this virtual instrument, the number of detectors and their position are determined 
by the number of individual detector's positions in the *experiment*.


MDEventWorkspace
================

There is only one *virtual* instrument and *N* ExperimentInfo.  
*N* is the total number of experiment points in the *experiment*. 


Usage
-----

**Example - convert SPICE file for HB3A to Fullprof file:**

.. testcode:: ExConvertHB3AToMD

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
    
.. testcleanup::  ExConvertHB3AToMD

  DeleteWorkspace(Workspace='QSampleMD')
  DeleteWorkspace(Workspace='ExpInfoTable')
  DeleteWorkspace(Workspace='VirtualInstrumentTable')
  DeleteWorkspace(Workspace='PeakTable')

Output:

.. testoutput:: ExConvertHB3AToMD

  Output MDEventWorkspace has 397 events.
  There are 1 peaks found in output MDWorkspace
  In Q-sample frame, center of peak 0 is at (-6.95467, -0.06937, 8.14106) at detector with ID 29072

.. categories::
