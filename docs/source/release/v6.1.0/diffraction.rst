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

- The :ref:`ConvertUnits <algm-ConvertUnits>` algorithm has been extended to use a quadratic relationship between d spacing and TOF when doing conversions between these units. The diffractometer constants DIFA, DIFC and TZERO that determine the form of the quadratic can be loaded into a workspace using a new :ref:`ApplyDiffCal <algm-ApplyDiffCal>` algorithm. This functionality was previously only available in :ref:`AlignDetectors <algm-AlignDetectors>` which only performed the conversion in the direction TOF to d spacing. This change will ensure that the conversion of focussed datasets from d spacing back to TOF at the end of the ISIS powder diffraction data reduction is performed correctly.

Engineering Diffraction
-----------------------

Single Crystal Diffraction
--------------------------
- New version of algorithm :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` provides more accurate calibration results for CORELLI instrument.

:ref:`Release 6.1.0 <v6.1.0>`