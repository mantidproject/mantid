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
- Polaris.create_total_scattering_pdf no longer takes `output_binning` as a parameter, instead binning of the output pdf can be controlled with `delta_r`
- Polaris.create_total_scattering_pdf can rebin the Q space workspace before calculating the PDF by being given an input `delta_q`

Engineering Diffraction
-----------------------
Improvements
^^^^^^^^^^^^
- TOPAS files (`.abc`) have replaced the `.dat` files generated when focusing using the GUI.
- Focusing with the GUI will now generate a CSV containing the averaged values of all numerical sample logs.

Single Crystal Diffraction
--------------------------

Imaging
-------

:ref:`Release 5.1.0 <v5.1.0>`