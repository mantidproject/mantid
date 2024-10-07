.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Copy bin mask flags to a workspace from another workspace.

At present, the bin masks attached to the mask workspace's 0th spectrum are applied to
every spectrum in the input workspace.

Usage
-----

**Masking of a small workspace**

.. testcode:: exMaskBinsFromWorkspaceSimple

   # Create workspace with 10 bins of width 10
   ws = CreateSampleWorkspace(BankPixelWidth=1, Xmax=100, BinWidth=10)
   wsToMask = CreateSampleWorkspace(BankPixelWidth=1, XMax=100, BinWidth=10)

   # Mask a range of X-values
   wsMasked = MaskBins(wsToMask,XMin=16,XMax=32)

   # Copy the masks over
   copiedMasksWS = MaskBinsFromWorkspace(ws, wsMasked)

   # Show Y values in workspaces, and the masked bin indices
   print("After mask copying: {}".format(copiedMasksWS.readY(0)))
   print("Masked bin indices: {}".format(copiedMasksWS.maskedBinsIndices(0)))


Output:

.. testoutput:: exMaskBinsFromWorkspaceSimple

   After mask copying: [ 0.3  0.3  0.3  0.3  0.3 10.3  0.3  0.3  0.3  0.3]
   Masked bin indices: [1,2,3]

.. categories::

.. sourcelink::
