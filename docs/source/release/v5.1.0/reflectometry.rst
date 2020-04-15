=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

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

Bug fixes
---------

- Save/Load settings: A bug has been fixed where Experiment/Instrument settings were not being restored if the instrument changes on load.
- Lost settings on New Batch and Restore Defaults:

  - A bug has been fixed where creating a new Batch would result in the Experiment/Instrument settings of all batches being reset to their defaults.
  - A bug has been fixed where clicking Restore Defaults on an Experiment/Instrument tab would cause all Experiment and Instrument tabs in every batch to be reset to defaults. Now, only the tab where you click Restore Defaults is changed.

:ref:`Release 5.1.0 <v5.1.0>`
