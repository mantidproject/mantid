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

|FigureOptionsGear.png| ptions Menu
-----------------------------------

.. figure:: /images/PlotOptions.png
   :alt: Plot Options Axes Legend
   :align: center


| **TIP**: Change a Legend entry by editing `Set curve label` on the Curves tab.
  Then on the Legend tab there are Color and Font options.
|
|

.. figure:: /images/PlotOptionsCurves.png
   :alt: Plot Options Axes Legend
   :align: center

|
|

Scripting
---------

Click the generate a script button |GenerateAScript.png| on a 1D Plot:

.. code-block:: python

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   import numpy as np

   from mantid.api import AnalysisDataService as ADS

   MAR11060 = ADS.retrieve('MAR11060') #May replace with Load('MAR11060')

   fig, axes = plt.subplots(edgecolor='#ffffff', num='MAR11060-1', subplot_kw={'projection': 'mantid'})
   axes.plot(MAR11060, color='#1f77b4', label='MAR11060: spec 1', specNum=1)
   axes.plot(MAR11060, color='#ff7f0e', label='MAR11060: spec 2', specNum=2)
   axes.plot(MAR11060, color='#2ca02c', label='MAR11060: spec 3', specNum=3)
   axes.set_title('MAR11060')
   axes.set_xlabel('Time-of-flight ($\mu s$)')
   axes.set_ylabel('Counts ($\mu s$)$^{-1}$')
   axes.legend().set_draggable(True)

   plt.show()

.. plot::

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt

   MAR11060 = Load('MAR11060')

   fig, axes = plt.subplots(edgecolor='#ffffff', num='MAR11060-1', subplot_kw={'projection': 'mantid'})
   axes.plot(MAR11060, color='#1f77b4', label='MAR11060: spec 1', specNum=1)
   axes.plot(MAR11060, color='#ff7f0e', label='MAR11060: spec 2', specNum=2)
   axes.plot(MAR11060, color='#2ca02c', label='MAR11060: spec 3', specNum=3)
   axes.set_title('MAR11060')
   axes.set_xlabel('Time-of-flight ($\mu s$)')
   axes.set_ylabel('Counts ($\mu s$)$^{-1}$')
   axes.legend()   #.set_draggable(True) # uncomment to set the legend draggable

   plt.show()

For more advice: :ref:`02_scripting_plots`

|
|

Tiled Plots
===========

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

|FigureOptionsGear.png| ptions Menu
-----------------------------------

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

.. figure:: /images/PlotOptionsCurves.png
   :alt: Plot Options Curves
   :align: center

|
|

Scripting
---------

Click the generate a script button |GenerateAScript.png| on a Tiled Plot:

.. code-block:: python

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   import numpy as np

   from mantid.api import AnalysisDataService as ADS

   MAR11060 = ADS.retrieve('MAR11060') #May replace with Load('MAR11060')

   fig, axes = plt.subplots(edgecolor='#ffffff', ncols=2, nrows=2, num='MAR11060-1', subplot_kw={'projection': 'mantid'})
   axes[0][0].plot(MAR11060, color='#1f77b4', label='MAR11060: spec 1', specNum=1)
   axes[0][0].set_xlabel('Time-of-flight ($\mu s$)')
   axes[0][0].set_ylabel('Counts ($\mu s$)$^{-1}$')
   axes[0][0].legend().set_draggable(True)

   axes[0][1].plot(MAR11060, color='#1f77b4', label='MAR11060: spec 2', specNum=2)
   axes[0][1].set_xlabel('Time-of-flight ($\mu s$)')
   axes[0][1].set_ylabel('Counts ($\mu s$)$^{-1}$')
   axes[0][1].legend().set_draggable(True)

   axes[1][0].plot(MAR11060, color='#1f77b4', label='MAR11060: spec 3', specNum=3)
   axes[1][0].set_xlabel('Time-of-flight ($\mu s$)')
   axes[1][0].set_ylabel('Counts ($\mu s$)$^{-1}$')
   axes[1][0].legend().set_draggable(True)

   axes[1][1].plot(MAR11060, color='#1f77b4', label='MAR11060: spec 4', specNum=4)
   axes[1][1].set_xlabel('Time-of-flight ($\mu s$)')
   axes[1][1].set_ylabel('Counts ($\mu s$)$^{-1}$')
   axes[1][1].legend().set_draggable(True)

   plt.show()

.. plot::

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt

   MAR11060 = Load('MAR11060')

   fig, axes = plt.subplots(edgecolor='#ffffff', ncols=2, nrows=2, num='MAR11060-1', subplot_kw={'projection': 'mantid'})
   axes[0][0].plot(MAR11060, color='#1f77b4', label='MAR11060: spec 1', specNum=1)
   axes[0][0].set_xlabel('Time-of-flight ($\mu s$)')
   axes[0][0].set_ylabel('Counts ($\mu s$)$^{-1}$')
   axes[0][0].legend()   #.set_draggable(True) # uncomment to set the legend draggable

   axes[0][1].plot(MAR11060, color='#1f77b4', label='MAR11060: spec 2', specNum=2)
   axes[0][1].set_xlabel('Time-of-flight ($\mu s$)')
   axes[0][1].set_ylabel('Counts ($\mu s$)$^{-1}$')
   axes[0][1].legend()   #.set_draggable(True) # uncomment to set the legend draggable

   axes[1][0].plot(MAR11060, color='#1f77b4', label='MAR11060: spec 3', specNum=3)
   axes[1][0].set_xlabel('Time-of-flight ($\mu s$)')
   axes[1][0].set_ylabel('Counts ($\mu s$)$^{-1}$')
   axes[1][0].legend()   #.set_draggable(True) # uncomment to set the legend draggable

   axes[1][1].plot(MAR11060, color='#1f77b4', label='MAR11060: spec 4', specNum=4)
   axes[1][1].set_xlabel('Time-of-flight ($\mu s$)')
   axes[1][1].set_ylabel('Counts ($\mu s$)$^{-1}$')
   axes[1][1].legend()   #.set_draggable(True) # uncomment to set the legend draggable

   plt.show()

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

.. |FigureOptionsGear.png| image:: /images/FigureOptionsGear.png
   :width: 150px
.. |GenerateAScript.png| image:: /images/GenerateAScript.png
   :width: 30px
