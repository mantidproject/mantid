=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------



Bugfixes
--------
- Removed the IndirectNeutron python file because it is unused.
- Added a warning when a calibration workspace used in the `ISISIndirectEnergyTransfer <https://docs.mantidproject.org/nightly/algorithms/ISISIndirectEnergyTransfer-v1.html>`_ algorithm has zeroes for one or more spectra.


Algorithms
----------

New features
############
- New documentation of :ref:`AbinsDataFormats` is added, with information about supported codes and features for imported phonon data.

Bugfixes
############
- :ref:`LoadVesuvio <algm-LoadVesuvio-v1>` now accepts multiple consecutive runs for the ``Filename`` property,
  for example ``43066-43070, 43072-43074`` is the same as running ``43066-43074`` except the run ``43071`` will not be included.
- The identification of isotopes has been improved in Abins/Abins2D.

  - Previously, a species would only be identified as the standard
    isotopic mixture if the mass is very close to the Mantid reference
    data. In some cases the values used by external phonon calculators
    are significantly different and this could lead to misassignment.
    (e.g. Zn in CASTEP.) Now, Abins will initially choose the _nearest_
    mass option between an isotope and the standard isotopic mixture.
  - Many isotopes lack cross-section data and would lead to NaN
    intensities. Now, if a NaN cross-section is identified Abins
    will either use the standard mixture data (if the mass is within
    0.01) or raise an error.
- :ref:`Abins <algm-Abins-v1>` / :ref:`Abins2D <algm-Abins2D-v1>` now gives a more helpful error message when the Default Save Directory is unwriteable. (A writeable save directory is required for caching data.)

:ref:`Release 6.12.0 <v6.12.0>`
