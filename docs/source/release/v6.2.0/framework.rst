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
- All remote algorithms have been deprecated as they have not been used since v3.8.

Improvements
############

- :ref:`CreateSampleWorkspace <algm-CreateSampleWorkspace>` has new property InstrumentName.
- Event nexuses produced at ILL can now be loaded using :ref:`LoadEventNexus <algm-LoadEventNexus>`.

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
