=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

ISIS Reflectometry Interface
############################

Removed
-------

Please note that the old ISIS Reflectometry Interface has been removed from MantidPlot. We highly recommend that you use the new interface, which is available in both MantidWorkbench and MantidPlot. However, if the old interface is required in the short term, you can still use it in the previous release, version 4.2.0.

Improved
--------

- The per-angle defaults table on the Experiment now has column-specific tooltips. These correspond to the documentation for the relevant properties in the underlying workflow algorithm, :ref:`algm-ReflectometryISISLoadAndProcess`.

Algorithms
##########

New
---

- The new workflow algorithm :ref:`algm-ReflectometryILLAutoProcess` performs the complete data reduction for ILL reflectometers D17 and Figaro. Implements both coherent and incoherent summation types.

- New output properties have been added for the ISIS Reflectometry algorithms for transmission workspaces so that history is now attached to these outputs.

  - The affected algorithms are: :ref:`algm-CreateTransmissionWorkspace`, :ref:`algm-ReflectometryReductionOne`, :ref:`algm-ReflectometryReductionOneAuto`, :ref:`algm-ReflectometryISISLoadAndProcess`.

  - **Note that this may break existing scripts if you assign outputs directly to a python list:**

    e.g. if previously you called an algorithm as:

    ``qbin, q = ReflectometryReductionOneAuto(InputWorkspace=run, FirstTransmissionRun=trans, ThetaIn=0.7)``

    then this will now need to be as follows (note that the optional ``IvsLam`` also needs to be added here because it is declared before the transmission output and the list must always be in the same order):

    ``qbin, q, lam, trans = ReflectometryReductionOneAuto(InputWorkspace=run, FirstTransmissionRun=trans, ThetaIn=0.7)``

    If your scripts use the output property instead then they will not be affected, e.g. calls like this will still work as before:

    ``ReflectometryReductionOneAuto(InputWorkspace=run, FirstTransmissionRun=trans, ThetaIn=0.7, OutputWorkspaceBinned='qbin')``


Bug fixes
---------

- The detector is now correctly position for fractional line position inputs for :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>`.
- The `TwoTheta` input option works now correctly for :ref:`ReflectometryILLPreProcess <algm-ReflectometryILLPreProcess>`


:ref:`Release 5.0.0 <v5.0.0>`
