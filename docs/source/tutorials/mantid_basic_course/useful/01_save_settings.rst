.. _01_save_settings:

===================
Saving and Settings
===================

.. raw:: html

    <style> .red {color:#FF0000; font-weight:bold} </style>

.. role:: red


Saving
------

You have learnt how to fit data, which produces its results as output workspaces.
You have also learnt how to manipulate data with algorithms, and you will do more of this when analysing your data.

But it is important to be able to save these outputs!

.. figure:: /images/SaveButton.png
   :align: right
   :width: 150px
   :alt: SaveButton

You can use the Save drop-down menu in the Workspaces Toolbox to save a workspace to an ASCII or .nxs file.

These are equivalent to Executing the Algorithms :ref:`SaveAscii2 <algm-SaveAscii-v2>` and :ref:`SaveNexus <algm-SaveNexus-v1>`

You can also save your current session of Mantid as a Project. A Project is saved to a folder (worth creating a new one!) with a (.nxs) file for each workspace and a mantidproject file (ending in :red:`.mtdproj` for Workbench) that has information such as what workspaces were loaded and what plots you had open.

Opening a project will reload the Workspaces in the Workspaces Toolbox and reopen any plots or data tables. It will not, for instance, reopen Interfaces.

**Note**: MantidPlot projects will not open in Mantid Workbench and vice versa. You can simply reload all of the workspaces from their (.nxs) files (in the same folder as the project file), but plots will not be reproduced!


Settings
--------

Here is where you can personalise Mantid, just the way you like it!

Within the General tab you can set the default Facility/Instrument, control Notifications and Project Recovery and change the aesthetics of Mantid through its Font and Layout of toolboxes.

.. figure:: /images/SettingsGeneral.png
   :align: center
   :alt: SettingsGeneral

Use the Categories tab to de/select categories of Algorithms and Interfaces.

.. figure:: /images/SettingsCategories.png
   :align: center
   :alt: SettingsCategories

Set your basic preferences for plots, such as set Capsize > 0 if you want your error bars to have end caps!

.. figure:: /images/PlotSettings.png
   :align: center
   :alt: SettingsPlots


There are even a few Fitting options you can set, such as your Default peak / background function.

.. figure:: /images/SettingsFitting.png
   :align: center
   :alt: SettingsFitting
