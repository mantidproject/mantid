.. _Colorfill_Plots:

===========================
Colorfill and Contour Plots
===========================

.. TO UPDATE find these images in a .pptx file at https://github.com/mantidproject/documents/blob/master/Images/Images_for_Docs/formatting_plots.pptx

**Other Plot Types**

* :ref:`Basic_1D_Plots`
* :ref:`Waterfall_Plots`
* :ref:`3D_Plots`
* :ref:`Mesh_Plots`

**General Plot Help**

* :ref:`06_formatting_plots`
* :ref:`02_scripting_plots`

|
|

.. contents:: Colorfill and Contour Plots - Table of contents
    :local:

Colorfill Plots
===============

|
|

Plot Toolbar
------------

.. figure:: /images/PlotToolbarTiledColorfill.png
   :alt: Plot Toolbar Colorfill
   :align: center

|
|

Click Menus
-----------

.. figure:: /images/PlotClickMenusColorfill.png
   :alt: Click Menus Colorfill
   :align: center
   :width: 1500px

|
|

|O| ptions Menu
-----------------------------------

.. figure:: /images/PlotOptionsColorfillContour.png
   :alt: Plot Options Colorfill and Contour
   :align: center


|
|


Scripting
---------

Click the generate a script button |GenerateAScript.png| on a `Colorfill Plot <https://matplotlib.org/stable/api/_as_gen/matplotlib.pyplot.imshow.html>`_:

.. plot::
   :include-source:

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   import numpy as np
   from matplotlib.colors import LogNorm
   from matplotlib.ticker import LogLocator
   from mantid.api import AnalysisDataService as ADS

   MAR11060 = Load('MAR11060')

   fig, axes = plt.subplots(figsize=[8.0, 7.0], num='MAR11060-1', subplot_kw={'projection': 'mantid'})
   cfill = axes.imshow(MAR11060, aspect='auto', cmap='viridis', distribution=False, origin='lower')
   cfill.set_norm(LogNorm(vmin=0.0001, vmax=3792.3352))
   # If no ticks appear on the color bar remove the subs argument inside the LogLocator below
   cbar = fig.colorbar(cfill, ax=[axes], ticks=LogLocator(subs=np.arange(1, 10)), pad=0.06)
   cbar.set_label('Counts ($\\mu s$)$^{-1}$')
   axes.set_title('MAR11060')
   axes.set_xlabel('Time-of-flight ($\\mu s$)')
   axes.set_ylabel('Spectrum')
   axes.set_xlim([5.0, 19992.0])
   axes.set_ylim([0.5, 922.5])

   fig.show()

For more advice:

* :ref:`01_basic_plot_scripting`
* :ref:`02_scripting_plots`

|
|

Contour Plots
=============

A Contour Plot is essentially a Colorfill Plot with Contour lines overlaid.

|
|

Plot Toolbar
------------

.. figure:: /images/PlotToolbarContour.png
   :alt: Plot Toolbar Contour
   :align: center

|
|

Change the Color of the Contour lines:

.. figure:: /images/ColorPaletteWireframeContour.png
   :alt: Color Palette Wireframe and Contour
   :align: center
   :width: 600px

|
|

Click Menus
-----------

.. figure:: /images/PlotClickMenusContour.png
   :alt: Click Menus Contour
   :align: center
   :width: 1500px

|
|

|O| ptions Menu
-----------------------------------

.. figure:: /images/PlotOptionsColorfillContour.png
   :alt: Plot Options Colorfill and Contour
   :align: center

|
|

Scripting
---------

Basic example of plotting a `Contour Plot <https://matplotlib.org/stable/api/_as_gen/matplotlib.axes.Axes.contour.html>`_:

.. plot::
   :include-source:

   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   import numpy as np

   data = Load('SANSLOQCan2D.nxs')

   fig, axes = plt.subplots(subplot_kw={'projection':'mantid'})

   # IMPORTANT to set origin to lower
   c = axes.imshow(data, origin = 'lower', cmap='viridis', aspect='auto')

   # Overlay contours
   axes.contour(data, levels=np.linspace(10, 60, 6), colors='yellow', alpha=0.5)
   axes.set_title('SANSLOQCan2D.nxs')
   cbar=fig.colorbar(c)
   cbar.set_label('Counts ($\mu s$)$^{-1}$') #add text to colorbar
   fig.tight_layout()

   fig.show()

For more advice:

* :ref:`01_basic_plot_scripting`
* :ref:`02_scripting_plots`

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
* `Matplotlib Keyboard Shortcuts <https://matplotlib.org/stable/users/explain/figure/interactive.html#navigation-keyboard-shortcuts>`_

.. |O| image:: /images/FigureOptionsGear.png
   :width: 150px
.. |GenerateAScript.png| image:: /images/GenerateAScript.png
   :width: 30px
