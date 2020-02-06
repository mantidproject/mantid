.. _05_plotting_multiple_workspaces:

============================
Plotting Multiple Workspaces 
============================

.. This should be updated as Mantid Workbench gains more Plot Advanced features.

For a Simple, Waterfall or Tiled 1D plot, you right-click on the Workspace and select "Plot > Spectrum...". The short-cut to this is to just double-click on the Workspace!

Waterfall Plot
==============

Waterfall plots are a useful tools for comparing different, but maybe similar, spectra.

#. Load the file "EMU00020884.nxs"
#. Open the Plot Spectrum dialog with either method above
#. For "Spectrum Numbers", enter "5-10"
#. Change the "Plot Type" to "Waterfall Plot"
#. Press OK

Then one gets (after clicking the home button to rescale the axes):

.. figure:: /images/ArtWaterfallN1.png
   :align: center
   :width: 600px

In the plot window Toolbar, the three buttons to the right allow you to change the offset between the curves, reverse the order of the curves and fill in the area under a curve. if you're ever unsure what one of these buttons is for, hover your mouse over it for a tool-tip!

Tiled Plots
===========

Tiled Plots can be done in a similar manner to
Waterfall plots, but with the appropriate choice of "Plot Type".
If you produce a Tiled plot with more than about 4 tiles, you may need to 
enlarge plot window or set it to full screen to appreciate all of the spectra.

.. figure:: /images/EMUTiledplot.png
   :align: center
   :width: 800px


.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Displaying_data_2D|Mantid_Basic_Course|MBC_Displaying_data_Formatting}}

