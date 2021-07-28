.. _Mesh_Plots:

===============================
3D Mesh Plots for Sample Shapes
===============================

.. TO UPDATE find these images in a .pptx file at https://github.com/mantidproject/documents/blob/master/Images/Images_for_Docs/formatting_plots.pptx

`3D plotting in Matplotlib <https://matplotlib.org/mpl_toolkits/mplot3d/tutorial.html>`_

**Other Plot Types**

* :ref:`3D_Plots`
* :ref:`Basic_1D_Plots`
* :ref:`Waterfall_Plots`
* :ref:`Colorfill_Plots`


**General Plot Help**

* :ref:`06_formatting_plots`
* :ref:`02_scripting_plots`

|
|

.. contents:: 3D Mesh Plots - Table of contents
    :local:

Mesh Plots
==========

**Mesh Plots can only be accessed with a script, not through the Workbench interface**
Here the mesh is plotted as a Poly3DCollection `Polygon <https://matplotlib.org/stable/tutorials/toolkits/mplot3d.html#polygon-plots>`_.

These sample shapes can be copied using :ref:<algm-CopySample>.

Scripting
---------

Plot a MeshObject from an .stl (or .3mf) file:

.. plot::
   :include-source:

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    from mpl_toolkits.mplot3d.art3d import Poly3DCollection
    from mantid.api import AnalysisDataService as ADS

    # load sample shape mesh file for a workspace
    ws = CreateSampleWorkspace()
    # alternatively: ws = Load('filepath') or ws = ADS.retrieve('ws')
    ws = LoadSampleShape(ws, "tube.stl")

    # get shape and mesh vertices
    sample = ws.sample()
    shape = sample.getShape()
    mesh = shape.getMesh()

    # Create 3D Polygon and set facecolor
    mesh_polygon = Poly3DCollection(mesh, facecolors = ['g'], edgecolors = ['b'], alpha = 0.5, linewidths=0.1)

    fig, axes = plt.subplots(subplot_kw={'projection':'mantid3d'})
    axes.add_collection3d(mesh_polygon)

    # Auto scale to the mesh size
    axes_lims = mesh.flatten()
    axes.auto_scale_xyz(axes_lims, axes_lims, axes_lims)

    axes.set_title('Sample Shape: Tube')
    axes.set_xlabel('X / m')
    axes.set_ylabel('Y / m')
    axes.set_zlabel('Z / m')

    plt.show()

Plot a CSGObject defined in an XML string:

.. plot::
   :include-source:

    # import mantid algorithms, numpy and matplotlib
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np
    from mpl_toolkits.mplot3d.art3d import Poly3DCollection
    from mantid.api import AnalysisDataService as ADS

    ws = CreateSampleWorkspace()

    merge_xml = ' \
    <cylinder id="stick"> \
      <centre-of-bottom-base x="-0.5" y="0.0" z="0.0" /> \
      <axis x="1.0" y="0.0" z="0.0" />  \
      <radius val="0.05" /> \
      <height val="1.0" /> \
    </cylinder> \
    \
    <sphere id="some-sphere"> \
      <centre x="0.0"  y="0.0" z="0.0" /> \
      <radius val="0.5" /> \
    </sphere> \
    \
    <rotate-all x="90" y="-45" z="0" />
    <algebra val="some-sphere (# stick)" /> \
    '

    # Set sample geometry of workspace to this CSG object
    SetSample(ws, Geometry={'Shape': 'CSG', 'Value': cuboid_xml})

    sample = ws.sample()
    shape = sample.getShape()
    mesh = shape.getMesh()

    mesh_polygon_a = Poly3DCollection(mesh, facecolors = 'blue', linewidths=0.1)

    fig, axes = plt.subplots(subplot_kw={'projection':'mantid3d'})
    axes.add_collection3d(mesh_polygon_a)

    # Auto scale to the mesh size
    axes_lims = mesh.flatten()
    axes.auto_scale_xyz(axes_lims, axes_lims, axes_lims)

    axes.set_title('Sample Shape: Microphone')
    axes.set_xlabel('X / m')
    axes.set_ylabel('Y / m')
    axes.set_zlabel('Z / m')

    plt.show()

Plot Containers and Components:

