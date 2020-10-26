=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Improvements
------------

- The instrument geometry of PANTHER has been corrected according to the findings during the hot commissioning.
- New IDF has been added for the SHARP time-of-flight spectrometer at the ILL.

CrystalField
------------

New
###
- Extended the ``Background`` class to accept a list of functions through using the ``functions`` keyword. This
  allows more than two functions to be used for the ``Background`` of a CrystalField fit.

BugFixes
########
- Fixed a bug in the :ref:`Crystal Field Python Interface` where ties were not being applied properly for cubic crystal structures.


DGSPlanner
----------

Improvements
############

- Widgets were rearranged into groups of items based on their logical function

:ref:`Release 6.0.0 <v6.0.0>`
