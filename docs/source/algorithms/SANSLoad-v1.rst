.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm loads SANS data sets. The loading can handle nexus and raw files which can be plain or multi-period data. The SANS data sets which can be loaded with this algorithm are:

* sample scatter data which is the actual data under investigation. The algorithm loads the corresponding monitor workspace.
* sample transmission data
* sample direct data
* can scatter data. The algorithm also loads the corresponding monitor workspace.
* can transmission data
* can direct data

In addition a calibration file which is applied after the data has been loaded can be specified. This calibration workspace can be used when the *PublishToADS* option is enabled.


Relevant SANSState entries for SANSLoad
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The required information for the loading operation is retrieved from a SANSState input. It contains information
about the data files which are involved during the calibration process. It also contains information about the
calibration file which is applied to the scatter workspaces.

The elements of the SANSState are:

+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| Entry                          | Type                | Description                              | Mandatory                                    |
+================================+=====================+==========================================+==============================================+
| sample_scatter                 | String              | The name of the sample scatter file      | Yes                                          |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| sample_scatter_period          | Int                 | The selected period or (0 for all)       | No                                           |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| sample_transmission            | String              | The name of the sample transmission file | No, only if sample_direct was specified      |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| sample_transmission_period     | Int                 | The selected period or (0 for all)       | No                                           |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| sample_direct                  | String              | The name of the sample direct file       | No, only if sample_transmission was specified|
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| sample_direct_period           | Int                 | The selected period or (0 for all)       | No                                           |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| can_scatter                    | String              | The name of the can scatter file         | No, only if can_transmission was specified   |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| can_scatter_period             | Int                 | The selected period or (0 for all)       | No                                           |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| can_transmission               | String              | The name of the can transmission file    | No, only if can_direct was specified         |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| can_transmission_period        | Int                 | The selected period or (0 for all)       | No                                           |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| can_direct                     | String              | The name of the can direct file          | No, only if can_direct was specified         |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| can_direct_period              | Int                 | The selected period or (0 for all)       | No                                           |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| calibration                    | String              | The name of the calibration file         | No                                           |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| sample_scatter_run_number      | Int                 | The run number of the sample scatter     | auto setup                                   |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| sample_scatter_is_multi_period | Bool                | If the sample is multi-period            | auto setup                                   |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| instrument                     | SANSInstrument enum | The name of the calibration file         | auto setup                                   |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| idf_file_path                  | String              | The path to the IDF                      | auto setup                                   |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+
| ipf_file_path                  | String              | The path to the IPF                      | auto setup                                   |
+--------------------------------+---------------------+------------------------------------------+----------------------------------------------+


**Note that these settings should be only populated via the GUI or the Python Interface of ISIS SANS.**

Optimization Setting: *PublishToCache* and *UseCached*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The *PublishToCache* setting will store the calibration workspace on the *AnalysisDataService* when it has been loaded for the first time. The loaded workspaces themselves will not be published.

The *UseCached* setting will look for appropriate workspaces on the *AnalysisDataService* and use these workspaces instead of reloading them.

Move a workspace
~~~~~~~~~~~~~~~~

The algorithm perform an initial, instrument-specific move of the selected component. Currently this move mechanism is implemented for **SANS2D**, **LOQ**, **LARMOR**, and **ZOOM**. Other instruments will not be moved.
When moving a workspace a component and a beam position.

.. categories::

.. sourcelink::
