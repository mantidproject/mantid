.. _Basic_1D_Plots:

========================
Basic 1D and Tiled Plots
========================

.. TO UPDATE find these images in a .pptx file at https://github.com/mantidproject/documents/blob/master/Images/Images_for_Docs/formatting_plots.pptx

|
|

**Other Plot Types**

* :ref:`Waterfall_Plots`
* :ref:`Colorfill_Plots`
* :ref:`3D_Plots`
* :ref:`Mesh_Plots`

**General Plot Help**

* :ref:`06_formatting_plots`
* :ref:`02_scripting_plots`

|
|

.. contents:: Basic 1D and Tiled Plots - Table of contents
    :local:

Single 1D Plots
===============

|
|

To create a single 1D plot, right-click on a workspace and select ``Plot > Spectrum...``.
Check the ``Plot Type`` is set to ``Individual`` before choosing which spectra to plot and selecting ``OK``.

|
|

Plot Toolbar
------------

.. figure:: /images/PlotToolbar1DSpectrum.png
   :alt: Plot Toolbar 1D Spectrum
   :align: center

|
|

Click Menus
-----------

.. figure:: /images/PlotClickMenus1D.png
   :alt: Click Menus 1D
   :align: center
   :width: 1500px

|
|

|O| ptions Menu
---------------

.. figure:: /images/PlotOptions.png
   :alt: Plot Options Axes Legend
   :align: center


| **TIP**: Change a Legend entry by editing `Set curve label` on the Curves tab.
  Then on the Legend tab there are Color and Font options.
|
|

.. figure:: /images/PlotOptionsCurves.jpg
   :alt: Plot Options Axes Legend
   :align: center

|
|

Scripting
---------

Click the generate a script button |GenerateAScript.png| on a 1D Plot:

.. plot::
   :include-source:

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   from mantid.plots.utility import MantidAxType

   MAR11060 = Load('MAR11060')

   fig, axes = plt.subplots(edgecolor='#ffffff', num='MAR11060-1', subplot_kw={'projection': 'mantid'})
   axes.plot(MAR11060, color='#1f77b4', label='MAR11060: spec 1', wkspIndex=0)
   axes.plot(MAR11060, color='#ff7f0e', label='MAR11060: spec 2', wkspIndex=1)
   axes.plot(MAR11060, color='#2ca02c', label='MAR11060: spec 3', wkspIndex=2)
   axes.tick_params(axis='x', which='major', **{'gridOn': False, 'tick1On': True, 'tick2On': False, 'label1On': True, 'label2On': False, 'size': 6, 'tickdir': 'out', 'width': 1})
   axes.tick_params(axis='y', which='major', **{'gridOn': False, 'tick1On': True, 'tick2On': False, 'label1On': True, 'label2On': False, 'size': 6, 'tickdir': 'out', 'width': 1})
   axes.set_title('MAR11060')
   axes.set_xlabel('Time-of-flight ($\\mu s$)')
   axes.set_ylabel('Counts ($\\mu s$)$^{-1}$')
   legend = axes.legend(fontsize=8.0).set_draggable(True).legend

   fig.show()

For more advice: :ref:`02_scripting_plots`

|
|

Tiled Plots
===========

|
|

To create a tiled plot, right-click on a workspace and select ``Plot > Spectrum...``.
Check the ``Plot Type`` is set to ``Tiled`` before choosing which spectra to plot and selecting ``OK``.

|
|

Plot Toolbar
------------

.. figure:: /images/PlotToolbarTiledColorfill.png
   :alt: Plot Toolbar Tiled and Colorfill Plots
   :align: center

|
|

Click Menus
-----------

.. figure:: /images/PlotClickMenusTiled.png
   :alt: Click Menus Tiled Plots
   :align: center
   :width: 1500px

|
|

|O| ptions Menu
---------------

Tiled plots are essentially an array of axes (1D plots) on the same figure. As such, when editing them in the Options Menu, you should take care to select the correct set of axes:

.. figure:: /images/PlotOptionsTiledAxes.png
   :alt: Plot Options Tiled Axes
   :align: center

|

.. figure:: /images/PlotOptionsTiled.png
   :alt: Plot Options Axes Legend Tiled plots
   :align: center

| **TIP**: Change a Legend entry by editing `Set curve label` on the Curves tab.
  Then on the Legend tab there are Color and Font options.
|
|

.. figure:: /images/PlotOptionsCurves.jpg
   :alt: Plot Options Curves
   :align: center

|
|

Scripting
---------

An example script for a Tiled Plot:

.. plot::
   :include-source:

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   from mantid.plots.utility import MantidAxType

   MAR11060 = Load('MAR11060')

   fig, axes = plt.subplots(edgecolor='#ffffff', ncols=2, nrows=2, num='MAR11060-1', subplot_kw={'projection': 'mantid'})
   axes[0][0].plot(MAR11060, color='#1f77b4', label='MAR11060: spec 1', wkspIndex=0)
   axes[0][0].set_xlabel('Time-of-flight ($\\mu s$)')
   axes[0][0].set_ylabel('Counts ($\\mu s$)$^{-1}$')
   legend = axes[0][0].legend(fontsize=8.0) #.set_draggable(True).legend # uncomment to set the legend draggable

   axes[0][1].plot(MAR11060, color='#1f77b4', label='MAR11060: spec 2', wkspIndex=1)
   axes[0][1].set_xlabel('Time-of-flight ($\\mu s$)')
   axes[0][1].set_ylabel('Counts ($\\mu s$)$^{-1}$')
   legend = axes[0][1].legend(fontsize=8.0) #.set_draggable(True).legend # uncomment to set the legend draggable

   axes[1][0].plot(MAR11060, color='#1f77b4', label='MAR11060: spec 3', wkspIndex=2)
   axes[1][0].set_xlabel('Time-of-flight ($\\mu s$)')
   axes[1][0].set_ylabel('Counts ($\\mu s$)$^{-1}$')
   legend = axes[1][0].legend(fontsize=8.0) #.set_draggable(True).legend # uncomment to set the legend draggable

   axes[1][1].plot(MAR11060, color='#1f77b4', label='MAR11060: spec 4', wkspIndex=3)
   axes[1][1].set_xlabel('Time-of-flight ($\\mu s$)')
   axes[1][1].set_ylabel('Counts ($\\mu s$)$^{-1}$')
   legend = axes[1][1].legend(fontsize=8.0) #.set_draggable(True).legend # uncomment to set the legend draggable

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
* `Matplotlib Keyboard Shortcuts <https://matplotlib.org/stable/users/explain/figure/interactive.html#navigation-keyboard-shortcuts>`_

.. |O| image:: /images/FigureOptionsGear.png
   :width: 150px
.. |GenerateAScript.png| image:: /images/GenerateAScript.png
   :width: 30px
