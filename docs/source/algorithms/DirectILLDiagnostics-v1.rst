.. algorithm::

.. summary::

.. alias::

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

Usage
-----

**Example - Not implemented**

.. categories::

.. sourcelink::
