.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

SaveHKL outputs the peaks with corrections applied in a format
that works successfully in GSAS and SHELX. Peaks that have not been 
integrated and also peaks that were not indexed are removed.

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
**Example - a simple example of running SaveHKL.**

.. testcode:: ExSaveHKLSimple

    import os

    path = os.path.join(os.path.expanduser("~"), "MyPeaks.hkl")

    #load a peaks workspace from file
    peaks = LoadIsawPeaks(Filename=r'Peaks5637.integrate')
    SaveHKL(peaks, path)

    print(os.path.isfile(path))

Output:

.. testoutput:: ExSaveHKLSimple

    True

.. testcleanup:: ExSaveHKLSimple

    import os
    def removeFiles(files):
      for ws in files:
        try:
          path = os.path.join(os.path.expanduser("~"), ws)
          os.remove(path)
        except:
          pass

    removeFiles(["MyPeaks.hkl"])

**Example - an example of running SaveHKL with sorting and filtering options.**

.. testcode:: ExSaveHKLOptions

    import os

    #load a peaks workspace from file
    peaks = LoadIsawPeaks(Filename=r'Peaks5637.integrate')
    print("Number of peaks in table {}".format(peaks.rowCount()))
    
    path = os.path.join(os.path.expanduser("~"), "MyPeaks.hkl")
    SaveHKL(peaks, path, MinWavelength=0.5, MaxWavelength=2,MinDSpacing=0.2, SortBy='Bank')

    peaks = LoadHKL(path)
    print("Number of peaks in table {}".format(peaks.rowCount()))

Output:

.. testoutput:: ExSaveHKLOptions

    Number of peaks in table 434
    Number of peaks in table 234

.. testcleanup:: ExSaveHKLOptions

    import os
    def removeFiles(files):
      for ws in files:
        try:
          path = os.path.join(os.path.expanduser("~"), ws)
          os.remove(path)
        except:
          pass

    removeFiles(["MyPeaks.hkl"])



.. categories::

.. sourcelink::
