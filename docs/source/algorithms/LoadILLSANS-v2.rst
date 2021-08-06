.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This loads the nexus files produced by the SANS instruments D11 (B), D16, D22 (B), and D33 at the ILL.
For D33, it supports both monochromatic and TOF modes.
This loader reads the detector positions from the nexus file and places the detectors accordingly.
For D11 and D22 it supports the nominal and low resolution modes (pixel splitting).
It also supports the newer version for these instruments, denoted with B, as well as loading kinetic
SANS data.

Loading and placement of the instrument geometry can be turned off by setting `LoadInstrument` property
to a `false` value. This feature is currently not supported for D22B.

The output is a histogram workspace with unit of wavelength (Angstrom) in the case of TOF data,
and a point data workspace with X-axis with `Empty` unit being a column index in other cases.
It has a single bin for monochromatic, and many bins (ragged) for TOF mode for D33.

.. include:: ../usagedata-note.txt

**Example - Loads a D33 TOF file.**

.. testcode:: ExLoad

    ws = LoadILLSANS("ILL/D33/042610.nxs")
    numHistograms = ws.getNumberHistograms()
    numTOF = ws.blocksize()
    print('This workspace has {0} spectra and {1} TOF channels.'.format(numHistograms, numTOF))

Output:

.. testoutput:: ExLoad

    This workspace has 65538 spectra and 200 TOF channels.

**Example - Loads a D11B kinetic file.**

.. testcode:: ExLoad_D11B_kinetic

    ws = LoadILLSANS("ILL/D11B/017177.nxs")
    numHistograms = ws.getNumberHistograms()
    numBins = ws.blocksize()
    print('This workspace has {0} spectra and {1} bins.'.format(numHistograms, numBins))

Output:

.. testoutput:: ExLoad_D11B_kinetic

    This workspace has 65538 spectra and 85 bins.

.. categories::

.. sourcelink::
