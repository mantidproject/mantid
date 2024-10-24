
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm generates an elastic peak position (EPP) table like the one created by the :ref:`FindEPP <algm-FindEPP>` algorithm. Unlike :ref:`FindEPP <algm-FindEPP>` which finds the elastic peak positions by fitting, CreateEPP calculates the peak position from the instrument geometry and incident energy. The algorithm is can be used in cases where :ref:`FindEPP <algm-FindEPP>` fails e.g. due to poor signal-to-noise ratio.

The ideal elastic peak position :math:`t_{EPP}` corresponds to the time-of-flight between the source and the detector:

:math:`t_{EPP} = (l_1 + l_2) / v`,

where :math:`l_1` is the source-to-sample distance, :math:`l_2` is the sample-to-detector distance and :math:`v` is the neutron velocity. The velocity in turn is given by

:math:`v = \sqrt{2 E_i / m_n}`,

where :math:`E_i` is the incident energy and :math:`m_n` the neutron mass.

CreateEPP reads :math:`E_i` from the 'Ei' sample log entry of *InputWorkspace*.

The columns in the *OutputWorkspace* are the same as in :ref:`FindEPP <algm-FindEPP>`. Since the calculation done by CreateEPP gives only the ideal elastic peak positions, the rest of the columns are filled as follows:

- 'Sigma': set to the *Sigma* input property.
- 'PeakCentreError', 'SigmaError' and 'HeightError': set unconditionally to zero.
- 'Height': set to the Y value of the bin at the elastic peak position.
- 'chiSq': set unconditionally to one.
- 'FitStatus': set unconditionally to "success".

Usage
-----

**Example - CreateEPP**

.. testcode:: CreateEPPExample

    from mantid.kernel import DeltaEModeType, UnitConversion
    import numpy

    CreateSampleWorkspace(
        OutputWorkspace='exWS',
        Function='Flat background',
        NumBanks=3,
        BankPixelWidth=2,
        Xmax=10000,
        BinWidth=50,
        PixelSpacing=0.01,
        BankDistanceFromSample=1.0
    )
    ws = mtd['exWS']
    # Fill the sample workspace with some Gaussian elastic peaks.
    Ei = 10.0  # Incident energy, in meV.
    def peak(xs, centre):  # A helper function to generate a peak.
        xs = (xs[:-1] + xs[1:]) * 0.5  # Convert bin edges to bin centres.
        ys = 10.0 * numpy.exp(-numpy.square((xs - centre) / 100))
        return ys
    instrument = ws.getInstrument()
    source = instrument.getSource()
    sample = instrument.getSample()
    L1 = sample.getDistance(source)
    for i in range(ws.getNumberHistograms()):
        detector = ws.getDetector(i)
        L2 = sample.getDistance(detector)
        tof = UnitConversion.run('Energy', 'TOF', Ei, L1, L2, 0.0, DeltaEModeType.Direct, Ei)
        ys =ws.dataY(i)
        ys += peak(ws.dataX(i), tof)
    # The 'Ei' sample log shall hold the incident energy.
    ws.mutableRun().addProperty('Ei', Ei, True)

    # Compare CreateEPP and FindEPP results.
    createEPPWS = CreateEPP(InputWorkspace=ws)
    findEPPWS = FindEPP(InputWorkspace='exWS')

    epp1 = createEPPWS.cell('PeakCentre', 0)
    print('CreateEPP gives {0:.8f} as the first elastic peak position.'.format(epp1))
    epp2 = findEPPWS.cell('PeakCentre', 0)
    print('FindEPP gives {0:.8f}.'.format(epp2))

Output:

.. testoutput:: CreateEPPExample

    CreateEPP gives 7952.80766719 as the first elastic peak position.
    FindEPP gives ....

.. categories::

.. sourcelink::
