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

Improvements
############

- :ref:`CrossCorrelate <algm-CrossCorrelate>` has additional parameter to set the maximum d-space shift during cross correlation

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

Algorithms
----------

Improvements
############
- :ref:`LoadNexusMonitors <algm-LoadNexusMonitors-v2>` now utilizes the log filter provided by `LoadNexusLogs <algm-LoadNexusLogs>`

Bugfixes
########
- :ref:`LoadNexusLogs <algm-LoadNexusLogs>` now logs that are poorly formed create a warning message and the other logs are loaded. Previously it stopped loading logs at that point.

SliceViewer
-----------

Improvements
############

Bugfixes
########
- Fix cursor tracking from getting stuck and displaying incorrect signals when viewing MDHistogram workspaces in :ref:`sliceviewer`.

:ref:`Release 6.2.0 <v6.2.0>`
