=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

ISIS Reflectometry Interface
----------------------------

New
###

- Fully-automatic processing has been added to the interface. Click ``Autoprocess`` to process all of the runs for an investigation and to start polling for new runs. Whenever new runs are found, they will automatically be added to the table and processed.
- A new option has been added to the Settings tab to control whether partial bins should be included when summing in Q.
- ``ReflectometryReductionOneAuto`` takes the polarization correction properties from the instrument parameter file when ``PolarizationAnalysis`` is set to ``ParameterFile``. The instrument parameter file can store the efficiencies as vectors of doubles.

Features Removed
################

* Added a deprecation notice to the ISIS Reflectometry (Old) interface, this is due to be removed in March 2019. It will be present in releases up until that date, but new features will only be added to the new interface.

Algorithms
----------

- Removed version 1 of ``ReflectometryReductionOne`` and ``ReflectometryReductionOneAuto``.
- Renamed algorithms ``PolarizationCorrection`` to ``PolarizationCorrectionFredrikze`` and ``PolarizationEfficiencyCor`` to ``PolarizationCorrectionWildes``.

New
###

- Added algorithm ``PolarizationEfficiencyCor`` which calls ``PolarizationCorrectionFredrikze`` or ``PolarizationCorrectionWildes`` depending on chosen ``Method`` property.
- Added algorithms that help create a matrix workspace with polarization efficiencies ready to be used with ``PolarizationEfficiencyCor``

  - ``CreatePolarizationEfficiencies`` creates efficiencies from polynomial coefficients
  - ``JoinISISPolarizationEfficiencies`` joins individual efficiencies into one matrix workspace
  - ``LoadISISPolarizationEfficiencies`` loads efficiencies form files
- The ILL reflectometry loader :ref:`algm-LoadILLReflectometry` implements the NeXus file changes of January 2018 and can load again all valid Nexus files for D17 and FIGARO which are available since 2013 and 2017, respectively.
- Algorithms for reflectometry reduction at ILL have been added. These handle the basic polarized/unpolarized reduction in SumInLambda or SumInQ modes. Included algorithms:

  - :ref:`algm-ReflectometryILLPreprocess`
  - :ref:`algm-ReflectometryILLSumForeground`
  - :ref:`algm-ReflectometryILLPolarizationCor`
  - :ref:`algm-ReflectometryILLConvertToQ`
- A new algorithm :ref:`algm-ReflectometryMomentumTransfer` provides conversion to momentum transfer and :math:`Q_{z}` resolution calculation for reflectivity workspaces.
- A new algorithm :ref:`ReflectometrySumInQ <algm-ReflectometrySumInQ>` is available for coherent summation of the reflected beam.

- :ref:`algm-ReflectometryReductionOne` and :ref:`algm-ReflectometryReductionOneAuto` no longer include partial bins by default when summing in Q. A new property, `IncludePartialBins`, has been added to re-enable partial bins.

- Added a boolean property ``Debug`` to the reflectometry algorithms that controls output of additional and/or intermediate workspaces.

Bug fixes
#########

- Correct the angle to the value of ``ThetaIn`` property if summing in lambda in ``ReflectometryReductionOne-v2``.
- Fixed an incorrectly calculated detector angle when loading FIGARO files using :ref:`algm-LoadILLReflectometry`.

Liquids Reflectometer
---------------------
- New REF_L instrument geometry for 2018 run cycle.

Magnetism Reflectometer
-----------------------
- Added live data information to Facilities.xml
- Allow for the use of workspace groups as input to the reduction.
- Added algorithm to compute scattering angle from a workspace.

:ref:`Release 3.13.0 <v3.13.0>`
