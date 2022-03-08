.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Sorts workspaces into groups based on their sample changer position

Usage
-----

**Example - IRIS energy conversion**

.. testcode:: ExIRISReduction

   indirect_reductions = ISISIndirectEnergyTransfer(InputFiles='IRS94297.nxs, IRS94298.nxs, IRS94299.nxs',
                                                    OutputWorkspace='IndirectReductions',
                                                    Instrument='IRIS',
                                                    Analyser='graphite',
                                                    Reflection='002',
                                                    SpectraRange=[3, 53])

   grouped_workspaces = GroupBySampleChangerPosition(InputWorkspace=indirect_reductions, OutputGroupPrefix='Indirect', OutputGroupSuffix='Reductions')

   for workspace_name in grouped_workspaces:
      print(mtd[workspace_name].getNames()[0])

Output:

.. testoutput:: ExIRISReduction

   iris94297_graphite002_red
   iris94298_graphite002_red
   iris94299_graphite002_red

.. categories::

.. sourcelink::
