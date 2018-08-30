.. _01_loading_data:

============
Loading Data 
============

.. figure:: /images/ShowLoadandWorkspaceAreaInMantidPlot.png
   :alt: ShowLoadandWorkspaceAreaInMantidPlot.png

Loading a File
==============

First, let's load the dataset MAR11060.raw collected on the ISIS MARI
instrument:

#. In MantidPlot click on the the "Load" button (as shown highlighted in
   red), and select "File". This will open the Load Dialog window.
#. Browse to the location of the file MAR11060.raw.

   -  If you have successfully added the directory containing this file
      to your data search directories (see
      :ref:`getting started`) then you can simply
      enter the filename in the File textbox.
   -  Typing "MAR11060" would actually be enough for MantidPlot to find
      the data, but if you have set MARI as you default instrument
      typing just "11060" would work!

#. Mantid will suggest the OutputWorkspace name to be "MAR11060", but
   feel free to use whatever you like.
#. Leave the other properties alone and press the Run button. A
   Workspace will appear in the Workspaces list (as shown highlighted in
   green).
#. Click on this workspace entry and drag it into the main area to the
   left of the workspace. This will display the data in a matrix window
   named 'MAR11060 - Mantid', as shown below:

.. figure:: /images/ShowMatrixOfMar11060.png
   :alt: ShowMatrixOfMar11060.png

Other equivalent methods exist for loading files in MantidPlot:

-  Navigating to the "File->Load->File" menu item.
-  Clicking the Load File Button toolbar button.
-  Using the Load Algorithm. (More on this later!)

.. figure:: /images/300px-LoadAlgorithmsSep2013.png
   :alt: The list of Load algorithms for Mantid v2.6.
   :width: 300px

Types of Data Files
===================

Mantid can load many different data formats. A few examples are:

-  ISIS, SNS, ILL, PSI .nexus data files.
-  ISIS .raw data and log files.
-  Simulated data formats.
-  Ascii data, Table data, etc.
-  Live data streams.

Fortunately, you don't have to learn how to use all of these Load
algorithms. In fact, just one, "Load", which was used earlier when you
loaded the workspace. Whenever you use "Load" Mantid takes care of the
following:

-  Expanding out run numbers to full file names.
-  Finding the file in the data search directories, and optionally the
   facility archive.
-  Determining the format of the file and using the correct algorithm to
   read it.
-  Loading or summing multiple files.

Loading Lots of Data Files
==========================

You can load multiple files into mantid with a single Load command,
either keeping each workspace separate, or summing the data into a
single workspace:

+-----------+------------------------------+---------------+
| Usage     | Description                  | Example       |
+===========+==============================+===============+
| Input     | Result                       |               |
+-----------+------------------------------+---------------+
| \ ``,``\  | Load a list of runs.         | ``INST1,2,3`` |
+-----------+------------------------------+---------------+
| \ ``+``\  | Sum a list of runs together. | ``INST1+2+3`` |
+-----------+------------------------------+---------------+
| \ ``:``\  | Load a range of runs.        | ``INST1:4``   |
+-----------+------------------------------+---------------+
| \ ``-``\  | Sum a range of runs.         | ``INST1-4``   |
+-----------+------------------------------+---------------+

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Introduction|Mantid_Basic_Course|MBC_The_Workspace_Matrix}}
