============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New Features
------------
- New algorithm :ref:`algm-FlipperEfficiency` for the Polarised SANS workflow.
- New algorithm :ref:`algm-HeliumAnalyserEfficiency` for calculating the efficiency of a helium-3 analyser.
- New algorithm :ref:`algm-DepolarizedAnalyserTransmission`:

.. figure::  ../../images/6_10_release/depolarized-analyser-transmission-algo.png
   :width: 400px

Bugfixes
--------
- Rows in the table on the :ref:`ISIS_SANS_Runs_Tab-ref` no longer occasionally pick up an output name from a different row and then overwrite the data in that output workspace.
- Saving from the ``Save Other`` dialog in the :ref:`ISIS_SANS_Runs_Tab-ref` no longer throws an error.

:ref:`Release 6.10.0 <v6.10.0>`
