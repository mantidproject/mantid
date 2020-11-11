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

.. figure:: ../images/Workbench/InstrumentViewerOverview.png
    :align: center
    :width: 635


Instrument View Pane
--------------------
The instrument view window can be obtained by right clicking on the workspace of interest and selecting "Show instrument". 
A 2D projection of the detector arrangement will be visible with each detector pixel color coded depending on the integrated number of counts in its corresponding spectrum, as in the image below.

.. figure:: ../images/Workbench/InstrumentViewerWindow.png
    :align: center
    :width: 635

It is possible to alter the view of the instrument by holding down, either, mouse button and moving (dragging) it within the window. 
There are other mouse and key board controls and these are listed at the bottom of the instrument view window. 
There are other ways to change the view, find and get information about components and these are described in the next sections.

The control panel of the instrument view has four tabs: Render, Pick, Mask, and Instrument Tree.


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

  myiv.set_tof_range(1, 10000)

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

