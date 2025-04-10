.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithms is to convert an MDEventWorkspace in Q-sample coordinate
to HKL coordinate for a reactor-based four-circle single crystal diffractometer.
Meanwhile, the algorithm is able to export the MDEvents to file.

Outline of algorithm
####################

1. Convert MDEvent's coordinate system

2. Export MDEvent in Q-sample coordinate and HKL coordinate.


Inputs
######

**InputWorkspace** is an MDEventWorkspace ???.

**PeakWorkspace** is an optional input as in many cases especially after calculating :ref:`UB matrix <Lattice>`, ...

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
  hklws = mtd['HKLMD']
  print('Output QSample and HKL workspaces have %d and %d events.'%(mdws.getNEvents(), hklws.getNEvents()))

  BinMD(InputWorkspace='HKLMD', AlignedDim0='H,-0.3,0.3,60', AlignedDim1='K,-0.4,0.5,90', AlignedDim2='L,4,8,10', OutputWorkspace='BinndHKL')
  histws = mtd['BinndHKL']
  events_array = histws.getNumEventsArray()
  print('events[22, 53, 5] = {:.1f}'.format(events_array[22, 53, 5]))
  print('events[30, 40, 5] = {:.1f}'.format(events_array[30, 40, 5]))

.. testcleanup::  ExConvertHB3AToHKL

  DeleteWorkspace(Workspace='QSampleMD')
  DeleteWorkspace(Workspace='ExpInfoTable')
  DeleteWorkspace(Workspace='VirtualInstrumentTable')
  DeleteWorkspace(Workspace='HKLMD')
  DeleteWorkspace(Workspace='HB3A_exp0406_scan0298')
  DeleteWorkspace(Workspace='spicematrixws')


Output:

.. testoutput:: ExConvertHB3AToHKL

  Output QSample and HKL workspaces have 1631 and 1631 events.
  events[22, 53, 5] = 19.0
  events[30, 40, 5] = 38.0

.. categories::

.. sourcelink::
