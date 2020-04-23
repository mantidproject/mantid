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
- New instrument geometry for MaNDi instrument at SNS

Imaging
-------

:ref:`Release 5.1.0 <v5.1.0>`
