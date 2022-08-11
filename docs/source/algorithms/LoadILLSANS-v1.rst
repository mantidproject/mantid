.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This loads the nexus files produced by the SANS instruments D11, D22, D33 and D16 at the ILL.

This loader reads the detector positions from the nexus file and places the detectors accordingly.

For D33, it supports both monochromatic and TOF modes.

For D11 and D22 it supports the nominal and low resolution modes (pixel splitting).

For D16, it supports monochromatic and scan mode. The `Wavelength` parameter is intended for old D16 files that may lack
the wavelength value.

It also supports the newer version for these instruments.

The output is a histogram workspace with unit of wavelength (Angstrom), or the parameter being scanned for D16 scans.
It has a single bin for monochromatic, many bins (ragged) for TOF mode for D33, and as many bins as there are scan
points for D16.

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

.. categories::

.. sourcelink::
