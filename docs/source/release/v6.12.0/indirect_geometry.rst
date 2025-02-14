=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:


Bugfixes
--------
- Removed the unused ``IndirectNeutron.py`` file.


Algorithms
----------

New features
############
- :ref:`AbinsDataFormats` documentation has been updated with information about supported codes and features for imported phonon data.

Bugfixes
############
- :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer-v1>` now outputs a warning when a calibration workspace has zeroes for one or more spectra.
- :ref:`LoadVesuvio <algm-LoadVesuvio-v1>` now supports multiple run ranges in the ``Filename`` property,
  for example ``43066-43070, 43072-43074``.
- :ref:`Abins <algm-Abins-v1>` and :ref:`Abins2D <algm-Abins2D-v1>` isotope identification improvements:

  - Previously, a species would only be identified as the standard
    isotopic mixture if the mass is very close to the Mantid reference
    data. In some cases, the values used by external phonon calculators
    are significantly different and this could lead to misassignment
    (e.g. Zn in CASTEP). ``Abins`` will now initially choose the *nearest*
    mass option between an isotope and the standard isotopic mixture.
  - Many isotopes lack cross-section data and would lead to NaN
    intensities. Now, if a NaN cross-section is identified, ``Abins``
    will either use the standard mixture data (if the mass is within
    0.01) or raise an error.
- :ref:`Abins <algm-Abins-v1>` and :ref:`Abins2D <algm-Abins2D-v1>` now provide a more helpful error message when the Default Save Directory is unwriteable (a writeable save directory is required for caching data).

:ref:`Release 6.12.0 <v6.12.0>`
