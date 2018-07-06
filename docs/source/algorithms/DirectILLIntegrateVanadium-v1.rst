.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm integrates the workspace given in *InputWorkspace* using the :ref:`ComputeCalibrationCoefVan <algm-ComputeCalibrationCoefVan>` algorithm. It is part of :ref:`ILL's direct geometry reduction algorithms <DirectILL>`.

.. note::
    At the moment, the integration range is fixed to :math:`\pm` 3 * FWHM (:math:`2\sqrt{2 \ln 2}`) times the 'Sigma' column in *EPPWorkspace*).

Input workspaces
################

The *InputWorkspace* should be loaded using the :ref:`DirectILLCollectData <algm-DirectILLCollectData>` algorithm. It will also give the EPP workspace  needed for *EPPWorkspace*.

Vanadium temperature
####################

A correction for the Debye-Waller factor is applied to the integrated vanadium, as explained in the documentation of :ref:`ComputeCalibrationCoefVan <algm-ComputeCalibrationCoefVan>`. The temperature for the DWF calculation is taken from the 'Sample.temperature' sample log of the *InputWorkspace*. This value can be overriden by the *Temperature* property, if needed.

Usage
-----

**Example - Integrating fake IN4 workspace**

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

    # Prepare the workspace for integration.
    # We also need the elastic peak position table (EPP).
    DirectILLCollectData(
        InputWorkspace=ws,
        OutputWorkspace='preprocessed',
        ElasticChannelWorkspace=elasticChannelWS,
        IncidentEnergyCalibration='Energy Calibration OFF', # Normally enabled for IN4.
        OutputEPPWorkspace='epps'
    )

    DirectILLIntegrateVanadium(
        InputWorkspace='preprocessed',
        OutputWorkspace='norm-factors',
        EPPWorkspace='epps',
        DebyeWallerCorrection='Correction OFF',
        Temperature=293
    )

    norms = mtd['norm-factors']
    print('Integrated vanadium contains {} bin in each of {} histograms.'
        .format(norms.blocksize(), norms.getNumberHistograms()))

Output:

.. testoutput:: FakeIN4Example

    Integrated vanadium contains 1 bin in each of 396 histograms.

.. categories::

.. sourcelink::
