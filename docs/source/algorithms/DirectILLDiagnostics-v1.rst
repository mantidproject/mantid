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

The workspace in *InputWorkspace* should be loaded using the :ref:`DirectILLPrepareData <algm-DirectILLPrepareData>` algorithm. It also provides the data needed for *EPPWorkspace* and *FlatBkgWorkspace*.

Usage
-----

**Example - Not implemented**

.. categories::

.. sourcelink::
