.. _04_pim_ex_3:

============================
Python in Mantid: Exercise 3
============================

.. contents:: Table of contents
    :local:


Using ISIS Data
===============

Graphing
--------

#. Load the GEM data set - GEM40979.raw using SpectrumMin=431 & SpectrumMax=750
#. Convert units to dSpacing
#. Smooth the data using SmoothData using NPoints=20
#. Plot workspace indices 0,1,2
#. Rescale x-axis to range to 4 < x < 6
#. Rescale y-axis to range to 0 < y < 5e3
#. Plot workspace index 5
#. Merge this plot with the previous
#. Change the title of the X axis
#. Change the title of the Y axis

Reference: http://www.mantidproject.org/MantidPlot:_1D_Plots_in_Python

Instrument view
---------------

#. Load LOQ48097 data set
#. Get the instrument view
#. Change the colour map to _standard.map (All of the colourmap files can be found in C:\MantidInstall\colormaps)
#. Set the colour map range to (0,195)
#. Show the window

Using SNS Data
==============

Graphing
--------

#. Load the processed CNCS data file Training_Exercise3a_SNS.nxs
#. Plot workspace index 0
#. Merge the plot from index 1 with the first graph created
#. Merge index 2 with the first graph also
#. Rescale x-axis to range to -1.5 < x < 1.8
#. Change the y-axis to have a log scale

Instrument view
---------------

#. Load ARCS example data set (Training_Exercise3b_SNS.nxs)
#. Get the instrument view
#. Change the color map to BlackBodyRadiation.map (All of the colourmap files can be found in C:\MantidInstall\colormaps)
#. Set the colour map range to (0,2000)
#. Show the window

Using ILL Data
==============

Graphing
--------

#. Load the IN6 file : 164198.nxs
#. Plot workspace index 205
#. Merge the plot from index 209 with the first graph created
#. Rescale x-axis to range to 4400 < x < 5000 us.
#. Change the y-axis to have a log scale

Instrument view
---------------

#. Load D33 data set : D33041421_tof.nxs
#. Get the instrument view
#. Change the color map to BlackBodyRadiation.map (All of the colourmap files can be found in C:\MantidInstall\colormaps or /opt/Mantid/colormaps)
#. Set the colour map range to (0,500)
#. Show the window