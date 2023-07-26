.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

SaveHKL outputs the peaks with corrections applied in a format
that works successfully in GSAS and SHELX. Peaks that have not been
integrated and also peaks that were not indexed are removed.

`hklFile.write('%4d%4d%4d%8.2f%8.2f%4d%8.4f%7.4f%7d%7d%7.4f%4d%9.5f%9.4f\\n'%
(H, K, L, FSQ, SIGFSQ, hstnum, WL, TBAR, CURHST, SEQNUM, TRANSMISSION,
DN, TWOTH, DSP))`

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

Direction cosines are required for certain absorption and extinction corrections. They are calculated according to [1]_
and are the dot products of the scattered and reverse incident beam directions with the direction of the reciprocal
lattice vectors. The components are interleaved starting with the reverse incident part.

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
    os.remove(path)

**Example - an example of running SaveHKL with sorting and filtering options.**

.. testcode:: ExSaveHKLOptions

    import os

    #load a peaks workspace from file
    peaks = LoadIsawPeaks(Filename=r'Peaks5637.integrate')
    print("Number of peaks in table {}".format(peaks.rowCount()))

    path = os.path.join(os.path.expanduser("~"), "MyPeaks.hkl")
    SaveHKL(peaks, path, MinWavelength=0.5, MaxWavelength=2, MinDSpacing=0.2, SortBy='Bank')

    peaks = LoadHKL(path)
    print("Number of peaks in table {}".format(peaks.rowCount()))

Output:

.. testoutput:: ExSaveHKLOptions

    Number of peaks in table 434
    Number of peaks in table 234

.. testcleanup:: ExSaveHKLOptions

    import os
    os.remove(path)


**Example - SaveHKL with shape from SetSample**

.. testcode:: ExSaveHKLSetSample

    import os
    path = os.path.join(os.path.expanduser("~"), "MyPeaks.hkl")

    # load a peaks workspace from file
    peaks = LoadIsawPeaks(Filename=r'SXD23767.peaks')
    SetSample(peaks,
              Geometry={'Shape': 'Cylinder', 'Height': 4.0,
                        'Radius': 0.8,
                        'Center': [0.,0.,0.]},
              Material={'ChemicalFormula': 'V', 'SampleNumberDensity': 0.1})
    SaveHKL(peaks, path)
    print(os.path.isfile(path))

Output:

.. testoutput:: ExSaveHKLSetSample

    True

.. testcleanup:: ExSaveHKLSetSample

    import os
    os.remove(path)


References
----------

.. [1] A. Katrusiak, *Absorption Correction for Crystal-Environment Attachments from Direction Cosines*, Zeitschrift für Kristallographie - Crystalline Materials, **216** (2001) 646–647. doi: `10.1524/zkri.216.12.646.22488 <http://dx.doi.org/10.1524/zkri.216.12.646.22488>`_


.. categories::

.. sourcelink::
