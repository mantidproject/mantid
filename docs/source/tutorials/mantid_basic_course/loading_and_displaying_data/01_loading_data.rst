.. _01_loading_data:

============
Loading Data
============

.. figure:: /images/ShowLoadandWorkspaceAreaInMantidWB.png
    :width: 400px
    :alt: Loading in Mantid Workbench

Loading a File
==============

First, let's use the Workspaces Toolbox to load the dataset MAR11060.raw collected on the ISIS MARI
instrument:

#. In Workbench click on the "Load" button (as shown highlighted in
   red), and select "File". This will open the Load Dialog window.
#. Browse to the location of the file MAR11060.raw.

   -  If you successfully added the "TrainingCourseData" directory
      to the data search directories (see
      :ref:`getting_started`) then you can simply
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

Different data formats require different loader algorithms. Fortunately, you don't have to learn how to use all of these
algorithms, in fact, you only need to know one: ``Load``. Whenever you use ``Load``, Mantid determines the format of the file and uses the correct algorithm to read it.

Loading Lots of Data Files
==========================

You can load multiple files into Mantid at the same time,
either keeping each workspace separate, or summing the data into a
single workspace. To use the examples below, make sure to add ``TrainingCourseData`` and ``TrainingCourseData\loqdemo`` in
:ref:`ManageUserDirectories <getting_started>`. Next, click "Load" at the top of the Workspaces Toolbox and in the new Load Dialog
use these example inputs with symbols for the ``File`` input:

+-----------+--------------------------------------------------------+----------------------+
| Usage     | Description                                            | Example              |
+===========+========================================================+======================+
| Input     | Result                                                 |                      |
+-----------+--------------------------------------------------------+----------------------+
| \ ``,``\  | Load a list of runs into a Group of Workspaces         |  ``MAR11060,11015``  |
+-----------+--------------------------------------------------------+----------------------+
| \ ``+``\  | Sum a list of runs into one Workspace.                 |  ``LOQ74014+74020``  |
+-----------+--------------------------------------------------------+----------------------+
| \ ``:``\  | Load a range of runs into a Group of Workspaces        |  ``LOQ74019:74020``  |
+-----------+--------------------------------------------------------+----------------------+
| \ ``-``\  | Sum a range of runs into one Workspace.                | ``LOQ74019-74020`` * |
+-----------+--------------------------------------------------------+----------------------+

\* As these files have different numbers of spectra, you will see the error ``Left and right sides should contain the same amount of spectra....``
For this example please load ``LOQ74019-74020`` with the parameter ``SpectrumMax = 8``.
