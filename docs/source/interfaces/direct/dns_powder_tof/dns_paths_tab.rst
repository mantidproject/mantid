.. _dns_paths_tab-ref:

DNS Path Tab
------------

.. image::  ../../../images/DNS_interface_overview.png
   :align: center
   :height: 400px
\

In this tab you can set the directories containing the datafiles which should be processed. If you set the **Data Directory** filed all other fields will be automatically set to default values. If a DNS data file is found in the directory, the proposal number and
username will be shown.

If the GUI is called from commandline, the current working directory is automatically chosen as data directory.

Main Controls
^^^^^^^^^^^^^

+------------------------------+----------------------------------------------------------------------------------------------+
| **Data Directory**           | Path to the DNS .d_dat files                                                                 |
+------------------------------+----------------------------------------------------------------------------------------------+
| **PSD Directory**            | Path to the position sensitive detector event list mode .mdat files (not used at the moment) |
+------------------------------+----------------------------------------------------------------------------------------------+
| **Logbook Directory**        | Path to the Nicos log files (not used at the moment)                                         |
+------------------------------+----------------------------------------------------------------------------------------------+
| **Script Directory**         | Path to the directory in which reduction scripts will be saved                               |
+------------------------------+----------------------------------------------------------------------------------------------+
| **Standard Files Directory** | Path to the DNS calibration files (normally Vanadium, NiCr and Empty)                        |
+------------------------------+----------------------------------------------------------------------------------------------+
| **Export Directory**         | Path to the directory in which the processed data are saved (in the chosen formats)          |
+------------------------------+----------------------------------------------------------------------------------------------+

The button **Clear Directories** clears all the fields, this shoud be done if the **Data Directory** is changed and new automatic setting of the other directories is wanted.

The **Clear Cached Filelist** button deletes the saved filelist of the **Data Tab**, which is cached for speed reasons in the data directory in the file *last_filelist.txt*

:ref:`DNS Powder TOF <dns_powder_tof-ref>`
