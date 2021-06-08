=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

Algorithms
----------
Bugfixes
############
GetEiV2 algorithm has been rarely failing on noisy functions with strange error message "Workspace contains Inf or NaN" despite no Inf or NaN-s were actually present in the data. The actual reason for the failure was the denominator of some function, which measures the half-width of the peak may become zero. The solution fixes the issue with zero values in denominator, but still returns unreliable result for noisy function.

Improvements
############

- :ref:`CreateSampleWorkspace <algm-CreateSampleWorkspace>` has new property InstrumentName.

Fit Functions
-------------
- new method `IPeakFunction::intensityError` calculates the error in the integrated intensity of the peak due to uncertainties in the values of the fit parameters.

Data Objects
------------

Python
------


.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Installation
------------


MantidWorkbench
---------------

See :doc:`mantidworkbench`.

SliceViewer
-----------

Improvements
############

Bugfixes
########

- Added parser for input Names to :ref:`algm-CreateMDHistoWorkspace` to allow inputs such as `Names='[H,0,0],[0,K,0],[0,0,L]'`.

:ref:`Release 6.2.0 <v6.2.0>`
