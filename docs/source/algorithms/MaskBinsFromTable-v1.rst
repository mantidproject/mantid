.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Masks bins in a workspace. The user specified masking parameters,
such as spectra, xmin and xmax are given via a TableWorkspace.

It calls algorithm MaskBins underneath.
If DetectorIDsList column exists it will convert this list to a list of spectra before calling MaskBins.

The table may have several rows, in which case it calls Maskbins for each row.


Usage
-----

**Masking of a small workspace**

.. testcode:: exMaskBinsFromTableSimple

   # Create workspace with 10 bins of width 10
   ws = CreateSampleWorkspace(BankPixelWidth=1, Xmax=100, BinWidth=10)

   # Create table workspace with masking information in it
   mask_info = CreateEmptyTableWorkspace()
   mask_info.addColumn("str", "SpectraList")
   mask_info.addColumn("double", "XMin")
   mask_info.addColumn("double", "XMax")
   mask_info.addRow(["", 16.0, 32.0]) # 1st mask: SpectraList=ALL, Xmin=16, XMax=32
   mask_info.addRow(["", 71.5, 79.0]) # 2nd mask: SpectraList=ALL, Xmin=71.5, XMax=79


   # Mask a range of X-values
   wsMasked = MaskBinsFromTable(ws,MaskingInformation=mask_info)

   # Show Y values in workspaces
   print("Before masking: {}".format(ws.readY(0)))
   print("After masking: {}".format(wsMasked.readY(0)))


Output:

.. testoutput:: exMaskBinsFromTableSimple

   Before masking: [ 0.3  0.3  0.3  0.3  0.3 10.3  0.3  0.3  0.3  0.3]
   After masking: [ 0.3  0.   0.   0.   0.3 10.3  0.3  0.   0.3  0.3]



Related Algorithms
------------------

MaskBins
########

:ref:`algm-MaskBins` masks bins in a workspace. Masked bins should
properly be regarded as having been completely removed from the
workspace. Bins falling within the range given (even partially) are
masked, i.e. their data and error values are set to zero and the bin is
added to the list of masked bins. This range is masked for all spectra
in the workspace (though the workspace does not have to have common X
values in all spectra).

.. categories::

.. sourcelink::
