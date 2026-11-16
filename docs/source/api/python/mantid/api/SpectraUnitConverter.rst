======================
 SpectraUnitConverter
======================

This is a Python binding to the C++ class Mantid::API::SpectraUnitConverter.

It lets :py:obj:`~mantid.api.MatrixWorkspace` methods that take a ``unitConverter`` work with X values in another unit,
much as if :ref:`ConvertUnits <algm-ConvertUnits>` had been run on the workspace first, but without converting or
copying it. Only the values asked for are converted, using the geometry of each spectrum, which is looked up once when
the converter is made. So a converter is worth keeping for as long as the workspace and unit stay the same, but must be
made again if the workspace's instrument geometry changes. The converter keeps the workspace alive, even once it has
been deleted from the Analysis Data Service. A workspace method given a converter made for any other
workspace raises a ``ValueError``.

.. testcode:: IntegrateInDSpacing

   from mantid.api import SpectraUnitConverter

   ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=2)
   to_d_spacing = SpectraUnitConverter(ws, "dSpacing")
   x_min, x_max = ws.getXMinMax([1, 2, 3], unitConverter=to_d_spacing)
   counts = ws.getIntegratedCountsForWorkspaceIndices([1, 2, 3], 3, x_min, x_max, False, unitConverter=to_d_spacing)
   print(f"Counts between {x_min:.0f} and {x_max:.0f} Angstrom: {[round(c) for c in counts]}")

Output:

.. testoutput:: IntegrateInDSpacing

   Counts between 0 and 3297 Angstrom: [40, 40, 40]

*bases:* :py:obj:`object`

.. module:`mantid.api`

.. autoclass:: mantid.api.SpectraUnitConverter
    :members:
    :undoc-members:
