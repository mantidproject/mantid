.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm can:

#. Add an Instrument to a Workspace without any real instrument associated with it, or
#. Replace a Workspace's Instrument with a new Instrument, or
#. Edit all detector parameters of the instrument associated with a Workspace.  (Partial instrument editing is not supported.)

Requirements on input properties
--------------------------------

#. PrimaryFightPath (L1): If it is not given, L1 will be the distance
   between source and sample in the original instrument. Otherwise, L1 is
   read from input. The source position of the modified instrument is (0,
   0, -L1);
#. SpectrumIDs: If not specified (empty list), then Spectrum Numbers will be
   set up to any array such that SpectrumNos[wsindex] is the spectrum Number of
   workspace index 'wsindex';
#. L2 and Polar cannot be empty list;
#. SpectrumIDs[i], L2[i], Polar[i], Azimuthal[i] and optional
   DetectorIDs[i] correspond to the detector of a same spectrum.
#. Angles are specified in degrees.

Limitations
-----------

There are some limitations of this algorithm.

#. The key to locate the detector is via spectrum Number;
#. For each spectrum, there is only one and only one new detector. Thus, if one spectrum is associated with a group of detectors previously, the replacement (new) detector is the one which is (diffraction) focused on after this algorithm is called.

Usage
-----

**Example - Adding a new instrument to a workspace**

.. testcode:: AddExample

    import numpy
    ws = CreateWorkspace(
        DataX=[0., 1.],
        DataY=[1., 2., 3.],
        NSpec=3)
    EditInstrumentGeometry(
        ws,
        PrimaryFlightPath=5.,
        SpectrumIDs=[1, 2, 3],
        L2=[2.0, 2.3, 2.6],
        Polar=[10.0, 15.0, 23.0],
        Azimuthal=[0.0, 0.0, 0.0],
        DetectorIDs=[100, 101, 102],
        InstrumentName='Bizarrio')
    spectrumInfo = ws.spectrumInfo()
    for i in range(ws.getNumberHistograms()):
        print('Histogram {} scattering angle: {:.3} degrees'.format(i + 1, numpy.rad2deg(spectrumInfo.twoTheta(i))))

Output:

.. testoutput:: AddExample

    Histogram 1 scattering angle: 10.0 degrees
    Histogram 2 scattering angle: 15.0 degrees
    Histogram 3 scattering angle: 23.0 degrees

.. categories::

.. sourcelink::
