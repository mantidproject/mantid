.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Removes all masked spectra from a workspace and stores all unmasked ones in the output workspace.
The mask is taken either form the optional MaskedWorkspace property or form the input workspace
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
    print 'Input workspace has %s spectra' % ws.getNumberHistograms()

    # Removed the 5 masked spectra
    removed = RemoveMaskedSpectra(ws)
    print 'Output workspace has %s spectra' % removed.getNumberHistograms()

Output:

.. testoutput:: RemoveMaskedSpectraExample

    Input workspace has 200 spectra
    Output workspace has 195 spectra

.. categories::

