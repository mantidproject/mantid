=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

General
-------

New features
############
- :ref:`LoadCIF <algm-LoadCIF>` now accepts the short forms of the Hermann-Mauguin symbols. These short forms may
  include a single space or none at all.

CrystalField
-------------

Bugfixes
############
- Removed unnecessary fix for ``IB21`` for ``C1`` and ``Ci`` symmetries in the Crystal Field peak base function to match
  the documentation in :ref:`Crystal Field Tables`.


MSlice
------

New features
############
- Update to Python 3.11.
- Changed the layout of the ``Compose`` button. Furthermore, the button is now always called ``Compose`` and does not
  change its name anymore when selecting an item from the dropdown menu.
- Invisible workspaces from :ref:`MSlice-ref` are now deleted when the workspace they originated from is deleted from
  the ADS (``Workspaces`` widget in Mantid Workbench).


Bugfixes
############
- Over-plotting a Cut plot with changed intensity and a Bragg peak no longer causes a crash.
- Changing intensity to ``GDOS`` on an interactive Cut plot no longer causes a crash.
- Attempting to save a file to a read-only folder will now display an error instead of causing a crash.
- Width calculation for Cut plots no longer causes a rounding error.
- Invalid characters in user input boxes such as ``Intensity``, ``Bose`` and ``Scale`` no longer cause a crash.


:ref:`Release 6.13.0 <v6.13.0>`
