.. _05_plotting_multiple_workspaces:

============================
Plotting Multiple Workspaces 
============================

Plot Advanced
=============

#. Run the Python script "TempInGroup.py" found in the data directory,
   by opening it in the Mantid Script Window and selecting
   Execute->Execute All
#. Right click on the workspace "TestGroup"
#. Select "Plot Advanced ..."

Then we get the following dialog box:

.. figure:: /images/250px-PlotAdvancedDefault.png
   :alt: centre
   :width: 250px

Surface Plot
============

#. Select "Plot Type" of "Surface Plot of Group"
#. Press "Plot All"

Then we get the following surface plot:

.. figure:: /images/ArtSurfacePlotN1.PNG
   :alt: centre
   :width: 500px

Plot with Value of Chosen Log
-----------------------------

#. Do another advanced plot on "TestGroup".
#. Select surface plot again.
#. In "Log value to plot against" select "Temp".
#. Press "Plot All"

and the "Temp" log appears on an axis in place of workspace index:

.. figure:: /images/ArtSurfacePlotT1.PNG
   :alt: centre
   :width: 500px

Plot with Custom Log
--------------------

.. figure:: /images/ArtRightGUISurfaceCustomSq1.PNG
   :alt: centre
   :width: 250px

#. Select Plot advanced and fill in as shown above
#. Press "OK"

Then one gets the following with custom log "Square" shown in an axis in
place of Workspace index:

.. figure:: /images/ArtSurfacePlotCSq1.PNG
   :alt: centre
   :width: 500px

Contour Plot
============

A contour plot can be generated in the same manner as surface plot, but
with "Contour Plot of Group" as the "Plot type" instead of "Surface Plot
of Group".

Waterfall Plot
==============

#. Select "Plot Advanced" on the group "TestGroup"
#. Enter spectra number 6
#. Select "Plot Type" of "Waterfall Plot"
#. Press OK

Then one gets (after moving the legend to a convenient place):

.. figure:: /images/ArtWaterfallN1.PNG
   :alt: centre
   :width: 600px

.. _plot-with-value-of-chosen-log-1:

Plot with Value of Chosen Log
-----------------------------

#. Select "Plot Advanced" on the group "TestGroup"
#. Enter spectra number 6
#. Select "Plot Type" of "Waterfall Plot"
#. In "Log value to plot against" select "Temp".
#. Press OK

Then one gets (after moving the legend to a convenient place):

.. figure:: /images/ArtWaterfallT1.PNG
   :alt: centre
   :width: 600px

The values of the selected log "temp" are shown in the legend.

.. _plot-with-custom-log-1:

Plot with Custom Log
--------------------

#. Select "Plot Advanced" on the group "TestGroup"
#. Enter spectra number 6
#. Select "Plot Type" of "Waterfall Plot"
#. In "Log value to plot against" select "Custom".
#. Add custom log values "1,4,9,16,25,36,49,64,81,100".
#. Press OK

Then one gets (after moving the legend to a convenient place):

.. figure:: /images/ArtWaterfallC1.PNG
   :alt: centre
   :width: 600px

The custom log values are shown in the legend.

Plot of 2 spectra with Custom Log
---------------------------------

.. figure:: /images/ArtRightGUIWaterfallCustom2sp1.PNG
   :alt: centre
   :width: 400px

#. Select 4 workspaces and fill in Plot Advanced as shown above
#. Press "OK"

Then one gets (after moving the legend to a convenient place):

|centre| 

Here the legend shows both the log value and the spectrum
number.

1D and Tiled Plots
==================

Simple 1D plots and Tile Plots can be done in a similar manner as
Waterfall plots, but with the appropriate choice of "Plot Type".

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Displaying_data_2D|Mantid_Basic_Course|MBC_Displaying_data_Formatting}}

.. |centre| image:: /images/ArtWaterfallCustom2sp1.PNG
   :width: 600px
