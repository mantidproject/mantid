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

Scripting
---------


Basic example of plotting a general Poly3DCollection `Polygon <https://matplotlib.org/stable/tutorials/toolkits/mplot3d.html#polygon-plots>`_:

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


For more advice: :ref:`02_scripting_plots`

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
