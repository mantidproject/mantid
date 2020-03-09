.. _03_differences:

=================================
Extra functionality of MantidPlot
=================================

While we are working to bring Workbench functionality in line with Mantid Plot, there are a few features are as yet obly available in Plot.


.. figure:: /images/GeneralCategory.png
   :align: right
   :width: 250px

Interfaces
----------

Almost all interfaces have been moved over to Workbench, with the exception of the 
General category.


3D and Advanced plotting
------------------------

- Right-click on a Workspace and select "Show Data"
- With the Data Table as your active window, a "3D Plot" drop-down menu appears at the top of the main window. Within this are many options such as Contour and 3D surface plots (which are coming soon to Workbench!) Have a play with the options available!

- Right-click on a Workspace and select "Plot advanced". This will open up a menu where you can produce a tiled or waterfall plot (features that are in Workbench!). A little extra here is plotting by log value (say temperature).
- Right-clicking on a WorkspaceGroup will allow "Plot advanced" options of Surface and Contour for comparing different workspaces.

.. figure:: /images/250px-PlotAdvancedDefault.png
   :align: center
   :width: 250px

Spectrum / Slice Viewer
--------------------------------

In Mantid Plot you can right-click on a workspace and show SliceViewer or SpectrumViewer. Within workbench we are combining these (into just the SliceViewer). Some of the functionality of SpectrumViewer within Plot has been implemented in Workbench but not all. Open Spectrum Viewer to see what options there are!

.. figure:: /images/PlotSliceviewer.png
   :align: center

Also a key feature of SliceViewer in MantidPlot is the peaks viewer functionality. It can show you where assigned peaks are in comparison to the raw data. This is being added to Workbench very soon!

.. figure:: /images/PlotPeaksviewer.png
   :align: center
