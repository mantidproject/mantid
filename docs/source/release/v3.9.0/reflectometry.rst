=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

Algorithms
----------

- :ref:`algm-Stitch1D` documentation has been improved, it now includes a workflow diagram illustrating the different steps in the calculation and a note about how errors are propagated.


Reflectometry Reduction Interface
---------------------------------

ISIS Reflectometry (Polref)
###########################

- Settings tab now displays individual global options for experiment and instrument settings.
- New 'Save ASCII' tab added, similar in function and purpose to the 'Save Workspaces' window accessible from Interfaces->ISIS Reflectometry->File->Save Workspaces.
- When runs are transferred to the processing table groups are now labeled according to run title.
- Column :literal:`dQ/Q` is used as the rebin parameter to stitch workspaces.
- The interface is now arranged in two different groups.
- Documentation regarding the interface has been updated accordingly.

ISIS Reflectometry
##################

|

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Reflectometry%22>`__
