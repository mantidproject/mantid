============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New Features
------------
- (`#40443 <https://github.com/mantidproject/mantid/pull/40443>`_) ``transmission.wide_angle_correction`` is a new
  option in the :ref:`sans_toml_v1-ref` and :ref:`ISIS_Sans_interface_contents` Interface, which allows users to compute
  wide-angle corrections on transmission data.
- (`#40253 <https://github.com/mantidproject/mantid/pull/40253>`_) :ref:`algm-FlipperEfficiency` and
  :ref:`algm-SANSISISPolarizationCorrections` have new options to calculate the efficiency of an analyzer flipper.
- (`#40585 <https://github.com/mantidproject/mantid/pull/40585>`_) The :ref:`ISIS_Sans_interface_contents` Interface
  has a new option for selecting a range of phi values in the masking tab to generate multiple phi slices from a
  single 1D reduction.
- (`#40831 <https://github.com/mantidproject/mantid/pull/40831>`_) The ``PhiRanges`` function, in the
  :ref:`ISIS Command Interface<ScriptingSANSReductions>`, uses the new TOML field for phi range in the reduction and
  accepts ``use_mirror`` as a keyword argument.
- (`#40429 <https://github.com/mantidproject/mantid/pull/40429>`_) :ref:`algm-GenerateFlatCellWorkspaceLOQ` has been
  added, for creating a ``flatcell`` workspace for the ISIS SANS reduction of LOQ. The algorithm also generates a ``.xml``
  file with the detector IDs to mask.


Bugfixes
--------
- (`#40353 <https://github.com/mantidproject/mantid/pull/40353>`_) The :ref:`ISIS_Sans_interface_contents` Interface now
  sends an appropriate error message when a directory is set as an user file path and the interface opens normally after
  the error is displayed.
- (`#40242 <https://github.com/mantidproject/mantid/pull/40242>`_) In the ISIS SANS workflow, the default number of
  cylindrical slices used in the calculation of the solid angle correction performed by the :ref:`algm-Q1D` and
  :ref:`algm-Qxy` algorithms has been changed from 10 to 11. This resolves a bug where unexpected results were
  seen for certain detector orientations and geometries. The number of cylindrical slices used in the ISIS SANS workflow
  can be changed via the :ref:`user file<sans_toml_v1-ref>`.
- (`#40253 <https://github.com/mantidproject/mantid/pull/40253>`_) The dialog of :ref:`algm-SANSISISPolarizationCorrections`
  no longer enables line edits for not-needed runs on either calibration or correction.
- (`#40411 <https://github.com/mantidproject/mantid/pull/40411>`_) The :ref:`ISIS_Sans_interface_contents` interface no
  longer creates a duplicate warning box when pressing enter after setting a wrong value on the line edits of the
  settings tab.

:ref:`Release 6.15.0 <v6.15.0>`
