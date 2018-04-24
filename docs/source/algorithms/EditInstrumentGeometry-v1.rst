.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm can

| ``1. add an Instrument to a Workspace without any real instrument associated with, or``
| ``2. replace a Workspace's Instrument with a new Instrument, or``
| ``3. edit all detectors' parameters of the instrument associated with a Workspace (partial instrument editing is not supported). ``

Requirements on input properties
--------------------------------

1. PrimaryFightPath (L1): If it is not given, L1 will be the distance
between source and sample in the original instrument. Otherwise, L1 is
read from input. The source position of the modified instrument is (0,
0, -L1);

2. SpectrumIDs: If not specified (empty list), then Spectrum Numbers will be
set up to any array such that SpectrumNos[wsindex] is the spectrum Number of
workspace index 'wsindex';

3. L2 and Polar cannot be empty list;

4. SpectrumIDs[i], L2[i], Polar[i], Azimuthal[i] and optional
DetectorIDs[i] correspond to the detector of a same spectrum.

5. Angles are specified in degrees.

Limitations
-----------

There are some limitations of this algorithm.

1. The key to locate the detector is via spectrum Number;

2. For each spectrum, there is only one and only one new detector. Thus,
if one spectrum is associated with a group of detectors previously, the
replacement (new) detector is the one which is (diffraction) focused on
after this algorithm is called.

Instruction
-----------

1. For powder diffractomer with 3 spectra, user can input

| ``  SpectrumIDs = "1, 3, 2"``
| ``  L2 = "3.1, 3.2, 3.3"``
| ``  Polar = "90.01, 90.02, 90.03"``
| ``  Azimuthal = "0.1,0.2,0.3"``
| ``  to set up the focused detectors' parameters for spectrum 1, 3 and 2.``

.. categories::

.. sourcelink::
