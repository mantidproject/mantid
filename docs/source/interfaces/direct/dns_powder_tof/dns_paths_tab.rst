.. _dns_paths_tab-ref:

DNS Paths Tab
-------------

.. image::  ../../../images/DNS_interface_overview.png
   :align: center
   :height: 400px
\

In this tab you can set the directories containing the datafiles for processing, as well as the directories where the output files will be located. The **Data Directory** field is mandatory and a user should browse to the location where data files are stored. If **Automatically Set Other Directories** checkbox is selected, the paths for PSD and Standard Files directories will be automatically set to default values. Moreover, the folders for the processed files and generated reduction scripts will be set to default values, as well. If a DNS data file is found in the **Data Directory** directory, the proposal number and username will be shown (if available).

If the GUI is called from commandline, the current working directory is automatically chosen as the data directory.

Main Controls Summary
^^^^^^^^^^^^^^^^^^^^^

+------------------------------+----------------------------------------------------------------------------------------------+
| **Data Directory**           | Path to DNS .d_dat files                                                                     |
+------------------------------+----------------------------------------------------------------------------------------------+
| **PSD Directory**            | Path to the position sensitive detector event list mode (.mdat files, not used at the moment)|
+------------------------------+----------------------------------------------------------------------------------------------+
| **Script Directory**         | Path to the directory in which python reduction scripts will be saved                        |
+------------------------------+----------------------------------------------------------------------------------------------+
| **Standard Files Directory** | Path to the DNS calibration files (normally Vanadium, NiCr and Empty)                        |
+------------------------------+----------------------------------------------------------------------------------------------+
| **Export Directory**         | Path to the directory in which the processed data are saved (in the chosen formats)          |
+------------------------------+----------------------------------------------------------------------------------------------+

In order to speed up the performance of data processing, the list of files that are contained in the **Data Directory** is cached to the file named *last_filelist.txt*. In order to clean up the cache, the **Clear Cached Filelist** button should be used.

Whenever a user operates regularly with the same set of input/output directories, it might be convenient to save the corresponding configuration of input/output folders to an .xml file. This can be done by clicking on the "File" → "Save As" buttons at the top left corner of the GUI. After the configuration have been saved, it can simply be loaded by chosing "File" → "Open" → "config_filename.xml". This approach might help the user to save some time. 

:ref:`DNS Powder TOF <dns_powder_tof-ref>`
