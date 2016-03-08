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

**InputWorkspace** is an MDEventWorkspace ???.

**PeakWorkspace** is an optional input as in many cases especially after calculating UB matrix, ... 

**UBMatrix** is ???. 


Outputs
#######

The output is an MDEventWorkspace that... .. 

MDEvent
+++++++

Each MDEvent in output MDEventWorkspace contain 
* *H*
* *K*
* *L*
* Signal
* Error
* Detector ID
* Run Number

Compare with ConvertMD
======================

Comparing with ????

Usage
-----

**Example - Convert detector counts of an HB3A's measurement to HKL.**

.. testcode:: ExConvertHB3AToHKL

  # Create input table workspaces for experiment information and virtual instrument parameters
  CollectHB3AExperimentInfo(ExperimentNumber='406', ScanList='298', PtLists='-1,22', 
      DataDirectory='',
      GenerateVirtualInstrument=False,
      OutputWorkspace='ExpInfoTable', DetectorTableWorkspace='VirtualInstrumentTable')

  # Convert to MDWorkspace
  ConvertCWSDExpToMomentum(InputWorkspace='ExpInfoTable', CreateVirtualInstrument=False, 
      OutputWorkspace='QSampleMD',
      Directory='')
      
  ConvertCWSDMDtoHKL(InputWorkspace='QSampleMD', 
                UBMatrix='0.13329061, 0.07152342, -0.04215966, 0.01084569, -0.1620849, 0.0007607, -0.14018499, -0.07841385, -0.04002737',
                OutputWorkspace='HKLMD')
              
  
  # Examine
  mdws = mtd['QSampleMD']
  print 'Output MDEventWorkspace has %d events.'%(mdws.getNEvents())
  
  hklws = mtd['HKLMD']
  print 'H: range from %.5f to %.5f.' % (hklws.getXDimension().getMinimum(), hklws.getXDimension().getMaximum())
  print 'K: range from %.5f to %.5f.' % (hklws.getYDimension().getMinimum(), hklws.getYDimension().getMaximum())
  print 'L: range from %.5f to %.5f.' % (hklws.getZDimension().getMinimum(), hklws.getZDimension().getMaximum())

.. testcleanup::  ExConvertHB3AToHKL

  DeleteWorkspace(Workspace='QSampleMD')
  DeleteWorkspace(Workspace='ExpInfoTable')
  DeleteWorkspace(Workspace='VirtualInstrumentTable')
  DeleteWorkspace(Workspace='HKLMD')
  DeleteWorkspace(Workspace='HB3A_exp0406_scan0298')
  DeleteWorkspace(Workspace='spicematrixws')

Output:

.. testoutput:: ExConvertHB3AToHKL

  Output MDEventWorkspace has 1631 events.
  H: range from -0.26128 to 0.24943.
  K: range from -0.35012 to 0.44396.
  L: range from 4.96512 to 7.18855.

.. categories::

.. sourcelink::
