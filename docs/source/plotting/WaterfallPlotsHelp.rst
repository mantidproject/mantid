.. _Waterfall_Plots:

===============
Waterfall Plots
===============

.. TO UPDATE find these images in a .pptx file at https://github.com/mantidproject/documents/blob/master/Images/Images_for_Docs/formatting_plots.pptx

|
|

**Other Plot Types**

* :ref:`Basic_1D_Plots`
* :ref:`Colorfill_Plots`
* :ref:`3D_Plots`
* :ref:`Mesh_Plots`

**General Plot Help**

* :ref:`06_formatting_plots`
* :ref:`02_scripting_plots`

|
|

.. contents:: Waterfall Plots - Table of contents
    :local:

|
|

To create a waterfall plot, right-click on a workspace and select ``Plot > Spectrum...``.
Then set the ``Plot Type`` to ``Waterfall`` before choosing which spectra to plot and selecting ``OK``.

|
|

Plot Toolbar
============

.. figure:: /images/PlotToolbarWaterfall.png
   :alt: Plot Toolbar Waterfall
   :align: center

|
|

From the waterfall toolbar, access the Offset and Fill Area menus:

.. figure:: /images/PlotToolbarWaterfallMenus.png
   :alt: Waterfall Toolbar Menus
   :align: center
   :width: 700px

|
|

Click Menus
-----------

.. figure:: /images/PlotClickMenusWaterfall.png
   :alt: Click Menus Waterfall
   :align: center
   :width: 1500px

|
|

|O| ptions Menu
-----------------------------------

Waterfall plots are very similar to :ref:`1D plots<Basic_1D_Plots>` of multiple spectra. They are simply offset, as controlled from the Waterfall Toolbar menus above.

.. figure:: /images/PlotOptions.png
   :alt: Plot Options Axes Legend
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

An example script for a Waterfall Plot:

.. plot::
   :include-source:

   # import mantid algorithms and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt

   from mantid.api import AnalysisDataService as ADS

   # Load data and choose 1st ws from group
   data = Load('MUSR00015189')
   data_ws = ADS.retrieve('data_1')

   # Get figure and axes with mantid projection
   fig, ax = plt.subplots(subplot_kw={'projection': 'mantid'})

   # Define colors and labels for desired spectra
   colors = ('red', 'green', 'darksalmon', 'navy', '#AB0EA2')
   labels = ('MUSR15189_1 Sp1','MUSR15189_1 Sp2', 'MUSR15189_1 Sp3', 'MUSR15189_1 Sp4', 'MUSR15189_1 Sp5')

   # Plot 5 spectra in a loop
   for i in range(5):
       ax.plot(data_ws, color=colors[i], label=labels[i], specNum=(i+1), linewidth = 4)

   # Add title and tidy the x-axis range
   plt.title('Waterfall MUSR00015189_1 Spec1-5')
   ax.set_xlim(-2,35)

   '''---- Convert from 1D plot to Waterfall plot ----'''
   # Can set the x/y offset and choose to fill under the curves
   ax.set_waterfall(True, x_offset=50, fill=True)

   # Separately turn on the area fill, and choose one color for all
   ax.set_waterfall_fill(True, colour="#00d1ff")

   # Update the offsets
   ax.update_waterfall(x_offset=10, y_offset=30)

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
