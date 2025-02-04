=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

General
-------

New features
############
- Deprecated :ref:`SofQWCentre <algm-SofQWCentre>` and :ref:`SofQWPolygon <algm-SofQWPolygon>`.

Bugfixes
############
- Definition files for :ref:`PyChop <PyChop>` for SNS instruments ARCS and SEQUOIA fixed.

  This was due to the wrong files from the original `repository <https://github.com/sns-chops/resolution>`_
  being included into Mantid. The correct files are now include so that PyChop calculations in Mantid
  now agree with that from the `online SNS calculator <https://rez.mcvine.ornl.gov/>`_.
  Please note that the flux values for SNS instruments are in n/cm^2/s/MW whereas for ISIS instruments
  it is in n/cm^2/s at a nominal proton current of 160uA.


CrystalField
-------------

New features
############


Bugfixes
############



MSlice
------

New features
############
- MSlice is now installed via Conda as part of Mantid workbench and not compiled as an external project anymore.
- The documentation now includes an example script for saving multiple cut plots as ASCII files.
- Matplotlib is now at version 3.9.2.
- MSlice has a new observer for workspaces in the Analysis Data Service in Mantid. Consequently, modifications to workspaces in Mantid are now synchronised with MSlice for shared workspaces. For example, this change now ensures that a deleted workspace in Mantid is also deleted in MSlice, which mitigates a common past error of `RuntimeError: Variable invalidated, data has been deleted`. The behaviour of the MSlice interface is thereby made consistent with that of other interfaces.

Bugfixes
############
- Performing binary operations on vectors of different length no longer crashes Mantid. An error is displayed instead.


:ref:`Release 6.12.0 <v6.12.0>`
