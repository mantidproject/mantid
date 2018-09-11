.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is to create input workspaces to ConvertCWSDExpToMomentum for
HB3A (four-circle single crystal diffractometer in HFIR).


Inputs
======

The inputs required by algorithm *ConvertHB3AExperimentInfo* are the experiment number, scan numbers
and selected Pt. numbers.
By these parameters, the algorithm can determine the names of the data files and generate a list of
detectors for downstream algorithm to create virutal instrument.


OutputWorkspaces
================

Two TableWorkspaces, which contain experiment information, are returned.

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


How to use algorithm with other algorithms
------------------------------------------

Algorithm *CollectHB3AExperimentInfo* is designed to provide input information for algorithm
*ConvertCWSDExpToMomentum*, whose output will be used to calculated :ref:`UB matrix <Lattice>` and integrate
single cystal peaks.


Usage
-----

**Example - Collect HB3A experiment information for Exp No.355:**

.. testcode:: ExCollect355Info

  CollectHB3AExperimentInfo(ExperimentNumber=355,ScanList=[11,38],PtLists=[-1,11,-1,12],
      DataDirectory='',
      Detector2ThetaTolerance=0.01,
      OutputWorkspace='ExpInfoTable', DetectorTableWorkspace='VirtualInstrumentTable')

  # Examine
  expinfows = mtd['ExpInfoTable']
  virtualdetws = mtd['VirtualInstrumentTable']

  print('Number of input files = {}'.format(expinfows.rowCount()))
  print('Number of detectors in virtual instrument = {}'.format(virtualdetws.rowCount()))
  print('Virtual detectors are from ID = {} to ID = {}'.format(virtualdetws.cell(0,0), virtualdetws.cell(131072-1,0)))


.. testcleanup:: ExCollect355Info

  DeleteWorkspace(expinfows)
  DeleteWorkspace(virtualdetws)


Output:

.. testoutput:: ExCollect355Info

  Number of input files = 2
  Number of detectors in virtual instrument = 131072
  Virtual detectors are from ID = 0 to ID = 131070

.. categories::

.. sourcelink::
