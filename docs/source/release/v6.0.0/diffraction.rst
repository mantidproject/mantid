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

Engineering Diffraction
-----------------------

Single Crystal Diffraction
--------------------------

- Fix problem that was causing matrix diagonalization to return NaNs in certain cases. The diagonalization is used in :ref:`CalculateUMatrix <algm-CalculateUMatrix>` and :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>`

Imaging
-------

:ref:`Release 6.0.0 <v6.0.0>`