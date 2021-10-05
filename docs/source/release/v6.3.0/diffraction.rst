===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Powder Diffraction
------------------
- `GetDetOffsetsMultiPeaks`, which is deprecate since v6.2.0, is removed.
- `CalibrateRectangularDetectors`, which is deprecate since v6.2.0, is removed. And system test CalibrateRectangularDetectors_Test is removed.
- Extending :ref:`MultipleScatteringCorrection <algm-MultipleScatteringCorrection>` to the sample and container case.

Bugfixes
########
- For processing vanadium run, we don't want to find environment automatically in :ref:`SetSampleFromLogs <algm-SetSampleFromLogs>`.
- Identification in :ref:`AlignComponents <algm-AlignComponents>` of the first and last detector-ID for an instrument component with unsorted detector-ID's.

Engineering Diffraction
-----------------------

Improvements
############
- Improved axes scaling in the plot of the :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` :ref:`Fitting tab <ui engineering fitting>`.
- Automatically disable zoom and pan when opening the fit browser in the :ref:`Fitting tab <ui engineering fitting>` of the Engineering Diffraction interface (as they interfered with the interactive peak adding tool).


Single Crystal Diffraction
--------------------------
- Existing :ref:`PolDiffILLReduction <algm-PolDiffILLReduction>` and :ref:`D7AbsoluteCrossSections <algm-D7AbsoluteCrossSections>` can now reduce and properly normalise single-crystal data for the D7 ILL instrument.

:ref:`Release 6.3.0 <v6.3.0>`
