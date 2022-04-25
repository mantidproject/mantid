.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm performs a fit of lattice parameters using the principle approach described in a paper by Pawley [Pawley]_. In this approach the reflection positions are calculated from lattice parameters and the reflection's Miller indices (:math:`hkl`), while the other profile parameters for each peak are freely refined.

PawleyFit requires a MatrixWorkspace with at least one spectrum in terms of either :math:`d` or :math:`Q`, the index of the spectrum can be supplied to the algorithm as a parameter. Furthermore, the range which is used for refinement can be changed by setting the corresponding properties.

In addition, a TableWorkspace with information about the reflections that are found in the spectrum must be passed as well. There must be four columns with the captions "HKL", "d", "FWHM (rel.)" and "Intensity". The HKL column can be supplied either as V3D or as a string with 3 numbers separated by space, comma or semi-colon and possibly surrounded by square brackets. One way to obtain such a table is to use three algorithms that are used in analysis of POLDI data, which produce tables in a suitable format. Details are given in the usage example section.

Along with the workspaces containing fit results and parameter values, the algorithm also outputs the reduced :math:`\chi^2`-value, which is also written in the log.

Usage
-----

.. include:: ../usagedata-note.txt

For the usage example there is a calculated, theoretical diffraction pattern (including a bit of noise) for Silicon, which crystallizes in space group :math:`Fd\overline{3}m` and has a cubic cell with lattice parameter :math:`a=5.4311946\,\mathrm{\AA{}}`.

It's important to check the output data, which is found in the workspace labeled si_fitted. Plotting it should show that the residuals are just containing background noise and no systematic deviations. Of course, depending on the sample and the measurement this will differ between cases.

.. figure:: /images/PawleyFitResultTheoreticalSilicon.png
   :figwidth: 15 cm
   :align: center
   :alt: Result of the Pawley fit example with silicon.

   Result of the Pawley fit example with silicon.

.. [Pawley] Pawley, G. S. “Unit-Cell Refinement from Powder Diffraction Scans.”, J. Appl. Crystallogr. 14, 1981, 357. doi:10.1107/S0021889881009618.

.. categories::

.. sourcelink::
