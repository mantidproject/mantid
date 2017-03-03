.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs detector diagnostics for the workspace provided by *InputWorkspace*. The output is a mask workspace which can be further fed to :ref:`DirectILLReduction <algm-DirectILLReduction>` to mask the detectors diagnosed as bad. Optionally, a user specified hard mask given by *MaskedDetectors* or *MaskedComponents* can be added to the diagnostics mask. Algorithm's workflow diagram is shown below:

.. diagram:: DirectILLDiagnostics-v1_wkflw.dot

Diagnostics performed
#####################

The algorithm does two test for each spectrum in *InputWorkspace*: elastic peak diagnostics and flat background diagnostics. Basically both tests calculate the median of the test values over all spectra, then compare the individual values to the median. For more detailed information, see :ref:`MedianDetectorTest <algm-MedianDetectorTest>`.

Elastic peak diagnostics
^^^^^^^^^^^^^^^^^^^^^^^^

The EPP table given in *EPPWorkspace* and the value of *ElasticPeakWidthInSigmas* are used to integrate the spectra around the elastic peaks, giving the elastic intensities. The intensities are further normalised by the opening solid angles of the detectors, given by :ref:`SolidAngle <algm-SolidAngle>` before the actual diagnostics.

Flat background diagnostics
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Flat backgrounds in *FlatBkgWorkspace* are used for the diagnostics as-is.

Input workspaces
################

The workspace in *InputWorkspace* should be loaded using the :ref:`DirectILLCollectData <algm-DirectILLCollectData>` algorithm. It also provides the data needed for *EPPWorkspace* and *FlatBkgWorkspace*.

Diagnostics reporting
#####################

The optional *OutputDiagnosticsReportWorkspace* property returns a table workspace summarizing the diagnostics. The table has three columns:

#. 'WorkspaceIndex'
#. 'ElasticIntensity': Holds the value of integrated elastic peaks used for the diagnostics.
#. 'FlagBkg': Holds the value of the flat backgrounds used for the diagnostics.
#. 'Diagnosed': Non-zero values in this column indicate that the spectrum did not pass the diagnostics.

The columns can be plotted to get an overview of the diagnostics.

Usage
-----

**Example - Not implemented**

.. categories::

.. sourcelink::
