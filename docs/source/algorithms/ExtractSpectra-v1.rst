.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Extracts a number of spectra from a MatrixWorkspace and puts them into a new workspace.
Optionally it can cut the number of bins (x values). See the :ref:`algm-CropWorkspace`
algorithm for more details on how it works for ragged workspaces and workspaces with
common bin boundaries.


Usage
-----

**Example - ExtractSpectra**

.. testcode:: ExtractSpectraExample

    # Create an input workspace
    ws = CreateSampleWorkspace()
    print('Input workspace has %s bins' % ws.blocksize())
    print('Input workspace has %s spectra' % ws.getNumberHistograms())

    # Extract spectra 1,3 and 5 and crop the x-vector to interval 200 <= x <= 1300
    cropped = ExtractSpectra(ws,200,1300,WorkspaceIndexList=[1,3,5])
    print('Output workspace has %s bins' % cropped.blocksize())
    print('Output workspace has %s spectra' % cropped.getNumberHistograms())

Output:

.. testoutput:: ExtractSpectraExample

    Input workspace has 100 bins
    Input workspace has 200 spectra
    Output workspace has 5 bins
    Output workspace has 3 spectra

.. categories::

.. sourcelink::
