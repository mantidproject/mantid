.. _03_pim_ex_3:

============================
Python in Mantid: Exercise 3
============================

.. contents:: Table of contents
    :local:

A - Direct Matplotlib with SNS Data
===================================

#. Load the processed CNCS data file Training_Exercise3a_SNS.nxs
#. Directly using MPL (eg. axes.plot() ), plot the first 5 spectra
#. Optionally set labels and colors for each spectrum

#. Again directly in MPL, plot the final spectrum with errorbars
#. Optionally, set a capsize > 0, and choose a color and label

#. Rescale x-axis limits to -1.5 < x < 1.8
#. Change the y-axis to have a log scale
#. Give the plot a title, legend and don't forget to show the plot


B - plotSpectrum with ISIS Data
===============================

#. Load the GEM data set - `GEM40979.raw` using SpectrumMin=431 & SpectrumMax=750
#. Convert units to dSpacing
#. Smooth the data using :ref:`algm-SmoothData` using NPoints=20

#. Using `plotSpectrum()`, plot workspace indices 0,1,2
#. Set the x-axis limits to 4 < x < 6
#. Set the y-axis limits to 0 < y < 5e3

#. Again using `plotSpectrum()`, plot workspace index 5 on the same plot window
#. Change the label of the Y axis
#. Give the plot a legend with labels for each curve
#. Give the plot a title


C - 2D and 3D Plot ILL Data
===========================

#. Load the file 164198.nxs - note that if you may have to uncheck "Search Data Archive" or give the full path to load this file rather than the corresponding run number of your default instrument.
#. :ref:`algm-ExtractSpectra` using X range 470-490 and WorkspaceIndex range 199-209. Use this region of interest for plotting.

#. Produce a figure and axes for subplots with ncols=2, nrows=1, using the mantid projection and also set the figsize = (6,4)
#. Produce a 2D colorfill plot, using the imshow method, on both subplots (indexed as axes[0] and axes[1])
#. For both set the colormap to `jet` and aspect='auto'
#. Overlay contour lines on the second subplot (axes[1]), colored white and with alpha = 0.5
#. Set the title to 'Colorfill' for axes[0] and 'Contour' for axes[1]
#. Add a colorbar to this figure
#. Set the colorbar label to 'Counts ($\mu s$)$^{-1}$'

#. Get another figure and axes for subplots with ncols=2, nrows=1, using the mantid3d projection and also set figsize = (8,3)
#. Add a Surface and Wireframe plot to the subplot axes respectively (indexed as axes[0] and axes[1])
#. Set the colormap for the surface plot to 'summer' and the color for the wireframe to 'darkmagenta'
#. Set the title for each subplot as 'Surface' and 'Wireframe'

#. Remember to show the plots


:ref:`Solutions <03_pim_sol>`
