=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

General
-------

New features
############
- :ref:`SofQWCentre <algm-SofQWCentre>` and :ref:`SofQWPolygon <algm-SofQWPolygon>` have been deprecated.

Bugfixes
############
- :ref:`PyChop <PyChop>` definition files for SNS instruments ARCS and SEQUOIA hav been fixed.

  The correct files are now included so that PyChop calculations in Mantid
  agree with those from the `online SNS calculator <https://rez.mcvine.ornl.gov/>`_.
  Please note that flux units for SNS instruments are n/cm^2/s/MW, whereas for ISIS instruments
  they are n/cm^2/s at a nominal proton current of 160uA.


MSlice
------

New features
############
- MSlice is installed via Conda as part of Mantid workbench and not compiled as an external project.
- The documentation now includes an example script for saving multiple cut plots as ASCII files.
- Matplotlib is now at version 3.9.2.
- MSlice has a new observer for workspaces in the Analysis Data Service in Mantid. Consequently, modifications to workspaces in Mantid are now synchronised with MSlice for shared workspaces. For example, this change now ensures that a deleted workspace in Mantid is also deleted in MSlice, which mitigates a common past error of `RuntimeError: Variable invalidated, data has been deleted`. The behaviour of the MSlice interface is thereby made consistent with that of other interfaces.

Bugfixes
############
- Performing binary operations on vectors of different length no longer crashes Mantid. An error is displayed instead.


:ref:`Release 6.12.0 <v6.12.0>`
