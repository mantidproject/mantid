.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithms is to convert an experiment done on reactor-based four-circle instrument
(such as HFIR HB3A) to a MDEventWorkspace with each MDEvent in momentum space.


In this algorithm's name, ConvertCWSDToMomentum, *CW* stands for constant wave
(reactor-source instrument); *SD* stands for single crystal diffraction.

Furthermore, the unit of the output matrix workspace can be converted to
momentum transfer (Q).


Outline of algorithm
####################

1. Create output workspace

 * Build a virtual instrument, requiring

   - position of source
   - position of sample
   - detector ID, position, detector size of pixels

2. Read in data via table workspace

 * From each row, (1) file name and (2) starting detector ID are read in.
 * Detector position in (virtual) instrument of MDEventWorkspace is compared with the position in MatrixWorkspace
 * Momentum is calculated by goniometry values


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
##################

A virtual instrument is built in the algorithm.
In this virtual instrument, the number of detectors and their position are determined
by the number of individual detector's positions in the *experiment*.


MDEventWorkspace
################

There is only one *virtual* instrument and *N* ExperimentInfo.
*N* is the total number of experiment points in the *experiment*.

Inconsistency between using virtual instrument and copying instrument
#####################################################################

It is found that the results, i.e., the peak's position in sample-momentum
space, by FindPeaksMD, are different between the MDEventWorkspaces
output by this algorithm with copying instrument or creating virtual instrument.

It is caused by the difference of the instruments in the MDEventWorkspace.
The native HB3A's detector is of type *RectangularDetector*,
while the virtual instrument's detector is just of class *ComAssembly*.

FindPeaksMD calculates the centre of the mass for peak centre,
and then locates the nearest pixel of the peak center
and re-define the position of the pixel as peak center.

For virtual instrument, CompAssembly::testIntersectionWithChildren()
is used to find the pixel's position;
while for rectangular detector, RectangularDetector::testIntersectionWithChildren()
is used.
Due to the difference in algorithm, there is slightly difference between
the position of the pixel found.

Use cases
---------

It is found that creating an instrument with tens of thousands detectors is very
time consuming in Mantid.
It is caused by creating a map upon these detectors.
With this generation of Mantid, there is no simple solution for it.

For HB3A, there are usually :math:`2\theta` scan, :math:`\omega` scan and :math:`\phi`.
Only the :math:`2\theta` scan requires to create virtual instrument,
while the MDEventWorkspace can be created by copying instrument instance
from parent MatrixWorkspace for the other type of scans.

Therefore, it is suggested to do :math:`\omega` and :math:`\phi` scans for HB3A
with 2D angular detector.


Usage
-----

**Example - convert an HB3A's experiment to MDWorkspace in sample momentum workspae and creating virtual instrument**

.. testcode:: ExConvertHB3AToMDVirtualInstrument

  # Create input table workspaces for experiment information and virtual instrument parameters
  CollectHB3AExperimentInfo(ExperimentNumber='355', ScanList='11', PtLists='-1,11',
      DataDirectory='',
      GenerateVirtualInstrument = True,
      OutputWorkspace='ExpInfoTable', DetectorTableWorkspace='VirtualInstrumentTable')

  # Convert to MDWorkspace
  ConvertCWSDExpToMomentum(InputWorkspace='ExpInfoTable', DetectorTableWorkspace='VirtualInstrumentTable',
      CreateVirtualInstrument = True,
      OutputWorkspace='QSampleMD', SourcePosition='0,0,2', SamplePosition='0,0,0', PixelDimension='1,2,2,3,3,4,3,3',
      Directory='')

  # Find peak in the MDEventWorkspace
  FindPeaksMD(InputWorkspace='QSampleMD', DensityThresholdFactor=0.10000000000000001,
      OutputWorkspace='PeakTable')

  # Examine
  mdws = mtd['QSampleMD']
  print('Output MDEventWorkspace has {} events.'.format(mdws.getNEvents()))
  peakws = mtd['PeakTable']
  print('There are {} peaks found in output MDWorkspace'.format(peakws.getNumberPeaks()))
  peak = peakws.row(0)
  qsample = peak['QSample']
  print('In Q-sample frame, center of peak 0 is at ({:.5f}, {:.5f}, {:.5f}) at detector with ID {}'.
      format(qsample.X(), qsample.Y(), qsample.Z(), peak['DetID']))

.. testcleanup::  ExConvertHB3AToMDVirtualInstrument

  DeleteWorkspace(Workspace='QSampleMD')
  DeleteWorkspace(Workspace='ExpInfoTable')
  DeleteWorkspace(Workspace='VirtualInstrumentTable')
  DeleteWorkspace(Workspace='PeakTable')

Output:

.. testoutput:: ExConvertHB3AToMDVirtualInstrument

  Output MDEventWorkspace has 397 events.
  There are 1 peaks found in output MDWorkspace
  In Q-sample frame, center of peak 0 is at (-6.98263, 0.07773, 8.21074) at detector with ID 26214

**Example - convert an HB3A experiment to MDEventWorkspace by copying instrument.:**

.. testcode:: ExConvertHB3AToMDCopyInstrument

  # Create input table workspaces for experiment information and virtual instrument parameters
  CollectHB3AExperimentInfo(ExperimentNumber='355', ScanList='11', PtLists='-1,11',
      DataDirectory='',
      GenerateVirtualInstrument=False,
      OutputWorkspace='ExpInfoTable', DetectorTableWorkspace='VirtualInstrumentTable')

  # Convert to MDWorkspace
  ConvertCWSDExpToMomentum(InputWorkspace='ExpInfoTable',
      CreateVirtualInstrument = False,
      OutputWorkspace='QSampleMD', Directory='')

  # Find peak in the MDEventWorkspace
  FindPeaksMD(InputWorkspace='QSampleMD', DensityThresholdFactor=0.10000000000000001,
      OutputWorkspace='PeakTable')

  # Examine
  mdws = mtd['QSampleMD']
  print('Output MDEventWorkspace has {} events.'.format(mdws.getNEvents()))
  peakws = mtd['PeakTable']
  print('There are {} peaks found in output MDWorkspace'.format(peakws.getNumberPeaks()))
  peak = peakws.row(0)
  qsample = peak['QSample']
  print('In Q-sample frame, center of peak 0 is at ({:.5f}, {:.5f}, {:.5f}) at detector with ID {}'.
      format(qsample.X(), qsample.Y(), qsample.Z(), peak['DetID']))

.. testcleanup::  ExConvertHB3AToMDCopyInstrument

  DeleteWorkspace(Workspace='QSampleMD')
  DeleteWorkspace(Workspace='ExpInfoTable')
  DeleteWorkspace(Workspace='VirtualInstrumentTable')
  DeleteWorkspace(Workspace='PeakTable')

Output:

.. testoutput:: ExConvertHB3AToMDCopyInstrument

  Output MDEventWorkspace has 397 events.
  There are 1 peaks found in output MDWorkspace
  In Q-sample frame, center of peak 0 is at (-3.58246, -4.40802, -3.06320) at detector with ID 32881

.. categories::

.. sourcelink::
