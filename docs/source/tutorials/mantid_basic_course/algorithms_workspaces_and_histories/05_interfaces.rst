.. _05_interfaces:

==========
Interfaces
==========

Some analysis workflows are too complex, need more
user input or need a better way to present the results than a single
algorithm interface can provide. For this reason Mantid Workbench has specific interfaces
for specific data analysis. Several interfaces have now been developed to handle different
aspects of the reduction and analysis workflow for data from various
scientific techniques.


An Example - The ISIS SANS Interface
========================================

This interface is able to run an appropriate data reduction with multiple user options
that can be altered as required. As with all our interfaces, it is able to speed up the process of analysing your data.

.. figure:: /images/ISISSANSInterface.PNG
   :alt: center

A simple walkthrough
--------------------

#. Start the interface with the Interfaces > SANS > "ISIS SANS"
#. On the **Run** tab, in the top right click *Manage Directories* and browse to the ``TrainingCourseData\loqdemo`` folder, then click OK
#. Click on the *Load User File* button. In the new file explorer, instead of filtering for ``TOML`` files, change this to ``Text`` files.
   Now browse to the ``TrainingCourseData\loqdemo`` folder and open "MaskFile.txt".
#. Similarly, Load the batch file called "batch_mode_reduction.csv"
#. Some pre-set values will have populated the Run table and the **Settings** tab
#. Back on the **Run** tab, notice that the Run table is editable and in the lower-right corner, there are several
   options for Reducing and Saving data. Leave these as default for now.
#. In the top-right, click *Process All*, and notice in the main Mantid window, Workspaces will appear in the Workspaces Toolbox.
#. Plot the *second_time_main_1D_2.2_10.0* Workspace to produce the plot seen below.

.. figure:: /images/ISISSANSInterfaceplot.PNG
   :align: center
   :width: 600px
