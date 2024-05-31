============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New Features
------------
- A :ref:`algm-FlipperEfficiency` algorithm has been created as part of the Polarised SANS workflow.
- Added algorithm for calculating the efficiency of a helium-3 analyser.
- Algorithm :ref:`algm-DepolarizedAnalyserTransmission` has been created.

Bugfixes
--------
- Fixed a bug where rows in the table on the :ref:`ISIS_SANS_Runs_Tab-ref` would sometimes pick up an output name from a different row and then overwrite the data in that output workspace.
- An error is no longer produced when saving from the ``Save Other`` dialog in the :ref:`ISIS_SANS_Runs_Tab-ref`.

:ref:`Release 6.10.0 <v6.10.0>`