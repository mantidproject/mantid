=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

MSlice
------

Bugfixes
############

- (`#1124 <https://github.com/mantidproject/mslice/pull/1124>`_)The temperature setter is now more robust for Slices and
  Cuts.
- (`#1131 <https://github.com/mantidproject/mslice/pull/1131>`_) :ref:`MSlice-ref` no longer crashes when saving an
  ``MDEventWorkspace2D`` as a MATLAB file.
- (`#1142 <https://github.com/mantidproject/mslice/pull/1142>`_) Plot windows are now closed when the corresponding
  workspace is deleted.
- (`#1140 <https://github.com/mantidproject/mslice/pull/1140>`_) :ref:`MSlice-ref` no longer crashes when saving a
  script from a Cut plot.
- (`#1155 <https://github.com/mantidproject/mslice/pull/1155>`_) Residual hidden workspaces are now removed from the
  ADS and can no longer cause a memory leak.
- (`#1158 <https://github.com/mantidproject/mslice/pull/1158>`_) The colorbar font sizes are now preserved when making
  changes to the colorbar.


:ref:`Release 6.15.0 <v6.15.0>`
