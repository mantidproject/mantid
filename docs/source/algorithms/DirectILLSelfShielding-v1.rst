.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates self shielding correction factors for the input workspace. It is part of :ref:`ILL's direct geometry reduction suite <DirectILL>`. *InputWorkspace* should have a sample defined using :ref:`SetSample <algm-SetSample>`. Beam profile can be optionally set using :ref:`SetBeam <algm-SetBeam>`. The algorithm uses :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` as its backend.

To speed up the simulation, the sparse instrument option of :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` is used by default. The number of detectors to simulate can be given by *SparseInstrumentRows* and *SparseInstrumentColumns*.

By default the correction factors are calculated for each bin. By specifying *NumberOfSimulatedWavelengths*, one can restrict the number of points at which the calculation is done thus reducing the execution time. In this case CSplines are used to interpolate the correction factor over all bins.

The correction factor contained within the *OutputWorkspace* can be further fed to :ref:`DirectILLApplySelfShielding <algm-DirectILLApplySelfShielding>`.

Usage
-----

**Example - Absorption corrections of fake IN4 workspace**

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
    
    DirectILLCollectData(
        InputWorkspace=ws,
        OutputWorkspace='preprocessed',
        ElasticChannelWorkspace=elasticChannelWS,
        IncidentEnergyCalibration='Energy Calibration OFF', # Normally we would do this for IN4.
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
    # The correction factors should be applied using DirectILLApplySelfShielding.
    corrections = mtd['absorption_corrections']
    f_short = corrections.readY(0)[0]
    f_long = corrections.readY(0)[-1]
    print('Absoprtion corrections factors for detector 1')
    print('Short final wavelengths: {:.4f}'.format(f_short))
    print('Long final wavelengths:  {:.4f}'.format(f_long))

Output:

.. testoutput:: FakeIN4Example
   :options: +ELLIPSIS +NORMALIZE_WHITESPACE

    Absoprtion corrections factors for detector 1
    Short final wavelengths: 0.4...
    Long final wavelengths:  0.2...

.. categories::

.. sourcelink::
