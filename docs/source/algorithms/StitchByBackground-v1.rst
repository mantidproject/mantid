
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm allows for the stitching of a list of single-spectra workspaces into one workspace by using a number of
user-defined stitch points at x values and an overlap region around these points in which to perform a linear background fit.

In comparison to :ref:`algm-Stitch1DMany` or :ref:`algm-MatchAndMergeWorkspaces`, this algorithm does not perform any
rebinning during the stitching process, creating a ragged workspace that can be rebinned appropriately later in the
reduction.

Usage
-----

**Example - StitchByBackground**

.. testcode:: StitchByBackgroundExample

    ws_list = []
    stitch_points = []

    for i in range(5):
        CreateSampleWorkspace(OutputWorkspace=f"ws_{i+1}", NumBanks=1, BankPixelWidth=1, Function="Multiple Peaks", XMin=20000*i, XMax=20000*(i+1))
        ws_list.append(f"ws_{i+1}")
        stitch_points.append(20000*(i+1))
    stitch_points.pop(-1)

    StitchByBackground(InputWorkspaces=ws_list, StitchPoints=stitch_points, OutputWorkspace="out", OverlapWidth=2000, CropUpperBound=95000, CropLowerBound=0)
    print(f"Stitched workspace has {mtd['out'].blocksize()} bins.")

Output:

.. testoutput:: StitchExample

  Stitched workspace has 475 bins.

.. categories::

.. sourcelink::
