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

New features
############

- Algorithms for reflectometry reduction at ILL have been added. These handle the basic reduction in SumInLambda mode. Included algorithms:
    - :ref:`algm-ReflectometryILLPreprocess`
    - :ref:`algm-ReflectometryILLSumForeground`
    - :ref:`algm-ReflectometryILLPolarizationCor`
    - :ref:`algm-ReflectometryILLConvertToQ`
- A new algorithm :ref:`algm-ReflectometryMomentumTransfer` provides conversion to momentum transfer and :math:`Q_{z}` resolution calculation for relfectivity workspaces.

Improvements
############

Bug fixes
#########

* Correct the angle to the value of ``ThetaIn`` property if summing in lambda in ``ReflectometryReductionOne-v2``.

Liquids Reflectometer
---------------------
* New REF_L instrument geometry for 2018 run cycle.

:ref:`Release 3.13.0 <v3.13.0>`
