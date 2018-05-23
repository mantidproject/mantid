.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm subtracts empty container data and applies self-shielding corrections to *InputWorkspace*. Both operations are optional: what is actually done depends on the input properties.

This algorithm is part of :ref:`ILL's direct geometry data reduction algorithms <DirectILL>`.

*SelfShieldingCorrectionWorkspace* can be obtained from the :ref:`DirectILLSelfShielding <algm-DirectILLSelfShielding>` algorithm.

Usage
-----

**Example - Absorption corrections and empty container subtraction**

.. testcode:: FakeIN4Example

    import numpy
    import scipy.stats
    
    # Create a fake IN4 workspace.
    # We need an instrument and a template first.
    empty_IN4 = LoadEmptyInstrument(InstrumentName='IN4')
    nHist = empty_IN4.getNumberHistograms()
    # Make TOF bin edges.
    xs = numpy.arange(530.0, 2420.0, 4.0)
    # Make some Gaussian spectra.
    ys = 1000.0 * scipy.stats.norm.pdf(xs[:-1], loc=970, scale=60)
    # Repeat data for each histogram.
    xs = numpy.tile(xs, nHist)
    ys = numpy.tile(ys, nHist)
    ws = CreateWorkspace(
        DataX=xs,
        DataY=ys,
        NSpec=nHist,
        UnitX='TOF',
        ParentWorkspace=empty_IN4
    )
    # Manually correct monitor spectrum number as LoadEmptyInstrument does
    # not know about such details.
    SetInstrumentParameter(
        Workspace=ws,
        ParameterName='default-incident-monitor-spectrum',
        ParameterType='Number',
        Value=str(1)
    )
    # Add incident energy information to sample logs.
    AddSampleLog(
        Workspace=ws,
        LogName='Ei',
        LogText=str(57),
        LogType='Number',
        LogUnit='meV',
        NumberType='Double'
    )
    # Elastic channel information is missing in the sample logs.
    # It can be given as single valued workspace, as well.
    elasticChannelWS = CreateSingleValuedWorkspace(107)
    
    # Create a fake 'empty container' workspace for background subtraction.
    ecws = Scale(
        InputWorkspace=ws,
        Factor=0.1
    )
    
    DirectILLCollectData(
        InputWorkspace=ws,
        OutputWorkspace='preprocessed',
        ElasticChannelWorkspace=elasticChannelWS,
        IncidentEnergyCalibration='Energy Calibration OFF', # Normally we would do this for IN4.
    )
    
    DirectILLCollectData(
        InputWorkspace=ecws,
        OutputWorkspace='preprocessed_ecws',
        ElasticChannelWorkspace=elasticChannelWS,
        IncidentEnergyCalibration='Energy Calibration OFF'
    )
    
    sampleGeometry = {
        'Shape': 'Cylinder',
        'Height': 8.0,
        'Radius': 1.5,
        'Center': [0.0, 0.0, 0.0]
    }
    sampleMaterial = {
        'ChemicalFormula': 'V',
        'SampleNumberDensity': 0.05
    }
    SetSample(
        InputWorkspace='preprocessed',
        Geometry=sampleGeometry,
        Material=sampleMaterial
    )
    
    DirectILLSelfShielding(
        InputWorkspace='preprocessed',
        OutputWorkspace='absorption_corrections',
        SimulationInstrument='Full Instrument', # IN4 is small enough.
        NumberOfSimulatedWavelengths=10
    )
    
    DirectILLApplySelfShielding(
        InputWorkspace='preprocessed',
        OutputWorkspace='absorptionCorrected',
        EmptyContainerWorkspace='preprocessed_ecws',
        SelfShieldingCorrectionWorkspace='absorption_corrections'
    )
    
    preprocessed = mtd['preprocessed']
    maxY = numpy.amax(preprocessed.readY(0))
    print('Elastic peak maximum before corrections: {:.3}'.format(maxY))
    corrected = mtd['absorptionCorrected']
    maxY = numpy.amax(corrected.readY(0))
    print('After empty container subtraction and absorption corrections: {:.3}'.format(maxY))

Output:

.. testoutput:: FakeIN4Example

    Elastic peak maximum before corrections: 26.7
    After empty container subtraction and absorption corrections: 51.5

.. categories::

.. sourcelink::
