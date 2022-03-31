.. _mslice_testing:

MSlice Testing
===================

.. contents::
   :local:

Introduction
------------
MSlice is a tool for visualizing cuts and slices of inelastic neutron scattering data. This version uses Mantid to process the data and plots it using matplotlib. It includes both a GUI and a commandline interface with a script generator.

See here for the current MSlice documentation: http://mantidproject.github.io/mslice

Set Up
------

#. Open ``Interfaces`` > ``Direct`` > ``MSlice``
#. Go to the ``Data Loading`` tab and select ``MAR21335_Ei60meV.nxs`` from the Mantid unit test files
#. Click ``Load Data``
#. This should open the ``Workspace Manager`` tab with a workspace called ``MAR21335_Ei60meV``

Taking Slices
-------------

1. Plotting a Slice
###################

#. In the ``Workspace Manager`` tab select the workspace ``MAR21335_Ei60meV``
#. Click ``Display`` in the ``Slice`` tab without changing the default values
#. On the slice plot, click ``Keep``
#. Modify the slice settings in the ``Slice`` tab, for instance the values for ``from`` and ``to``, and click ``Display``
#. A second slice plot should open with a plot reflecting your changes in the settings
#. The original slice plot should remain unchanged
#. Double-click on elements of the original slice plot and modify settings
#. Change the title and the y axis label to LaTeX, for instance ``$\mathrm{\AA}^{-1}$``, and ensure the text is displayed correctly (for ``$\mathrm{\AA}^{-1}$`` it should be :math:`\mathrm{\AA}^{-1}`)
#. Ensure that the slice plot changes accordingly
#. Click ``Make Current`` on the original slice plot
#. Modify the slice settings in the ``Slice`` tab again and click ``Display``
#. This time the new slice plot overwrites the original slice plot

2. Overplot recoil lines and Bragg peaks
########################################

#. Navigate to the ``Information`` menu on the slice plot
#. Select ``Hydrogen`` from the submenu for ``Recoil lines``. A blue line should appear on the slice plot.
#. Select two or three materials from the submenu for ``Bragg peaks`` and ensure that Bragg peaks in different colours per material are plotted on the slice plot.
#. Make sure that when deselecting one of the materials only the respective Bragg peaks are removed from the slice plot but the ones still selected remain.

3. Check plot toolbar
#####################

#. In the plot window, check that the following buttons are working as expected: Zoom in, Zoom out, Legends (add a recoil line to display a legend first), Save, Copy, Print and Plot Options. Modify plot options and make sure that the plot changes accordingly.


4. Generate a script
####################

#. Navigate to the ``File`` menu on the slice plot
#. Select ``Generate Script to Clipboard`` and paste the script into the Mantid editor. Please note that on Linux ``Ctrl + V`` might not work as expected. Use ``shift insert`` instead in this case.
#. Run the script and check that the same slice plot is displayed

Taking Cuts
-----------

1. Plotting a Cut
#################

#. In the ``Workspace Manager`` tab select the workspace ``MAR21335_Ei60meV``
#. Navigate to the ``Cut`` tab
#. In the row labelled ``along``, set the ``from`` value to ``0`` and the ``to`` value to ``10``
#. In the row labelled ``over``, set the ``from`` value to ``-5`` and the ``to`` value to ``5``
#. Click ``Plot``. A new window with a cut plot should open.

.. figure:: ../../../../docs/source/images/cut_q.png
   :alt: cut_q.png
   :align: center
   :width: 80%

#. Check that the menu item 'Intensity' is disabled as well as the item 'Recoil lines' within the menu item 'Information'.
#. On the cut plot, click ``Keep``
#. Modify the step size on the ``Cut`` tab and click ``Plot Over``. A second cut should appear on the cut plot in a different colour.
#. Click on ``Plot Options`` on the cut plot and modify settings
#. Ensure that the cut plot changes accordingly

2. Interactive Cuts
###################

#. Navigate to the ``Slice`` tab of the ``Workspace Manager`` tab
#. Click ``Display`` in the ``Slice`` tab without changing the default values
#. On the slice plot, select ``Interactive Cuts``
#. Use the cursor to select a rectangular region in the slice plot. A second window with a cut plot should open.
#. Check that the menu item 'Intensity' is disabled as well as the item 'Recoil lines' within the menu item 'Information' in the new plot window
#. Check that the 'File' menu only has one menu item, 'Close'
#. Change the rectangle by changing its size or dragging it to a different area of the slice plot. The cut plot should update accordingly.

.. figure:: ../../../../docs/source/images/interactive_cuts.png
   :alt: interactive_cuts.png
   :align: center
   :width: 80%

3. Overplot Bragg peaks
#######################

#. Navigate to the ``Information`` menu on the cut plot
#. Select ``Aluminium`` from the submenu for ``Bragg peaks``. Green lines should appear on the cut plot with a respective legend entry.
#. Deselect ``Aluminium`` form the submenu for ``Bragg peaks``. Both green lines and the respective legend entry should disappear.

4. Generate a script
####################

#. Navigate to the ``File`` menu on the cut plot
#. Select ``Generate Script to Clipboard`` and paste the script into the Mantid editor. Please note that on Linux ``Ctrl + V`` might not work as expected. Use ``shift insert`` instead in this case.
#. Run the script and check that the same cut plot is displayed

5. Waterfall plots
##################

#. Navigate to the ``Cut`` tab
#. In the row labelled ``along``, set the ``from`` value to ``0`` and the ``to`` value to ``10``
#. In the row labelled ``over``, set the ``from`` value to ``-5`` and the ``to`` value to ``5`` as well as the ``width`` value to ``2``
#. Click ``Plot``. A new window with a cut plot should open.
#. Click ``Waterfall`` and set the ``x`` value to ``0.5``, then hit enter. The cuts are now plotted with a ``0.5`` offset in direction of the x axis.
#. Set the ``y`` value to ``2`` and hit enter. The cuts are now plotted with an additional offset (``2``) in direction of the y axis.

.. figure:: ../../../../docs/source/images/waterfall_cut_plot.png
   :alt: interactive_cuts.png
   :align: center
   :width: 80%