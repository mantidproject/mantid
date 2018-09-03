.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is the main data reduction algorithm in :ref:`ILL's time-of-flight reduction suite <DirectILL>`. It performs the last steps of the reduction workflow, namely vanadium normalisation and transformation to :math:`S(q,\omega)` space (optionally :math:`S(2\theta,\omega)`). The algorithm's workflow diagram is shown below:

.. diagram:: DirectILLReduction-v1_wkflw.dot

Input workspaces
################

*InputWorkspace* should contain data treated by :ref:`DirectILLCollectData <algm-DirectILLCollectData>` and, optionally, by :ref:`DirectILLApplySelfShielding <algm-DirectILLApplySelfShielding>`.

The mandatory *IntegratedVanadiumWorkspace* should have gone through :ref:`DirectILLIntegrateVanadium <algm-DirectILLIntegrateVanadium>`. This workspace is used for the vanadium normalisation.

*DiagnosticsWorkspace* should be a product of :ref:`DirectILLDiagnostics <algm-DirectILLDiagnostics>`. It is used to mask the spectra of *InputWorkspace*.

Outputs
#######

The algorithm will transform the time-of-flight and spectrum numbers of *InputWorkspace* into :math:`S(q,\omega)` at its output. For :math:`2\theta` to :math:`q` transformation, :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` is used. By default, the output is transposed by :ref:`Transpose <algm-Transpose>`. This behavior can be turned off by the *Transpose* property.

The optional :math:`S(2\theta,\omega)` output can be enabled by the *OutputSofThetaEnergyWorkspace*.

Normalisation to absolute units
###############################

Normalisation to absolute units can be enabled by setting *AbsoluteUnitsNormalisation* to :literal:`'Absolute Units ON'`. In this case the data is multiplied by a factor

    :math:`f = \frac{N_V \sigma_V}{N_S}`

after normalisation to vanadium giving units of barn to the data. In the above, :math:`N_V` stands for the vanadium number density, :math:`\sigma_V` for vanadium total scattering cross section and :math:`N_S` sample number density. 

The material properties should be set for *InputWorkspace* and *IntegratedVanadiumWorkspace* by :ref:`SetSample <algm-SetSample>` before running this algorithm .

(Re)binning in energy and momentum transfer
###########################################

After conversion from time-of-flight to energy transfer, the binning may differ from spectrum to spectrum if the sample to detector distances are unequal. The :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` algorithm cannot work with such ragged workspaces and thus rebinning is necessary. The rebinning can be specified by the *EnergyRebinningParams* property. This is directly passed to :ref:`Rebin <algm-Rebin>` as the *Params* property. If *EnergyRebinningParams* is not specified, an automatic rebinning scheme is used:
- Find the spectrum with smallest bin border. Copy binning from this spectrum for negative energy transfers.
- For positive energy transfers, use the median bin width at zero energy transfer.

*QBinningParams* are passed to :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` and have the same format as *EnergyRebinningParamas*. If the property is not specified, :math:`q` is binned to a value that depends on the :math:`2\theta` separation of the detectors and the wavelength.

Transposing output
##################

After conversion to momentum transfer, the vertical axis of the data is in units of momentum transfer while the horizontal axis is in energy transfer. By default, the data is transposed such that momentum transfer is on the horizontal axis and energy transfer in the vertical. This can be turned off by setting *Transposing* to :literal:`'Transposing OFF'`.

Usage
-----

**Example - Fake IN4 workspace reduction**

.. testcode:: FakeIN4Example

    from mantid.kernel import DeltaEModeType, UnitConversion
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
    # Set some histograms to zero for detector diagnostics.
    ys = ws.dataY(13)
    ys *= 0.0
    ys = ws.dataY(101)
    ys *= 0.0
    
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
    # Add wavelength to sample logs
    wl = UnitConversion.run('Energy', 'Wavelength', 57.0, 0.0, 0.0, 0.0, DeltaEModeType.Direct, 0.0)
    AddSampleLog(
        Workspace=ws,
        LogName='wavelength',
        LogText=str(wl),
        LogType='Number',
        LogUnit='Angstrom',
        NumberType='Double'
    )
    # Elastic channel information is missing in the sample logs.
    # It can be given as single valued workspace, as well.
    elasticChannelWS = CreateSingleValuedWorkspace(107)
    
    # Create a fake 'vanadium' reference workspace.
    V_ws = Scale(
        InputWorkspace=ws,
        Factor=1.3
    )
    
    # Process vanadium.
    DirectILLCollectData(
        InputWorkspace=V_ws,
        OutputWorkspace='vanadium',
        ElasticChannelWorkspace=elasticChannelWS,
        IncidentEnergyCalibration='Energy Calibration OFF', # Normally we would do this for IN4.
        OutputEPPWorkspace='epps' # Needed for diagnostics and integration.
    )
    
    DirectILLDiagnostics(
        InputWorkspace='vanadium',
        OutputWorkspace='diagnostics_mask',
        EPPWorkspace='epps',
        MaskedComponents='rosace', #Exclude small-angle detectors.
    )
    
    DirectILLIntegrateVanadium(
        InputWorkspace='vanadium',
        OutputWorkspace='vanadium_factors',
        SubalgorithmLogging='Logging ON',
        EPPWorkspace='epps',
        Temperature=273.0
    )
    
    # Process sample.
    DirectILLCollectData(
        InputWorkspace=ws,
        OutputWorkspace='preprocessed',
        ElasticChannelWorkspace=elasticChannelWS,
        IncidentEnergyCalibration='Energy Calibration OFF'
    )
    
    # Absorption corrections and empty container subtractions could be added here.
    
    DirectILLReduction(
        InputWorkspace='preprocessed',
        OutputWorkspace='SofQW',
        IntegratedVanadiumWorkspace='vanadium_factors',
        DiagnosticsWorkspace='diagnostics_mask'
    )
    
    sofqw = mtd['SofQW']
    nHist = sofqw.getNumberHistograms()
    nBin = sofqw.blocksize()
    print('Size of the final S(q,w) workspace: {} histograms, {} bins'.format(nHist, nBin))

Output:

.. testoutput:: FakeIN4Example

    Size of the final S(q,w) workspace: 177 histograms, 260 bins

.. categories::

.. sourcelink::
