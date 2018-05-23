.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs detector diagnostics and masking. It is part of :ref:`ILL's direct geometry data reduction suite <DirectILL>`. The diagnostics are calculated using the counts from *InputWorkspace* which is preferably the raw workspace provided by the *OutputRawWorkspace* property of :ref:`DirectILLCollectData <algm-DirectILLCollectData>`. The output is a special mask workspace which can be further fed to :ref:`DirectILLReduction <algm-DirectILLReduction>` to mask the detectors diagnosed as bad. Optionally, an instrument specific default mask, a beam stop mask and/or a user specified hard mask given by *MaskedDetectors* or *MaskedComponents* can be added to the diagnostics mask.

A workflow diagram for the diagnostics is shown below:

.. diagram:: DirectILLDiagnostics-v1_wkflw.dot

Diagnostics performed
#####################

The algorithm performs two tests for each spectrum in *InputWorkspace*: elastic peak diagnostics and flat background diagnostics. Basically both tests calculate the median of the test values over all spectra, then compare the individual values to the median. For more detailed information, see :ref:`MedianDetectorTest <algm-MedianDetectorTest>`.

Elastic peak diagnostics
^^^^^^^^^^^^^^^^^^^^^^^^

The EPP table given in *EPPWorkspace* and the value of *ElasticPeakWidthInSigmas* are used to integrate the spectra around the elastic peaks, giving the elastic intensities. The intensities are further normalised by the opening solid angles of the detectors, given by :ref:`SolidAngle <algm-SolidAngle>` before the actual diagnostics.

Flat background diagnostics
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Similarly to elastic peak diagnostics, *EPPWorkspace* and *NonBgkRegionInSigmas* are used to integrate the time-independent background regions of *InputWorkspace*. *NonBkgRegionInSigmas* is a factor applied to the 'Sigma' column in *EPPWorkspace* and this interval around the elastic peak positions is excluded from the integration. No opening angle corrections are applied to the background diagnostics.

Beam stop
#########

The shadow cast on the detectors by a beam stop can be masked by the diagnostics, as well. This functionality is automatically enabled when 'beam_stop_diagnostics_spectra' instrument parameter is defined and can be disabled by *BeamStopDiagnostics*. The algorithm tries to mask a continuous region within the spectra listed in 'beam_stop_diagnostics_spectra'. The *BeamStopThreshold* property can be used to fine-tune the operation.

The 'beam_stop_diagnostics_spectra' instrument parameter lists ranges of spectrum numbers. Each range should cover a region of a physical detector tube, part of which is behind the beam stop.

The masking procedure proceeds as follows:

#. Pick a range from 'beam_stop_diagnostics_spectra'.
#. Integrate the spectra within the range.
#. Divide the range into two halves from the middle.
#. Pick one of the halves, take the maximum integrated value.
#. Starting from the spectrum containing the maximum value, and stepping towards the center of the range, find the first spectrum where the integrated intensity is less than the maximum intensity multiplied by *BeamStopThreshold*. Lets call this the threshold spectrum.
#. Mark all spectra from the middle of the range to the threshold spectrum as masked.
#. Repeat for the other half.

Defaul mask
###########

The default mask file is defined by the 'Workflow.MaskFile' instrument parameter.

Currently, there is a default mask available for ILL's IN5 instrument which masks 8 pixels at both ends of every detector tube.


Diagnostics reporting
#####################

The optional *OutputReportWorkspace* property returns a table workspace summarizing the diagnostics. The table has six columns:

#. 'WorkspaceIndex'
#. 'UserMask': Holds non-zero values for spectra masked by the default mask, *MaskedDetectors* and *MaskedComponents*.
#. 'ElasticIntensity': Holds the value of integrated elastic peaks used for the diagnostics.
#. 'IntensityDiagnosed': Holds non-zero values for spectra diagnosed as 'bad' in elastic peak diagnostics.
#. 'FlagBkg': Holds the value of the flat backgrounds used for the diagnostics.
#. 'FlatBkgDiagnosed': Non-zero values in this column indicate that the spectrum did not pass the background diagnostics.

The columns can be plotted to get an overview of the diagnostics.

Additionally, a string listing the masked and diagnosed detectors can be accessed via the *OutputReport* property.

ILL's instrument specific defaults
----------------------------------

The following settings are used when the :literal:`AUTO` keyword is encountered:

+------------------------+---------------------------+--------------------------+---------------------------+---------------------------+
| Property               | IN4                       | IN5                      | IN6                       | Ohters                    |
+========================+===========================+==========================+===========================+===========================+
| ElasticPeakDiagnostics | Peak Diagnostics ON       | Peak Diagnostics OFF     | Peak Diagnostics ON       | Peak Diagnostics ON       |
+------------------------+---------------------------+--------------------------+---------------------------+---------------------------+
| BkgDiagnostics         | Bkg Diagnostics ON        | Bkg Diagnostics OFF      | Bkg Diagnostics ON        | Bkg Diagnostics ON        |
+------------------------+---------------------------+--------------------------+---------------------------+---------------------------+
| BeamStopDiagnostics    | Beam Stop Diagnostics OFF | Beam Stop Diagnostics ON | Beam Stop Diagnostics OFF | Beam Stop Diagnostics OFF |
+------------------------+---------------------------+--------------------------+---------------------------+---------------------------+

Usage
-----

**Example - Diagnostics on fake IN4 workspace**

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
    # Set some histograms to zero to see if the diagnostics can catch them.
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
    # Elastic channel information is missing in the sample logs.
    # It can be given as single valued workspace, as well.
    elasticChannelWS = CreateSingleValuedWorkspace(107)
    
    DirectILLCollectData(
        InputWorkspace=ws,
        OutputWorkspace='preprocessed',
        ElasticChannelWorkspace=elasticChannelWS,
        IncidentEnergyCalibration='Energy Calibration OFF', # Normally we would do this for IN4.
        OutputEPPWorkspace='epps' # Needed for the diagnostics.
    )
    
    diagnostics = DirectILLDiagnostics(
        InputWorkspace='preprocessed',
        OutputWorkspace='diagnosed',
        EPPWorkspace='epps',
        NoisyBkgLowThreshold=0.01,
        OutputReportWorkspace='diagnostics_report'
    )
    
    print(diagnostics.OutputReport)
    print('Some small-angle detectors got diagnosed as bad due to detector solid angle corrections.')
    report = mtd['diagnostics_report']
    I0 = report.cell('ElasticIntensity', 0)
    I304 = report.cell('ElasticIntensity', 303)
    print('Solid-angle corrected elastic intensity of spectrum 1: {:.8}'.format(I0))
    print('vs. corrected intensity of spectrum 304: {:.8}'.format(I304))

Output:

.. testoutput:: FakeIN4Example

    Spectra masked by default mask file:
    None
    Spectra masked by beam stop diagnostics:
    None
    Additional spectra marked as bad by elastic peak diagnostics:
    14, 102, 302-305, 314-317, 326-329, 338-341, 350-353, 362-365, 374-377, 386-389
    Additional spectra marked as bad by flat background diagnostics:
    14, 102
    Some small-angle detectors got diagnosed as bad due to detector solid angle corrections.
    Solid-angle corrected elastic intensity of spectrum 1: 555524.7
    vs. corrected intensity of spectrum 304: 1795774.9

.. categories::

.. sourcelink::
