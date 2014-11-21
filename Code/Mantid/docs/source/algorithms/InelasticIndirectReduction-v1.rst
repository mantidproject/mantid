.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Performs a reduction for an inelastic indirect geometry instrument.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - IRIS energy conversion:**

.. testcode:: ExIRISReduction

   InelasticIndirectReduction(InputFiles='IRS21360.raw',
      OutputWorkspace='IndirectReductions',
      Instrument='IRIS',
      Analyser='graphite',
      Reflection='002',
      DetectorRange=[3, 53],
      SaveFormats=['nxs'])

   reduction_workspace_names = mtd['IndirectReductions'].getNames()

   for workspace_name in reduction_workspace_names:
      print workspace_name

Output:

.. testoutput:: ExIRISReduction

   irs21360_graphite002_red

.. categories::
