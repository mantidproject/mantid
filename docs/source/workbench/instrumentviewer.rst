.. _InstrumentViewer:

========================
Instrument Viewer Widget
========================

Overview
--------

Mantid visualisation system allows users to view the geometry of an instrument and also select and query individual detectors.
An instrument is always shown in the context of a workspace containing data recorded with this instrument.
The detectors are coloured according to the data in the corresponding spectra of the workspace.
The user can select individual detectors by clicking on them and request to see the data recorded by this detector as a table or a graph.
The instrument view also allows users to see close-ups of any component.


Instrument View Pane
--------------------
The instrument view window can be obtained by right clicking on the workspace of interest and selecting "Show instrument". 
A 2D projection of the detector arrangement will be visible with each detector pixel color coded depending on the integrated number of counts in its corresponding spectrum, as in the image below.

.. figure:: ../images/Workbench/InstrumentViewer/Overview.png
    :align: center
    :width: 635

It is possible to alter the view of the instrument by holding down, either, mouse button and moving (dragging) it within the window. 
There are other mouse and key board controls and these are listed at the bottom of the instrument view window. 
There are other ways to change the view, find and get information about components and these are described in the next sections.

The control panel of the instrument view has four tabs: Render, Pick, Mask, and Instrument Tree.


Render Tab
----------
The Render tab contains controls for managing the on-screen appearance of the instrument and collected data"```
The top-most combo-box control allows the user to select the way the instrument is projected onto the screen.
The default setting is "Full 3D" which gives a 3D view of the instrument in an orthogonal projection.
The other options are "unwrapped views" projecting the instrument onto a curved surface and then unwrapping it onto the screen.
There is a choice between cylinders and spheres.
The unwrapped (or flat) views allow zooming by selecting a rectangular region with the mouse (left click and drag).
Right mouse click undoes the last zoom.

.. figure:: ../images/Workbench/InstrumentViewer/RenderTab.png
    :align: left
    :width: 635

The next control is Axis View, visible only in the 3D mode, resets the view so that the instrument is fully visible and the specified axis is perpendicular to the screen.

The Display Settings button controls the appearance of the instrument in the view. 
It's worth mentioning the "Use OpenGL" option. 
It toggles between two display modes of a flat view: the one that uses OpenGL to render the instrument and the one that doesn't.

.. figure:: ../images/Workbench/InstrumentViewer/DisplaySettings.png
    :align: left
    :width: 635

This option can be useful if the instrument is viewed over a slow network connection for example.

The "Save image" button allows the image to be save into a file.

The colour bar axis below maps the colours of the detectors to the integrated number of counts in their spectra. 
The axis also defines the minimum and maximum values which can be edited using the text boxes below and above the colour bar and also by clicking on the bar and dragging in the vertical direction. 
Clicking and dragging the upper half of the bar changes the maximum while the lower half modifies the minimum. 
Mantid comes with a number of color map files and these can be loaded by selecting the "Display Settings"->"Color Map".


Pick Tab
--------
Pick Tab is for displaying information about detectors of an instruments and data collected by them.
You can also manipulate the peak markers in this tab.
At the top of the tab there is a tool bar for switching between different interactive tools.
The text box below the tool bar show textual information about selected detector: its name, ID, index in the workspace, cartesian coordinates of the detector (xyz) in metres, spherical coordinates of the detector (rtp, which stands for r, \theta, and \phi) where the distance is in metres and the angles are in degrees, the full path of the detector in the instrument tree, integrated counts, and the units of the X vector in the underlying workspace.

.. figure:: ../images/Workbench/InstrumentViewer/PickTab.png
    :align: left
    :width: 635

More doc will be migrated from MantidPlot as new features being consolidated.


Draw Tab
--------
The Draw tab contains tools for creating and editing geometrical shapes which can be used for selecting regions of interest (ROI), masking or grouping detectors. 
The tab contains a mini toolbar, a shape property browser and a set of buttons to use the shapes.

.. figure:: ../images/Workbench/InstrumentViewer/DrawTab.png
    :align: left
    :width: 635

More doc will be migrated from MantidPlot as new features being consolidated.


Python Control
--------------
Many aspects of the instrument view can be controlled from Python.
To use the Python interface for InstrumentViewer, use the following code to import necessary library

.. code-block:: python

  from mantidqt.widgets.instrumentview.instrument_view import pyInstrumentView
  from mantidqt.widgets.instrumentview.instrument_view import SurfaceType, TabName

then load the Nexus data into a workspace

.. code-block:: python

  ws = LoadEventNexus(Filename=nexus_path, NumberOfBins=10)

Now we are done with the necesary preparation, time to get a handle to the window itself (this will create a fresh window)

.. code-block:: python

  myiv = pyInstrumentView(ws)
  myiv.show_view()

To set the integration range (time-of-flight), use

.. code-block:: python

  myiv.set_x_range(1, 10000)

To switch to a different tab, use

.. code-block:: python

  myiv.select_tab(TabName.Render)

To select the projection type (surface type), use

.. code-block:: python

  myiv.select_surface_type(SurfaceType.SphericalY)

To switch to a different viewing axis, use

.. code-block:: python

  myiv.set_axis("Y+")

To elect the range for the data (intensity, color map legend), use

.. code-block:: python

  myiv.set_intensity_min(1)
  myiv.set_intensity_max(1000)
  myiv.set_intensity_range(1, 1000)
