============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New Features
------------
- (`#40443 <https://github.com/mantidproject/mantid/pull/40443>`_) Add option in TOML :ref:`UserFiles<sans_toml_v1-ref>` and :ref:`ISIS SANS GUI <ISIS_Sans_interface_contents>` to compute wide-angle corrections on transmission data.
- (`#40253 <https://github.com/mantidproject/mantid/pull/40253>`_) Add the option to calculate the efficiency for an analyzer flipper in :ref:`algm-FlipperEfficiency` and in :ref:`algm-SANSISISPolarizationCorrections`.
- (`#40585 <https://github.com/mantidproject/mantid/pull/40585>`_) New option for selecting a range of phi values in the masking tab of :ref:`ISIS SANS GUI <ISIS_Sans_interface_contents>` to generate multiple phi slices out of a single 1D reduction.
- (`#40585 <https://github.com/mantidproject/mantid/pull/40585>`_) Add option in TOML :ref:`UserFiles<sans_toml_v1-ref>` to select a range of phi values to generate multiple phi slices in a single 1D reduction.
- (`#40253 <https://github.com/mantidproject/mantid/pull/40253>`_) Disable line edits for not needed runs on either calibration or correction for the algorithm dialog of :ref:`algm-SANSISISPolarizationCorrections`.
- (`#40411 <https://github.com/mantidproject/mantid/pull/40411>`_) Setting a wrong value on the line edits of the settings tab in the :ref:`SANS User Interface<ISIS_Sans_interface_contents>` no longer creates a duplicate warning box when pressing enter.
- (`#40429 <https://github.com/mantidproject/mantid/pull/40429>`_) Added :ref:`GenerateFlatCellWorkspaceLOQ <algm-GenerateFlatCellWorkspaceLOQ>` for creating a flatcell workspace for ISIS SANS reduction of LOQ. The algorithm also generates an xml file with the detector ids to mask.

Bugfixes
--------
- (`#40353 <https://github.com/mantidproject/mantid/pull/40353>`_) The :ref:`ISIS_Sans_interface_contents` Interface now sends an appropriate error message when a directory is set as an user file path, the interface opens normally after the error is displayed.
- (`#40242 <https://github.com/mantidproject/mantid/pull/40242>`_) In the ISIS SANS workflow, the default number of cylindrical slices used in the calculation of the solid angle correction performed by the :ref:`Q1D <algm-Q1D>` and :ref:`Qxy <algm-Qxy>` algorithms has been changed from 10 to 11.
  This resolves a `bug <https://github.com/mantidproject/mantid/issues/36493>`__ where unexpected results were seen for certain detector orientations and geometries. The number of cylindrical slices used in the ISIS SANS workflow can be changed from the user file.

:ref:`Release 6.15.0 <v6.15.0>`
