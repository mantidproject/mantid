.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Removes all masked spectra from a workspace and stores all unmasked ones in the output workspace.
The mask is taken either from the optional MaskedWorkspace property or from the input workspace
if it is the only input. The MaskedWorkspace property can either be a masked MatrixWorkspace or
a specialised MaskWorkspace.


Usage
-----

**Example - RemoveMaskedSpectra**

.. testcode:: RemoveMaskedSpectraExample

    # Create a workspace
    ws = CreateSampleWorkspace()
    # Mask some detectors
    MaskDetectors(ws,SpectraList=[10,20,30,40,50])
    print('Input workspace has {} spectra'.format(ws.getNumberHistograms()))

    # Removed the 5 masked spectra
    removed = RemoveMaskedSpectra(ws)
    print('Output workspace has {} spectra'.format(removed.getNumberHistograms()))

Output:

.. testoutput:: RemoveMaskedSpectraExample

    Input workspace has 200 spectra
    Output workspace has 195 spectra

.. categories::

.. sourcelink::
