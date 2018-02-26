========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

New features
------------

* The `MSlice <https://github.com/mantidproject/mslice>`_ user interface can now be lauched from the MantidPlot Interfaces menu.

Algorithms
##########

Improvements
------------

Instrument definitions
######################

* The MAPS instrument definition file dating back to 2017-06-03 was changed.

Algorithms
##########

- Fixed a bug in :ref:`algm-DirectILLApplySelfShielding` which could cause confusion among workspaces when the algorithm was run without both self shielding correction and empty container workspaces.

Crystal Field
#############

Multi-site calculations and fitting are now supported by the crystal field (Python commandline) interface. 

Calculation of dipole transition matrix elements has been added, together with the addition of a :math:`\chi_0` term in the :ref:`CrystalFieldSusceptibility <func-CrystalFieldSusceptibility>` function. 

Several bugs in the Python and C++ code has been fixed - see the `github page <https://github.com/mantidproject/mantid/pull/21604>`_ for details.

Features Removed
----------------

* The Direct Convert To Energy graphical interface has been removed, it had not been used for several years, and was a source of bugs as well as using testing effort that is better directed elsewhere.

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.12%22+is%3Amerged+label%3A%22Component%3A+Direct+Inelastic%22>`_
