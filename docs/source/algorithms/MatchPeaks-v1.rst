.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithms shifts the bins of each spectrum of an input workspace while keeping its X-values.
Peak positions of the input workspace can match the center bin, its difference to the elastic peak of a second input workspace, or the difference between a second input workspace and the center bin.

Usage
-----

.. testcode:: MatchPeaks

  # Create a workspace containing some data.
  ws = CreateSampleWorkspace(Function='User Defined',
                                WorkspaceType='Histogram',
                                UserDefinedFunction="name=Gaussian,
                                PeakCentre=3.2, Height=10, Sigma=0.3",
				NumBanks=1, BankPixelWidth=1,
                                XUnit='DeltaE', XMin=0, XMax=7, BinWidth=0.099)

  print('Peak height at center: {}').format(ws.readY(0)[35])

Output
######

.. testoutput:: MatchPeaks

   Peak height at center: 10.0 

.. testcleanup::

.. categories::

.. sourcelink::
