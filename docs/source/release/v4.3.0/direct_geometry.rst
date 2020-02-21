=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New
###

* New ``NOW4`` instrument definition for SNS

Improvements
############

- For Panther and IN5 the :ref:`DirectILLReduction <algm-DirectILLReduction-v1>` will now correctly mask the non-overlapping bins before grouping the pixels onto Debye-Scherrer rings.

BugFixes
########

- Fixed a bug in the :ref:`Crystal Field Python Interface` which prevented users from defining a cubic crystal field model.

:ref:`Release 4.3.0 <v4.3.0>`
