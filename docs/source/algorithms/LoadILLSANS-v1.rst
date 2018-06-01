.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This loads the nexus files produced by the SANS instruments D11, D22, D33 at the ILL.
For D33, it supports both monochromatic and TOF modes.
This loader reads the detector positions from the nexus file and places the detectors accordingly.
For D11 and D22 it supports the nominal and low resolution modes (pixel splitting).
The output is a histogram workspace with unit of wavelength.
It has a single bin for monochromatic, and many bins for TOF mode for D33.

.. include:: ../usagedata-note.txt

**Example - Loads a D33 TOF file.**

.. testcode:: ExLoad
    ws = LoadILLSANS('ILL/D33/042610.nxs')
    numHistograms = ws.getNumberHistograms()
    numTOF = ws.blocksize()
    print('This workspace has {0} spectra and {1} TOF channels.'.format(numHistograms, numTOF))

Output:

.. testoutput:: ExLoad

    This workspace has 65538 spectra and 200 TOF channels.

.. categories::

.. sourcelink::
