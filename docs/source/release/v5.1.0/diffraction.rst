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

- Polaris.create_total_scattering_pdf output workspaces now have the run number in the names.
- Polaris.create_total_scattering_pdf no longer takes `output_binning` as a parameter, instead binning of the output pdf can be controlled with `delta_r`.
- Polaris.create_total_scattering_pdf can rebin the Q space workspace before calculating the PDF by being given an input `delta_q`.
- Polaris.create_total_scattering_pdf fourier filter can be performed using the butterworth filter by calling with `bw_order`.
- SampleDetails.set_materials now differentiates between sample density and crystal density for converting between pdf types.
- :ref:`LoadWAND <algm-LoadWAND>` now adds `duration` log to the workspace
- Polaris.create_total_scattering_pdf now no longer calculates the PDF with the Lorch filter enabled by default and must be enabled.

Engineering Diffraction
-----------------------
Improvements
^^^^^^^^^^^^
- TOPAS files (`.abc`) have replaced the `.dat` files generated when focusing using the GUI.
- Focusing with the GUI will now generate a CSV containing the averaged values of all numerical sample logs.
- The currently loaded calibration is now shown at the bottom of the GUI.
- The location of the saved output files from the GUI is now shown in the messages log.

Single Crystal Diffraction
--------------------------
Improvements
^^^^^^^^^^^^
- :ref:`CombinePeaksWorkspaces <algm-CombinePeaksWorkspaces>` now combines the modulation vectors present in the two workspaces, provided the total number of vectors is less than 3.
- New algorithm :ref:`FindGoniometerFromUB <algm-FindGoniometerFromUB-v1>` for making UBs for runs at different goniometer angles share common indexing and determine the goniometer axis and rotation required to match UBs to a reference.
- New instrument geometry for MaNDi instrument at SNS
- New algorithm :ref:`AddAbsorptionWeightedPathLengths <algm-AddAbsorptionWeightedPathLengths-v1>` for calculating the absorption weighted path length for each peak in a peaks workspace. The absorption weighted path length is used downstream from Mantid in extinction correction calculations

:ref:`Release 5.1.0 <v5.1.0>`
