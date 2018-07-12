.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads an ASCII .hkl file to a PeaksWorkspace.

HKL File Format
###############
        
File has same format that works successfully in GSAS and SHELX from
ISAW:

hklFile.write('%4d%4d%4d%8.2f%8.2f%4d%8.4f%7.4f%7d%7d%7.4f%4d%9.5f%9.4f\\n'%
(H, K, L, FSQ, SIGFSQ, hstnum, WL, TBAR, CURHST, SEQNUM, TRANSMISSION,
DN, TWOTH, DSP))

HKL is flipped by -1 due to different q convention in ISAW vs Mantid.

FSQ = integrated intensity of peak (scaled)

SIGFSQ = sigma from integrating peak

hstnum = number of sample orientation (starting at 1)

WL = wavelength of peak

TBAR = output of absorption correction (-log(transmission)/mu)

CURHST = run number of sample

SEQNUM = peak number (unique number for each peak in file)

TRANSMISSION = output of absorption correction (exp(-mu\*tbar))

DN = detector bank number

TWOTH = two-theta scattering angle

DSP = d-Spacing of peak (Angstroms)/TR

Last line must have all 0's


Usage
-----

**Example**

.. testcode:: ExLoadHKL

    import os

    wsPeaks = LoadIsawPeaks('TOPAZ_3007.peaks')
    SaveHKL(wsPeaks, Filename='testHKL.hkl')
    wsHKL = LoadHKL('testHKL.hkl')

    #remove the file we created
    alg = wsHKL.getHistory().lastAlgorithm()
    filePath = alg.getPropertyValue("Filename")
    os.remove(filePath)

.. categories::

.. sourcelink::
