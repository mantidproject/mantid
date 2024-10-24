.. _main_window:

=====================
Workbench Main Window
=====================

.. raw:: html

    <style> .red {color:#FF0000; font-weight:bold} </style>
    <style> .green {color:#008000; font-weight:bold} </style>
    <style> .blue {color:#0000FF; font-weight:bold} </style>
    <style> .orange {color:#FF8C00; font-weight:bold} </style>
    <style> .purple {color:#A431E6; font-weight:bold} </style>
    <style> .turquoise {color:#40E0D0; font-weight:bold} </style>

.. role:: red
.. role:: blue
.. role:: green
.. role:: orange
.. role:: purple
.. role:: turquoise

Layout
======

When you open Mantid Workbench it will look something like this:

.. figure:: /images/MantidWorkbenchMainWindow.png
   :alt: Main Window
   :align: center

As a quick introduction, the main window contains these Toolboxes:

- :blue:`Workspaces Toolbox`: Where data is stored within Mantid, in what we call Workspaces (see later for in-depth explanation).
- :orange:`Algorithms Toolbox`: Find an algorithm to manipulate your data.
- :purple:`Plots Toolbox`: Click on the Plots button in the bottom-left to reveal this toolbox. When you plot a graph in Workbench, it can be tracked here.
- :green:`Script Editor`: Run Python scripts here to control Mantid. Much more functionality with scripts, but beyond the scope of this basic course.
- :red:`Messages Box`: Any output text, from an algorithm or script. :red:`Errors are displayed here in red`.
- :turquoise:`System Memory Usage`: Displays the current system memory usage.

**Note** to find out what Mantid version you are using, look at the first message in the :red:`Messages Box`. You can see I'm using Mantid 6.1.0.

**Note** You can click on the top bar of each toolbox and drag it to edit the layout.
To reset the layout for MantidWorkbench to the default, go to View > Restore Default Layout.

Quick Intro to Workspaces and Bins
==================================

A workspace is data stored within Mantid. It will appear in the Workspaces Toolbox.
When you load a data file, it will create a Workspace. Likewise you can save a Workspace to a file.
Many of the features of Mantid perform operations on Workspaces. There are even different types of Workspaces in Mantid, which hold data in different formats.

A usual Workspace consists of a number of spectra, each cut into blocks called bins (more about spectra later). Mantid sums the number of counts in each bin and the line we plot connects the top of these bins.

(Note that the bins can have different widths (defined by the instrument set-up)), such as below the bin-widths change from 10 to 20).

.. plot::
   :align: center

   import numpy as np
   import matplotlib.pyplot as plt

   neutrons_detected_at = np.array([5,11,15,22,23,25,35,36,37,
   	                                38,43,45,49,50,55,58,62,64,
   	                                74,76,78,80,85,90])
   counts_per_bin=(1,2,3,4,6,5,3)
   bin_edges = np.array([0,10,20,30,40,60,80,100])

   number_of_bins = len(bin_edges)-1
   bin_centres = np.zeros(number_of_bins)

   for i in range(number_of_bins):
       bin_centres[i] = (bin_edges[i+1] + bin_edges[i])/2

   fig, ax = plt.subplots(subplot_kw={'projection': 'mantid'})

   ax.hist(neutrons_detected_at, bins=bin_edges, align='mid', color='b', edgecolor='black',density = False, label='Bins')
   ax.plot(bin_centres,counts_per_bin,color='red', label = 'Line')


   # Add axis labels
   ax.set_xlabel("X data      eg. Time ($\mu s$)")
   ax.set_ylabel("Y data      eg. Counts ($\mu s$)$^{-1}$")
   ax.set_title("Bins and Line Plots")
   ax.set_xticks(bin_edges)
   ax.legend()
   fig.show()
