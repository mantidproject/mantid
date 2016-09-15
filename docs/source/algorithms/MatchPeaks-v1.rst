.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm circular shifts the Y- and E-values of each spectrum of an input workspace while keeping its X-values.
Peak positions of the input workspace can match the center bin, its difference to the elastic peak of a second input workspace, or the difference between a second input workspace and the center bin (option MatchInput2ToCenter).
Bins smaller than the 'MinBin' and larger than the 'MaxBin' value specified in the BinRangeTable represent circular appended bins in any spectrum.
MaskBins performs the masking accordingly.

Restrictions
############

The input workspaces should have one elastic peak (see :ref:`FindEPP <algm-FindEPP>`).
Furthermore, input workspaces should have the same number of bins and spectra as well as identical x-values.

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

  print('Peak height at center: {}').format(output_ws.readY(0)[ws.blocksize() / 2])

Output
######

.. testoutput:: MatchPeaks

   Peak height at center: 9.94327262198

.. categories::

.. sourcelink::
