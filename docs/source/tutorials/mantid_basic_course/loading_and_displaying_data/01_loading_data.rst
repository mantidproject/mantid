.. _01_loading_data:

============
Loading Data 
============

.. figure:: /images/ShowLoadandWorkspaceAreaInMantidWB.png
   :alt: Loading in Mantid Workbench

Loading a File
==============

First, let's load the dataset MAR11060.raw collected on the ISIS MARI
instrument:

#. In Workbench click on the the "Load" button (as shown highlighted in
   red), and select "File". This will open the Load Dialog window.
#. Browse to the location of the file MAR11060.raw.

   -  If you successfully added the "TrainingCourseData" directory 
      to the data search directories (see
      :ref:`getting started`) then you can simply
      enter the filename "MAR11060.raw" in the File textbox.
   -  Actually, typing "MAR11060" would be enough for Workbench to find
      the data.
   -  Furthermore, if you had set MARI as the default instrument in "File"
      > "Settings", then typing just "11060" would work!

#. Mantid will suggest the OutputWorkspace name to be "MAR11060", but
   feel free to use whatever you like.
#. Leave the other properties alone and press the Run button. A
   Workspace will appear in the Workspaces list (as shown highlighted in
   green).
#. Right-click on this workspace entry and select "Show Data". This will display the data in a matrix window
   named 'MAR11060 - Mantid', as shown below:

.. figure:: /images/ShowMatrixOfMar11060.png
   :alt: Show Data MAR11060.raw

Types of Data Files
===================

Mantid can load many different data formats. A few examples are:

-  ISIS, SNS, ILL, PSI .nexus data files.
-  ISIS .raw data and log files.
-  Simulated data formats.
-  Ascii data, Table data, etc.
-  Live data streams.

Fortunately, you don't have to learn how to use all of these Load
algorithms. In fact, you only need to know one : "Load". Whenever you use "Load" Mantid determines the format of the file and uses the correct algorithm to read it.

Loading Lots of Data Files
==========================

You can load multiple files into Mantid with a single Load command,
either keeping each workspace separate, or summing the data into a
single workspace:

+-----------+--------------------------------------------------------+---------------+
| Usage     | Description                                            | Example       |
+===========+========================================================+===============+
| Input     | Result                                                 |               |
+-----------+--------------------------------------------------------+---------------+
| \ ``,``\  | Load a list of runs into a Group of Workspaces         | ``INST1,2,3`` |
+-----------+--------------------------------------------------------+---------------+
| \ ``+``\  | Sum a list of runs into one Workspace.                 | ``INST1+2+3`` |
+-----------+--------------------------------------------------------+---------------+
| \ ``:``\  | Load a range of runs into a Group of Workspaces        | ``INST1:4``   |
+-----------+--------------------------------------------------------+---------------+
| \ ``-``\  | Sum a range of runs into one Workspace.                | ``INST1-4``   |
+-----------+--------------------------------------------------------+---------------+

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Introduction|Mantid_Basic_Course|MBC_The_Workspace_Matrix}}
