=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

Algorithms
----------

* New versions of algorithms :ref:`algm-ReflectometryReductionOne` and :ref:`algm-CreateTransmissionWorkspace`
have been added to remove duplicate steps in the reduction. An improvement in performance of factor x3 has been
observed when the reduction is performed with no monitor normalization.


Reflectometry Reduction Interface
---------------------------------

ISIS Reflectometry (Polref)
###########################

- Settings tab now displays individual global options for experiment and instrument settings.
- Documentation regarding the interface has been updated accordingly.
- New 'Save ASCII' tab added, similar in function and purpose to the 'Save Workspaces' window accessible from Interfaces->ISIS Reflectometry->File->Save Workspaces.

ISIS Reflectometry
##################

|

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Reflectometry%22>`__
