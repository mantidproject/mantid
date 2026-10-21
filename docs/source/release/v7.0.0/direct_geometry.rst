=======================
Direct Geometry Changes
=======================

General
-------

New features
############
- (`#41830 <https://github.com/mantidproject/mantid/pull/41830>`_) The :ref:`algm-SofQW` algorithm no longer has a ``Method`` property, since ``SofQWCentre`` and ``SofQWPolygon`` have been removed in this release. It now wraps :ref:`algm-SofQWNormalisedPolygon` directly.


MSlice
------

New features
############
- (`#1203 <https://github.com/mantidproject/mslice/pull/1203>`_, `#1208 <https://github.com/mantidproject/mslice/pull/1208/>`_) Updated MSlice for compatibility with Qt 6.
- (`#1232 <https://github.com/mantidproject/mslice/pull/1232>`_) Upgraded Python to version 3.13.

Bugfixes
############
- (`#1209 <https://github.com/mantidproject/mslice/pull/1209>`_) Fixed empty plots appearing when adding a grid to a Slice or Cut plot.
- (`#1241 <https://github.com/mantidproject/mslice/pull/1241>`_) Fixed inconsistencies when sharing the Jupyter console between MSlice and Mantid Workbench.
- (`#1236 <https://github.com/mantidproject/mslice/pull/1236>`_) Fixed an implicit conversion error in the legend checkbox and a crash when clearing multiple selected workspaces.
- (`#1236 <https://github.com/mantidproject/mslice/pull/1236>`_) Fixed the error reporter window reappearing after selecting 'Don't share my information'.

:ref:`Release 7.0.0 <v7.0.0>`
