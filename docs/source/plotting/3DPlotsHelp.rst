.. _3D_Plots:

===============================
3D Plots: Surface and Wireframe
===============================

.. TO UPDATE find these images in a .pptx file at https://github.com/mantidproject/documents/blob/master/Images/Images_for_Docs/formatting_plots.pptx

`3D plotting in Matplotlib <https://matplotlib.org/mpl_toolkits/mplot3d/tutorial.html>`_

**Other Plot Types**

* :ref:`Basic_1D_Plots`
* :ref:`Waterfall_Plots`
* :ref:`Colorfill_Plots`
* :ref:`Mesh_Plots`


**General Plot Help**

* :ref:`06_formatting_plots`
* :ref:`02_scripting_plots`

|
|

.. contents:: 3D Plots Surface and Wireframe - Table of contents
    :local:

Surface Plots
=============

|
|

Plot Toolbar
------------

.. figure:: /images/PlotToolbar3DSurface.png
   :alt: Plot Toolbar Surface Plots
   :align: center

|
|

Click Menus
-----------

.. figure:: /images/PlotClickMenus3DSurface.png
   :alt: Click Menus Surface Plots
   :align: center
   :width: 1500px

|
|

|O| ptions Menu
-----------------------------------

.. figure:: /images/PlotOptions3DSurface.png
   :alt: Plot Options 3D Surface
   :align: center


|
|


Scripting
---------


Basic example of plotting a `Surface <https://matplotlib.org/mpl_toolkits/mplot3d/tutorial.html#surface-plots>`_:

.. plot::
   :include-source:

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt

    data = Load('MUSR00015189.nxs')
    data = mtd['data_1'] # Extract individual workspace from group

    fig, ax = plt.subplots(subplot_kw={'projection':'mantid3d'})
    ax.plot_surface(data, cmap='viridis')

    fig.show()

For more advice: :ref:`02_scripting_plots`

|
|

Wireframe Plots
===============

|
|

Plot Toolbar
------------

.. figure:: /images/PlotToolbar3DWireframe.png
   :alt: Plot Toolbar 3D Wireframe
   :align: center

|
|

Change the Color of the Wireframe:

.. figure:: /images/ColorPaletteWireframeContour.png
   :alt: Color Palette Wireframe and Contour
   :align: center
   :width: 600px


Click Menus
-----------

.. figure:: /images/PlotClickMenus3DWireframe.png
   :alt: Click Menus 3D Wireframe
   :align: center
   :width: 1500px

|
|

|O| ptions Menu
-----------------------------------

.. figure:: /images/PlotOptions3DWireframe.png
   :alt: Plot Options 3D Wireframe
   :align: center

|
|

Scripting
---------

Basic example of plotting a `Wireframe <https://matplotlib.org/mpl_toolkits/mplot3d/tutorial.html#wireframe-plots>`_:

.. plot::
   :include-source:

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt

    data = Load('MAR11060.nxs')

    fig, ax = plt.subplots(subplot_kw={'projection':'mantid3d'})
    ax.plot_wireframe(data, color='#1f77b4')

    fig.show()

For more advice: :ref:`02_scripting_plots`

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

.. |O| image:: /images/FigureOptionsGear.png
   :width: 150px