.. code-block:: python

   ws = CreateSampleWorkspace()
   LoadInstrument(Workspace=ws,RewriteSpectraMap=True,InstrumentName="Pearl")
   SetSample(ws, Environment={'Name': 'Pearl'})

   sample = ws.sample()
   environment = sample.getEnvironment()

   '''getMesh() to plot the Sample Shape'''
   mesh = sample.getShape().getMesh()

   '''getMesh() to plot the Container Shape'''
   container_mesh = environment.getContainer().getShape().getMesh()

   '''getMesh() to plot any Component Shape'''
   # note Component index 0 is the Container:
   # container_mesh = environment.getComponent(0).getShape().getMesh()
   component_mesh = environment.getComponent(2).getMesh()

.. plot::
   :include-source:

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   from mpl_toolkits.mplot3d.art3d import Poly3DCollection

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()
   LoadInstrument(Workspace=ws,RewriteSpectraMap=True,InstrumentName="Pearl")
   SetSample(ws, Environment={'Name': 'Pearl'})

   sample = ws.sample()
   environment = sample.getEnvironment()

   mesh = sample.getShape().getMesh()
   container_mesh = environment.getContainer().getShape().getMesh()

   mesh_polygon_a = Poly3DCollection(mesh, facecolors = 'green', edgecolors='blue',alpha = 0.5, linewidths=0.1, zorder = 0.3)
   mesh_polygon_b = Poly3DCollection(container_mesh, edgecolors='red', alpha = 0.1, linewidths=0.05, zorder = 0.5)
   mesh_polygon_b.set_facecolor((1,0,0,0.5))

   fig, axes = plt.subplots(subplot_kw={'projection':'mantid3d'})
   axes.add_collection3d(mesh_polygon_a)
   axes.add_collection3d(mesh_polygon_b)

   for i in (1,3,5):
      print(i)
      mesh_polygon_i = Poly3DCollection(environment.getComponent(i).getMesh(), edgecolors='red', alpha = 0.1, linewidths=0.05, zorder = 0.5)
      mesh_polygon_i.set_facecolor((1,0,0,0.5))
      axes.add_collection3d(mesh_polygon_i)

   # Auto scale to the mesh size
   axes_lims = (-0.03,0.03)
   axes.auto_scale_xyz(axes_lims, axes_lims, axes_lims)

   axes.set_title('Pearl Sample in Container and Components(1,3,5)')
   axes.set_xlabel('X / m')
   axes.set_ylabel('Y / m')
   axes.set_zlabel('Z / m')
   axes.view_init(elev=5, azim=40)

   # Add arrow along beam direction (scaled down by 400)
   source = (ws.getInstrument().getSource().getPos())
   sample = ws.getInstrument().getSample().getPos() - source
   factor = 400
   axes.quiver(
         source[0]/factor, source[1]/factor, source[2]/factor,
         sample[0]/factor, sample[1]/factor, sample[2]/factor,
         color = 'black'
   )

   plt.show()

For more info on defining Containers and Components, see :ref:`_SampleEnvironment`.

For more plotting advice: :ref:`02_scripting_plots`

|
|

Plot Toolbar
------------

.. figure:: /images/PlotToolbar3DSurface.png
   :alt: Plot Toolbar Mesh Plots
   :align: center

|
|

Click Menus
-----------

.. figure:: /images/PlotClickMenus3DMesh.png
   :alt: Click Menus Surface Plots
   :align: center
   :width: 1500px

|
|

|FigureOptionsGear.png| ptions Menu
-----------------------------------

.. figure:: /images/PlotOptions3DSurface.png
   :alt: Plot Options 3D Surface
   :align: center


|
|

General
=======

**General Plot Help**

* :ref:`06_formatting_plots`
* :ref:`02_scripting_plots`

|
|

Plots Toolbox
-------------

.. figure:: /images/PlotsWindow.png
   :alt: Plot Toolbox
   :align: center
   :width: 800px

|
|

File > Settings
---------------

.. figure:: /images/PlotSettings.png
   :alt: Plot Settings
   :align: center
   :width: 850px

|
|

**Other Plotting Documentation**

* :ref:`plotting`
* `Matplotlib Keyboard Shortcuts <https://matplotlib.org/3.1.1/users/navigation_toolbar.html#navigation-keyboard-shortcuts>`_
* See :ref:`here <plotting>` for custom color cycles and colormaps

.. |FigureOptionsGear.png| image:: /images/FigureOptionsGear.png
   :width: 150px
