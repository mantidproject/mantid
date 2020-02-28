=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

ISIS Reflectometry Interface
############################

**The old ISIS Reflectometry Interface has been removed. If required then please use the previous release, version 4.2.0**

Improved
--------

- The per-angle defaults table on the Experiment now has column-specific tooltips on the table cells which correspond to the [ReflectometryISISLoadAndProcess](https://docs.mantidproject.org/nightly/algorithms/ReflectometryISISLoadAndProcess-v1.html?highlight=reflectometryisisloadandprocess) documentation

Algorithms
##########

New
---

- The new workflow algorithm :ref:`algm-ReflectometryILLAutoProcess` performs the complete data reduction for ILL reflectometers D17 and Figaro. Implements both coherent and incoherent summation types.

- New output properties have been added for the ISIS Reflectometry algorithms for transmission workspaces so that history is now attached to these outputs.

  - The affected algorithms are: :ref:`algm-CreateTransmissionWorkspace`, :ref:`algm-ReflectometryReductionOne`, :ref:`algm-ReflectometryReductionOneAuto`, :ref:`algm-ReflectometryISISLoadAndProcess`.

  - **Note that this may break existing scripts if you assign outputs directly to a python list**

    e.g. if previously you called an algorithm as:
    
    ``IvsQ, IvsQ_unbinned = ReflectometryReductionOneAuto(InputWorkspace=run, FirstTransmissionRun=trans, ThetaIn=0.7)``
    
    then this will now need to be as follows (note that the optional ``IvsLam`` also needs to be added here because it is declared before the transmission output and the list must always be in the same order):
    
    ``IvsQ, IvsQ_unbinned, IvsLam, transLam = ReflectometryReductionOneAuto(InputWorkspace=run, FirstTransmissionRun=trans, ThetaIn=0.7)``

    If your scripts use the output property instead then they will not be affected, e.g. calls like this will still work as before:
    
    ``ReflectometryReductionOneAuto(InputWorkspace=run, FirstTransmissionRun=trans, ThetaIn=0.7, OutputWorkspaceBinned='ivsq_bin')``

  
Bug fixes
---------

- The detector is now correctly position for fractional line position inputs for :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>`.
- The `TwoTheta` input option works now correctly for :ref:`ReflectometryILLPreProcess <algm-ReflectometryILLPreProcess>`


:ref:`Release 5.0.0 <v5.0.0>`
