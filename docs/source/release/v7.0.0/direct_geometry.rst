=======================
Direct Geometry Changes
=======================

General
-------

New features
############
- (`#41830 <https://github.com/mantidproject/mantid/pull/41830>`_) Since ``SofQWCentre`` and ``SofQWPolygon`` have been removed this release, :ref:`algm-SofQW` no longer has a ``Method`` property. It now simply wraps :ref:`algm-SofQWNormalisedPolygon`.

Bugfixes
############


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
- (`#1203 <https://github.com/mantidproject/mslice/pull/1203>`_) MSlice is now compatible with Qt6.
- (`#1232 <https://github.com/mantidproject/mslice/pull/1232>`_) MSlice now uses python 3.13.

Bugfixes
############
- (`#1209 <https://github.com/mantidproject/mslice/pull/1209>`_) Adding gridlines to a Slice or Cut plot no longer causes empty plots to appear.
- (`#1241 <https://github.com/mantidproject/mslice/pull/1241>`_) Interactions with the Jupyter console are now consistent between both MSlice and Mantid Workbench.
- (`#1236 <https://github.com/mantidproject/mslice/pull/1236>`_) The legend checkbox no longer throws an error due to implicit conversions.
- (`#1236 <https://github.com/mantidproject/mslice/pull/1236>`_) The :ref:`Error Report <UserSupport>` window should no longer re-appear after selecting "Don't share any information".

:ref:`Release 7.0.0 <v7.0.0>`
