.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs detector diagnostics for the workspace provided by *InputWorkspace*, preferably the raw workspace provided by the *OutputRawWorkspace* property in :ref:`DirectILLCollectData <algm-DirectILLCollectData>`. The output is a mask workspace which can be further fed to :ref:`DirectILLReduction <algm-DirectILLReduction>` to mask the detectors diagnosed as bad. Optionally, an instrument specific default mask or a user specified hard mask given by *MaskedDetectors* or *MaskedComponents* can be added to the diagnostics mask. A workflow diagram for the diagnostics is shown below:

.. diagram:: DirectILLDiagnostics-v1_wkflw.dot

Diagnostics performed
#####################

The algorithm does two test for each spectrum in *InputWorkspace*: elastic peak diagnostics and flat background diagnostics. Basically both tests calculate the median of the test values over all spectra, then compare the individual values to the median. For more detailed information, see :ref:`MedianDetectorTest <algm-MedianDetectorTest>`.

Elastic peak diagnostics
^^^^^^^^^^^^^^^^^^^^^^^^

The EPP table given in *EPPWorkspace* and the value of *ElasticPeakWidthInSigmas* are used to integrate the spectra around the elastic peaks, giving the elastic intensities. The intensities are further normalised by the opening solid angles of the detectors, given by :ref:`SolidAngle <algm-SolidAngle>` before the actual diagnostics.

Flat background diagnostics
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Similarly to elastic peak diagnostics, *EPPWorkspace* and *NonBgkRegionInSigmas* are used to integrate the time-independent background regions of *InputWorkspace*. *NonBkgRegionInSigmas* is a factor applied to the 'Sigma' column in *EPPWorkspace* and this interval around the elastic peak positions is excluded from the integration. No opening angle corrections are applied to the background diagnostics.

By default, the background diagnostics are disabled for IN5.

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
