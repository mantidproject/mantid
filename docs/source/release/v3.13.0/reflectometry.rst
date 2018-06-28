=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

ISIS Reflectometry Interface
----------------------------

New features
############

- Fully-automatic processing has been added to the interface. Click ``Autoprocess`` to process all of the runs for an investigation and to start polling for new runs. Whenever new runs are found, they will automatically be added to the table and processed.
- A new option has been added to the Settings tab to control whether partial bins should be included when summing in Q.

Improvements
############

Bug fixes
#########

Features Removed
################

* Added deprecation notice to ISIS Reflectometry (Old) due to be removed in March 2019.

Algorithms
----------

* Removed version 1 of ``ReflectometryReductionOne`` and ``ReflectometryReductionOneAuto``.
* Renamed algorithms ``PolarizationCorrection`` to ``PolarizationCorrectionFredrikze`` and ``PolarizationEfficiencyCor`` to ``PolarizationCorrectionWildes``.

New features
############

* Added algorithm ``PolarizationEfficiencyCor`` which calls ``PolarizationCorrectionFredrikze`` or ``PolarizationCorrectionWildes`` depending on chosen ``Method`` property.
* Added algorithms that help create a matrix workspace with polarization efficiencies ready to be used with ``PolarizationEfficiencyCor``

  - ``CreatePolarizationEfficiencies`` creates efficiencies from polynomial coefficients
  - ``JoinISISPolarizationEfficiencies`` joins individual efficiencies into one matrix workspace
  - ``LoadISISPolarizationEfficiencies`` loads efficiencies form files
* Algorithms for reflectometry reduction at ILL have been added. These handle the basic reduction in SumInLambda mode. Included algorithms:
    - :ref:`algm-ReflectometryILLPreprocess`
    - :ref:`algm-ReflectometryILLSumForeground`
    - :ref:`algm-ReflectometryILLPolarizationCor`
    - :ref:`algm-ReflectometryILLConvertToQ`
* A new algorithm :ref:`algm-ReflectometryMomentumTransfer` provides conversion to momentum transfer and :math:`Q_{z}` resolution calculation for relfectivity workspaces.
* A new algorithm :ref:`ReflectometrySumInQ <algm-ReflectometrySumInQ>` is available for coherent summation of the reflected beam.

- :ref:`algm-ReflectometryReductionOne` and :ref:`algm-ReflectometryReductionOneAuto` no longer include partial bins by default when summing in Q. A new property, `IncludePartialBins`, has been added to re-enable partial bins.

Improvements
############

Bug fixes
#########

* Correct the angle to the value of ``ThetaIn`` property if summing in lambda in ``ReflectometryReductionOne-v2``.

Liquids Reflectometer
---------------------
* New REF_L instrument geometry for 2018 run cycle.

Magnetism Reflectometer
-----------------------
* Added live data information to Facilities.xml
* Allow for the use of workspace groups as input to the reduction.
* Added algorithm to compute scattering angle from a workspace.

:ref:`Release 3.13.0 <v3.13.0>`
