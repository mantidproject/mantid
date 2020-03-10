=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

New
###

* New ``NOW4`` instrument definition for SNS

Improvements
############

- For Panther and IN5 the :ref:`DirectILLReduction <algm-DirectILLReduction-v1>` will now correctly mask the non-overlapping bins before grouping the pixels onto Debye-Scherrer rings.

BugFixes
########

- Fixed a bug in the :ref:`Crystal Field Python Interface` which prevented users from defining a cubic crystal field model.
- Bug fixed affecting Direct > DGS Reduction interface - only facilities with applicable instruments can now be selected.

:ref:`Release 5.0.0 <v5.0.0>`
