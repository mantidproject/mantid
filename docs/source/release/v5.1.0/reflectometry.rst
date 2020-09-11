=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Algorithms
##########

New
----

- Added new :ref:`algm-LRReductionWithReference` to use a measured standard with a model reflectivity curve calculated by the `refl1d` package to produce a normalization curve for a sample reduction. Requires `refl1d` to be installed seperately.

Improvements
------------

- The ensemble of ILL reflectometry reduction is improved to produce results more consistent with the legacy software for D17 instrument.
- Sample waviness term is removed from resolution calculation in incoherent mode in :ref:`ReflectometryMomentumTransfer <algm-ReflectometryMomentumTransfer>`.
- Flag to enable / disable apply scaling factor from `ScalingFactorFile`, called `ApplyScalingFactor`, added to :ref:`algm-LiquidsReflectometryReduction`.
- Modified :ref:`algm-LRAutoReduction` to allow the option to autoreduce data with a reference measurement for normalization (instead of only direct beam) using the new :ref:`algm-LRReductionWithReference` algorithm of this release
- :ref:`algm-SaveReflectometryAscii`: leading separator is no longer included for Custom and ANSTO formats; Custom format header now uses the same separator as the columns and includes the 4th column header if applicable.
  
Bug fixes
---------

- :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>` has been fixed to update the sample logs of chopper gap and chopper position with correct units regardless the wrong setting in nexus files.
- The history for :ref:`algm-ReflectometryReductionOne` has been fixed so that the conversion to Q is now always included in the history.
- The sort function in `LRDirectBeamSort <algm-LRDirectBeamSort>` has been updated for Python3

Removed
-------

- The following deprecated algorithms have been removed: ``SaveANSTOAscii``, ``SaveILLCosmosAscii``, ``SaveReflCustomAscii``, and ``SaveReflThreeColumnAscii``. They have been replaced by :ref:`algm-SaveReflectometryAscii`.
  
ISIS Reflectometry Interface
############################

New
---

- **Background subtraction** options have been added to the ISIS Reflectometry Interface. The subtraction is performed using :ref:`algm-ReflectometryBackgroundSubtraction`.

.. figure:: ../../images/ISISReflectometryInterface/background_subtraction.png
  :class: screenshot
  :width: 500px
  :align: right
  :alt: Background subtraction on the ISIS Reflectometry Interface

  *Background subtraction on the ISIS Reflectometry Interface*

- **More reliable search** has been added - specify the cycle name in the search inputs to use journal file search instead of ICat. This is currently recommended due to ICat instability. However, ICat can still be used if the cycle number is not known/provided.

.. figure:: ../../images/Reflectometry-GUI-release5.1-search.png
  :class: screenshot
  :width: 200px
  :align: right
  :alt: Specify the cycle name in the search interface to get more reliable search results

  *Provide cycle name for more reliable search results*

- The output formats from the **Save ASCII** tab now follow standards more rigidly - see :ref:`algm-SaveReflectometryAscii`, which replaces the old deprecated algorithms.

- The **Options** dialog can now be accessed from the Tools menu, controlling the display of warnings and rounding precision.

Bug fixes
---------

- Save/Load settings: A bug has been fixed where Experiment/Instrument settings were not being restored if the instrument changes on load.
- Lost settings on New Batch and Restore Defaults:

  - A bug has been fixed where creating a new Batch would result in the Experiment/Instrument settings of all batches being reset to their defaults.
  - A bug has been fixed where clicking Restore Defaults on an Experiment/Instrument tab would cause all Experiment and Instrument tabs in every batch to be reset to defaults. Now, only the tab where you click Restore Defaults is changed.

:ref:`Release 5.1.0 <v5.1.0>`
