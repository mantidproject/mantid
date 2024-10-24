
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is useful for removing unwanted spectra using masks. If `MaskWorkspace`
is provided it is used to select the spectra to remove. Otherwise the internal mask
in the `InputWorkspace` is used.

Usage
-----

**Example - ExtractUnmaskedSpectra**

.. testcode:: ExtractUnmaskedSpectraExample

    # Create histogram workspace
    ws = CreateSampleWorkspace()

    # Mask 10 spectra
    MaskDetectors(ws, [1,2,3,4,5,6,7,8,9,10])

    # Extract unmasked into a new workspace
    ows = ExtractUnmaskedSpectra(ws)

    # Compare workspace sizes
    print('Number of spectra in original workspace {}'.format(ws.getNumberHistograms()))
    print('Number of spectra in cropped  workspace {}'.format(ows.getNumberHistograms()))

Output:

.. testoutput:: ExtractUnmaskedSpectraExample

    Number of spectra in original workspace 200
    Number of spectra in cropped  workspace 190

.. categories::

.. sourcelink::
