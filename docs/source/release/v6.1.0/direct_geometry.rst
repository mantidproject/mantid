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

* Updated instrument geometry for CHESS
* Loading of SHARP TOF and single-channel data has been added to :ref:`LoadILLTOF <algm-LoadILLTOF-v2>`
- Loading of the omega-scan mode is supported for ILL instruments IN5, PANTHER, and SHARP via :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` loader


MSlice
------

BugFixes
########
- Fixed bug that overwrote selection of background workspace when subtracting workspaces.
- Fixed runtime error when trying to delete workspace for the second time.
- Fix for colorcycle problem for multiple curves on one plot.


:ref:`Release 6.1.0 <v6.1.0>`
