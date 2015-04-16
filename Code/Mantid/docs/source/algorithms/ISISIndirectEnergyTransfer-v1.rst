.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Performs a reduction from raw time of flight to energy transfer for an inelastic
indirect geometry instrument at ISIS.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - IRIS energy conversion**

.. testcode:: ExIRISReduction

   ISISIndirectEnergyTransfer(InputFiles='IRS21360.raw',
                              OutputWorkspace='IndirectReductions',
                              Instrument='IRIS',
                              Analyser='graphite',
                              Reflection='002',
                              SpectraRange=[3, 53])

   reduction_workspace_names = mtd['IndirectReductions'].getNames()

   for workspace_name in reduction_workspace_names:
      print workspace_name

Output:

.. testoutput:: ExIRISReduction

   IRS21360_graphite002_red

.. categories::
