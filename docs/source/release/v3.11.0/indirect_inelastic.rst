==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

New features
------------

Algorithms
##########

Data Analysis
#############

Jump Fit
~~~~~~~~

Improvements
------------
- The *S(Q, W)* interface now automatically replaces NaN values with 0.


Bugfixes
--------

- An issue has been fixed in :ref:`algm-IndirectILLEnergyTransfer` when handling the data with mirror sense, that have shifted 0 monitor counts in the left and right wings. This was causing the left and right workspaces to have different x-axis binning and to fail to sum during the unmirroring step. 
- An issue has been fixed in :ref:`algm-IndirectILLReductionFWS` when the scaling of the data after vanadium calibration was not applied.

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
