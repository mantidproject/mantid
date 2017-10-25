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

- The ``CalibrationFile`` is now optional in :ref:`SNSPowderReduction <algm-SNSPowderReduction>`. In this case time focussing will use :ref:`ConvertUnits <algm-ConvertUnits>` and the instrument geometry. Care must be taken to supply a ``GroupingFile`` otherwise all of the spectra will be kept separate.
- New algorithm :ref:`algm-EstimateDivergence` estimates the beam divergence due to finite slit size
- :ref:`PDCalibration <algm-PDCalibration>` returns two more diagnostic workspaces: one for the fitted peak heights, one for the fitted peak widths.

Engineering Diffraction
-----------------------

Single Crystal Diffraction
--------------------------

- HB3A reduction interface has been enhanced.  A child window is added to it for users to pre-process scans and save the processed and merged data to NeXus files in order to save time when they start to reduce and visualize the data.

Imaging
-------

:ref:`Release 3.12.0 <v3.12.0>`
