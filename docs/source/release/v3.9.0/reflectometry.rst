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
- Fixed a bug where if the user answered 'no' to a popup asking if they wanted to process all runs, the progress bar would show activity as though a data reduction was occurring.
- The interface is now arranged in two different groups. Groups apply to tabs 'Run' and 'Settings'.
- Documentation regarding the interface has been updated accordingly.
- Error messages are displayed if the user either attempts to transfer zero runs or transfer runs with a different strategy to the one they used to search for runs with. 

ISIS Reflectometry
##################

|

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Reflectometry%22>`__
