.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm circular shifts (numpy.roll operation) the Y- and E-values of each spectrum of an input workspace while keeping its X-values.
Peak positions of the input workspace can be aligned with the centre of the X-axis as the default option.
Optionally, a second input workspace can be given. In this case, the peaks in the first workspace will be aligned to the peaks in the second workspace.
If `MatchInput2ToCenter` option is enabled, first workspace will be rolled according to the difference between the second workspace's peaks from the center.
If the third workspace is given, the first workspace will be shifted according to the difference of peak positions between the second and the third workspaces.
A `BinRangeTable` output contains two values, 'MinBin' and 'MaxBin'.
Bins smaller than the 'MinBin' value and larger than the 'MaxBin' value represent bins that where overflown the x-axis range due to the circular shift.
Enabling `MaskBins` will mask out these bins automatically.

Restrictions
############

The input workspaces should have one peak (see :ref:`FindEPP <algm-FindEPP>`), hence `NaN` and `Inf` values will be zeroed, so peak finding can succeed.
The peak position is expected to be around the x-axis center, at most quarter of the number of bins away from the center, otherwise no shift will be performed.
Furthermore, input workspaces should have the same number of bins and spectra as well as identical x-axes.

Usage
-----

.. testcode:: MatchPeaks

  # Create a workspace containing some data.
  ws = CreateSampleWorkspace(Function='User Defined',
                             WorkspaceType='Histogram',
                             UserDefinedFunction="name=Gaussian, PeakCentre=3.2, Height=10, Sigma=0.3",
                             NumBanks=1, BankPixelWidth=1,
                             XUnit='DeltaE', XMin=0, XMax=7, BinWidth=0.099)

  output_ws = MatchPeaks(InputWorkspace=ws)

  print('Peak height at center: {:.11f}'.format(output_ws.readY(0)[ws.blocksize() // 2]))

Output
######

.. testoutput:: MatchPeaks

   Peak height at center: 9.94327262198

.. categories::

.. sourcelink::
