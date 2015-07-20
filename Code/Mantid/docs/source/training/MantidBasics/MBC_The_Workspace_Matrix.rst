.. _train-MBC_The_Workspace_Matrix:

.. image:: ../../images/ShowMatrixOfMar11060.PNG
			:align: right


The Workspace Toolbox
=====================

MantidPlot needs a way of storing the data it loads into something.
These 'somethings' are called :ref:`workspaces <Workspace>`. You can see all of the
Workspaces that Mantid has loaded in the Workspaces toolbox, together
with buttons for:

-  Loading new workspaces.
-  Deleting workspaces.
-  Grouping workspaces. If a grouped workspace is already selected this
   will change to "Ungroup" the workspaces.
-  Sorting the workspace list.
-  Saving workspaces.

Next to each :ref:`workspace <Workspace>` is a little triangle you can click to review a
few more details about the workspace.

The Workspace Matrix View
=========================

The dialog with the blue background is the workspace matrix view, and
can be used to look at histogram or event data. You can create one in a
few ways:

-  Click and drag the workspace name into the main window area (the grey
   bit in the middle).
-  Right click the workspace name and select show data. The next dialog
   gives you options to limit the amount of data displayed; just click
   OK.
-  Double click the workspace name
-  Using python commands. There are more details in the Python in Mantid
   course.

.. image:: ../../images/WorkspaceMatrixAnnotated.png


Each row in the matrix shown shows data values of a single spectra. By
flipping between the tabs you can see the X, Y and E values.

-  X contains the Bin boundaries or bin centre values of the X axis. If
   they are bin Boundaries you will have one more column in the X tab
   than you do in the Y or E tabs.
-  Y contains the counts found in each bin of the spectrum.
-  E contains the errors associated with the counts. For most raw data
   this will initially be the square root of Y on loading.

The number at the left of each row we call the "Workspace Index", or WI
for short, and is simply the row number as data is read into the
workspace; in the same way as a spreadsheet program like Excel uses row
numbers. This always starts from zero and is important as it is used
quite a bit in displaying and running Algorithms on your data.

Linking :ref:`Workspace <Workspace>` Index to Spectra Number
=========================================

Many instruments use a unique number to refer to each of the spectra in
a data file. We call this the "Spectrum Number". If you load the whole
file you will often find that the :ref:`Workspace <Workspace>` Index and Spectrum Number
match each other closely, just out by one. However if you only load part
of a file, for example spectra 100 - 200, then the :ref:`Workspace <Workspace>` Indices
will still be 0-100, but Mantid will remember the original Spectra
Number of each spectrum.

Linking :ref:`Workspace <Workspace>` Index to Detector IDs
=======================================

The data in each spectrum can come from one or more detectors, and
sometimes it is useful to know exactly which detectors. You can get
spectra linked to multiple detectors either by hardware detector
grouping or by grouping the detectors in Mantid.

There are two methods you can use to see the detector table of a
workspace within MantidPlot:

-  Right-click on the workspace name and select "Show Detectors".
-  Right-click within the workspace matrix and select "View Detectors
   Table".

Either of these methods will display a table containing the :ref:`Workspace <Workspace>`
Indices, Spectra Numbers, Detector IDs and locations of the detectors,
together with a flag showing which are monitors. If a spectrum contains
a group of detectors then the position shown will be the average
position of those detectors.

.. image:: ../../images/Showmar11060detectortable.PNG


In the example above, the first three spectra correspond to data from
monitors. The fourth spectrum (with a Spectrum Number of -1) loaded from
the MAR11060 run is for some reason not present in the Instrument
definition, and the remaining rows that are visible show histograms
wired up to detectors with spectrum number 5-7 or equivalently detectors
with IDs of 4101-3.

