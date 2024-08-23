.. _Muon_Feature_Flags-ref:

Muon Feature Flags
==================

.. contents:: Table of Contents
  :local:

Overview
--------

Muon Feature Flags provide advanced users with access to additional features such as raw plots and the model fitting tab.


How to set up
--------------

To access feature flags you need to make changes to your ``Mantid.user.properties file``. To do this, follow these instructions:

1. Navigate to the location of the ``Mantid.user.properties`` file

   a. For Windows users - Open the file explorer and navigate to the ``MantidInstall\bin`` folder
   b. For Mac or Linux users - go to ``~/.mantid/Mantid.user.properties``

2. Open the ``Mantid.user.properties`` file.
3. At the bottom of the file add the line ``muon.GUI=<options>``. See section on :ref:`Options <feature_flag_options>` below for more details.
4. Save the file.
5. Open Mantid - You should now have access to the features you chose in step 4.

If this does not work see the :ref:`Warning <feature_flag_warning>` notice below for common ways this can fail.

.. _feature_flag_options:

Options
-------

The options are a comma separated list of ``feature:setting`` where the feature and settings are defined below:

+-------------------+----------------+-----------------------------------------------------------------+
| Feature           | Setting        | What it does                                                    |
+===================+================+=================================================================+
| model_analysis    | 1              | Adds the model fitting tab                                      |
+-------------------+----------------+-----------------------------------------------------------------+
| model_analysis    | 2              | Adds the model fitting tab and                                  |
|                   |                | the model data plot pane                                        |
+-------------------+----------------+-----------------------------------------------------------------+
| raw_plots         | 1              | Adds the raw plot pane                                          |
+-------------------+----------------+-----------------------------------------------------------------+
| Fit_wizard        | 1              | Adds the :ref:`Fit Script Generator <Fit_Script_Generator-ref>` |
+-------------------+----------------+-----------------------------------------------------------------+

The setting is always a number and, setting it to any other value than those above will results in the feature not being added.

One example of how to use this - if you want to have the model fitting tab and plotting pane in addition to the raw plots
you would need to add the following line to ``Mantid.user.properties`` file:

``muon.GUI = model_analysis:2, raw_plots:1``

.. _feature_flag_warning:

Warning
-------
Setting any of the following for the first time may remove the ``muon.GUI=<options>`` line from the properties file.

- Default facility
- Default instrument
- Manage user directories

To avoid this :

- Open Mantid and set the default settings (i.e. the list above)
- Close Mantid
- Edit the ``Mantid.user.properties file``

Updating the facility, instrument or user directories will not remove the ``muon.GUI=<options>`` line. It only seems to occur when setting these options for the first time.

.. categories:: Interfaces Muon
