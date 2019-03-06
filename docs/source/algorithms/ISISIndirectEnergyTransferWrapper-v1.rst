.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs a reduction from raw time of flight to energy transfer for an inelastic
indirect geometry instrument at ISIS. It does exactly the same thing as the
:ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>` algorithm except
the recording of child histories is turned off to avoid misleading output file sizes.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - IRIS energy conversion**

.. testcode:: ExIRISDataReduction

   ISISIndirectEnergyTransferWrapper(InputFiles='IRS21360.raw',
                                     OutputWorkspace='IndirectReductions',
                                     Instrument='IRIS',
                                     Analyser='graphite',
                                     Reflection='002',
                                     SpectraRange=[3, 53])

   reduction_workspace_names = mtd['IndirectReductions'].getNames()

   for workspace_name in reduction_workspace_names:
      print(workspace_name)

Output:

.. testoutput:: ExIRISDataReduction

   iris21360_graphite002_red

.. categories::

.. sourcelink::
