.. _06_formatting_plots:

================
Formatting Plots 
================

Plot Toolbar
============

The usual buttons on the Plot Toolbar:

- Home: Rescales the Axes on the plot
- Arrows: move back and forward between zoom levels (only activated after zooming or moving)
- Move: Interactively move the curve within the plot
- Zoom: Select an area to zoom in on
- Grid: Add gridlines
- Save: Save the image to file
- Print: Print the image!
- Gear: Open the Plot Options Menu
- Script: Generate a Script to reproduce this plot!
- Fit: Open the Fitting Tab

We will cover use of the Fitting Tab later...

.. figure:: /images/PlotToolbar.png
   :align: center
   :alt: Plot Toolbar

Plots Toolbox
=============

.. figure:: /images/PlotsWindow.png
   :align: left
   :alt: Plots Window

To show this window you may have to click on the Plots tab higlighted in Red.

The Plots Window great tool for organising your plots. It allows you to Show, Rename, Delete, Search for and Export Plots. It become very useful when you produce many plots and are struggling to find the one that you want!


Quickly adjusting 1D Plots
==========================

Many aspects of these graphs can be adjusted to fit how you want to
display the data. 

Double-click on the item you want to change can edit:

-  The graph title (Label).
-  The axes labels.
-  The axes themselves. You can change the range, scaling and gridlines.

Right-clicking on the centre of the Plot you can:

- Change the axes scale
- Toggle on/off the Normalization
- Add or remove Error bars
- Add Markers

Plot a 1D spectrum from the MARI or EMU workspaces we have used before and 
have a go at adjusting these plotting options. If you create a marker, have a go at dragging it and editing it!

.. figure:: /images/1DPlotmarkers.png
   :alt: Plot markers
   :align: center
   :width: 500px

Figure Options Menu
===================

.. figure:: /images/PlotOptions.png
   :alt: Plot Options
   :align: center
   :width: 800px

You have already seen the Figure Options Menu for Colorfill plots. For both 
Colorfill and normal 1D plots, you can use the Axes tab to edit the labels 
and Scales of the different Axes. Just as the "Images" tab was the most 
useful menu for Colorfill plots, for 1D plots this is the "Curves" tab. As 
shown above it is possible to edit the Color, Style and Width of a Line, add 
Markers and change their size and color, and show Error bars with options 
such as Capsize and Frequency of Errorbars (Error every how many points).

Please have a play with these options to see what you can do. For instance 
if you wish to change the label of a curve in the Legend, then edit the "Set 
curve label" entry on the Curves tab. Then on the Legend tab you will see 
Color and Font options.

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Displaying_data_in_multiple_workspaces|Mantid_Basic_Course|MBC_Exercise_Loading_And_Displaying_Data}}
