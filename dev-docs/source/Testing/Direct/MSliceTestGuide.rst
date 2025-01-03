.. _mslice_testing:
.. only:: html

  :math:`\renewcommand\AA{\mathring{A}}`

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

#. Ensure you have the `ISIS Sample Data <http://download.mantidproject.org>`__ available on your machine.
#. Open ``Interfaces`` > ``Direct`` > ``MSlice``
#. Go to the ``Data Loading`` tab and select ``MAR21335_Ei60meV.nxs`` from the sample data.
#. Click ``Load Data``
#. This should open the ``Workspace Manager`` tab with a workspace called ``MAR21335_Ei60meV``

Default Settings
----------------

#. In the ``Options`` menu, change ``Default Energy Units`` from ``meV`` to ``cm-1`` and ``Cut algorithm default`` from ``Rebin (Averages Counts)`` to ``Integration (Sum Counts)``.
#. The ``en`` setting on the ``Slice`` tab changes from ``meV`` to ``cm-1`` and the values in the row labelled ``y`` change.
#. Navigate to the ``Cut`` tab
#. Verify that ``en`` is set to ``cm-1`` and ``Cut Algorithm`` to ``Integration (Sum Counts)``
#. Change both settings back to their original values, ``Default Energy Units`` to ``meV`` and ``Cut algorithm default`` to ``Rebin (Averages Counts)``.

Taking Slices
-------------

1. Plotting a Slice
###################

#. In the ``Workspace Manager`` tab select the workspace ``MAR21335_Ei60meV``
#. Click ``Display`` in the ``Slice`` tab without changing the default values
#. On the slice plot, click ``Keep``

.. figure:: ../../../../docs/source/images/slice_plot.png
   :alt: slice_plot.png
   :align: center
   :width: 80%

2. Modifying a Slice
####################

#. Modify the slice settings in the ``Slice`` tab, for instance the values for x for ``from`` to ``1.5`` and ``to`` to ``5.5`` , and click ``Display``
#. A second slice plot should open with a plot reflecting your changes in the settings
#. The original slice plot should remain unchanged

.. figure:: ../../../../docs/source/images/modified_slice_plot.png
   :alt: modified_slice_plot.png
   :align: center
   :width: 80%

3. The Plots Tab
################

#. Navigate to the ``Plots`` tab of MSlice and check that there are entries for two plots
#. Open the ``Plots`` tab of Mantid and check that there are no entries for plots
#. Select one of the plots in the ``Plots`` tab of MSlice and click on ``Hide``, the corresponding plot should disappear
#. Now click on ``Show`` for this plot and it should re-appear again
#. Double-click on elements of the original slice plot and modify settings, for instance the plot itself and the colorbar axes
#. Change the plot title and the y axis label to LaTeX, for instance ``$\mathrm{\AA}^{-1}$``, and ensure the text is displayed correctly (for ``$\mathrm{\AA}^{-1}$`` it should be :math:`\mathrm{\AA}^{-1}`)
#. Ensure that the slice plot changes accordingly
#. Click ``Make Current`` on the original slice plot
#. Modify the slice settings in the ``Slice`` tab again and click ``Display``
#. This time the new slice plot overwrites the original slice plot

4. Overplot Recoil Lines and Bragg Peaks
########################################

#. Navigate to the ``Information`` menu on the slice plot
#. Select ``Hydrogen`` from the submenu for ``Recoil lines``. A blue line should appear on the slice plot.
#. Select two or three materials from the submenu for ``Bragg peaks`` and ensure that Bragg peaks in different colours per material are plotted on the slice plot.
#. Make sure that when deselecting one of the materials only the respective Bragg peaks are removed from the slice plot but the ones still selected remain.

.. figure:: ../../../../docs/source/images/recoil_line_bragg_peaks.png
   :alt: recoil_line_bragg_peaks.png
   :align: center
   :width: 80%

5. The Plot Toolbar
#####################

#. In the plot window, check that the following buttons are working as expected: Zoom in, Zoom out, ``Legends`` (add a recoil line to display a legend first), Save, Copy, Print and Plot Options. Modify plot options and make sure that the plot changes accordingly.


6. Generate a Script
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

2. Changing the intensity of a Cut
##################################
#. Navigate to the ``Intensity`` menu on the cut plot
#. Select ``Chi''(Q,E)`` and set a value of ``100``
#. The y axis of the cut plot should change to a higher maximal value

.. figure:: ../../../../docs/source/images/cut_q_chi.png
   :alt: cut_q_chi.png
   :align: center
   :width: 80%

3. Modifying a Cut
##################

#. Check that the menu item ``Recoil lines`` is disabled within the menu item ``Information``.
#. Modify the step size on the ``Cut`` tab to ``0.02`` and click ``Plot Over``. A second cut should appear on the cut plot in a different colour.
#. Click on Plot Options on the cut plot and modify settings
#. Ensure that the cut plot changes accordingly
#. Click on Save to Workbench on the ``Cut`` tab and check that in Mantid a workspace with the name ``MAR21335_Ei60meV_cut(-5.000,5.000)`` appears
#. In the row labelled ``over``, set the ``from`` value to ``-1`` and the ``to`` value to ``1`` and click ``Plot``
#. Navigate to the tab ``MD Histo`` tab and check that there are at least two entries, ``MAR21335_Ei60meV_cut(-5.000,5.000)`` and ``MAR21335_Ei60meV_cut(-1.000,1.000)``. Please note that there might be more entries from the previous tests.
#. Select ``MAR21335_Ei60meV_cut(-1.000,1.000)`` and click ``Save to Workbench``
#. Check that in Mantid a workspace with the name ``MAR21335_Ei60meV_cut(-1.000,1.000)`` appears
#. Navigate to the ``Cut`` tab
#. In the row labelled ``along``, select ``DeltaE``
#. In the row labelled ``over``, select ``2Theta``
#. In the row labelled ``along``, set the ``from`` value to ``-5`` and the ``to`` value to ``5``
#. In the row labelled ``over``, set the ``from`` value to ``30`` and the ``to`` value to ``60``
#. Click ``Plot``

.. figure:: ../../../../docs/source/images/cut_plot.png
   :alt: cut_plot.png
   :align: center
   :width: 80%

4. Interactive Cuts
###################

#. Navigate to the ``Slice`` tab of the ``Workspace Manager`` tab
#. Click ``Display`` in the ``Slice`` tab without changing the default values
#. On the slice plot, select ``Interactive Cuts``
#. Use the cursor to select a rectangular region in the slice plot. A second window with a cut plot should open.
#. Check that the menu item ``Intensity`` is disabled as well as the item ``Recoil lines`` within the menu item ``Information`` in the new plot window
#. Check that the ``File`` menu only has one menu item, ``Close``
#. Change the rectangle by changing its size or dragging it to a different area of the slice plot. The cut plot should update accordingly.
#. Click on ``Save Cut to Workspace`` and check the ``MD Histo`` tab of the Workspace Manager to verify that the new workspace was added
#. Click on Flip Integration Axis. The y axis label changes from ``Energy Transfer (meV)`` to :math:`|Q| (\mathrm{\AA}^{-1})` or vice versa, depending on the initial label.


.. figure:: ../../../../docs/source/images/flip_integration_axis.png
   :alt: flip_integration_axis.png
   :align: center
   :width: 10%

.. figure:: ../../../../docs/source/images/interactive_cuts.png
   :alt: interactive_cuts.png
   :align: center
   :width: 80%

5. Overplot Bragg Peaks
#######################

#. Navigate to the ``Information`` menu on the cut plot
#. Select ``Aluminium`` from the submenu for ``Bragg peaks``. Green lines should appear on the cut plot with a respective legend entry.
#. Deselect ``Aluminium`` form the submenu for ``Bragg peaks``. Both green lines and the respective legend entry should disappear.

.. figure:: ../../../../docs/source/images/cut_with_bragg_peaks.png
   :alt: cut_with_bragg_peaks.png
   :align: center
   :width: 80%

6. Generate a Script
####################

#. Navigate to the ``Cut`` tab
#. In the row labelled ``along``, select ``|Q|`` and set the ``from`` value to ``0`` and the ``to`` value to ``10``
#. In the row labelled ``over``, set the ``from`` value to ``-5`` and the ``to`` value to ``5``
#. Click ``Plot``. A new window with a cut plot should open.
#. Navigate to the ``Information`` menu on the cut plot
#. Select ``Aluminium`` from the submenu for ``Bragg peaks``. Green lines should appear on the cut plot with a respective legend entry.
#. Navigate to the ``File`` menu on a cut plot. Please note that this needs to be a cut plot created via the ``Cut`` tab and not an interactive cut.
#. Select ``Generate Script to Clipboard`` and paste the script into the Mantid editor. Please note that on Linux ``Ctrl + V`` might not work as expected. Use ``shift insert`` instead in this case.
#. Run the script and check that the same cut plot is displayed

7. Waterfall Plots
##################

#. Navigate to the ``Cut`` tab
#. In the row labelled ``along``, set the ``from`` value to ``0`` and the ``to`` value to ``10``
#. In the row labelled ``over``, set the ``from`` value to ``-5`` and the ``to`` value to ``5`` as well as the ``width`` value to ``2``
#. Click ``Plot``. A new window with a cut plot should open.
#. Click ``Waterfall`` and set the ``x`` value to ``0.5``, then hit enter. The cuts are now plotted with a ``0.5`` offset in direction of the x axis.
#. Set the ``y`` value to ``2`` and hit enter. The cuts are now plotted with an additional offset (``2``) in direction of the y axis.

.. figure:: ../../../../docs/source/images/waterfall_cut_plot.png
   :alt: waterfall_cut_plot.png
   :align: center
   :width: 80%

The Command Line Interface
--------------------------

1. Use the Mantid Editor
########################

#. Close all plots currently open but not the MSlice interface
#. Copy the following code into the Mantid editor. You might have to modify the file path for the Load command to the correct location of ``MAR21335_Ei60meV.nxs``.

.. code:: python

    import mslice.cli as mc

    ws = mc.Load('C:\\MAR21335_Ei60meV.nxs')
    wsq = mc.Cut(ws, '|Q|', 'DeltaE, -1, 1')
    mc.PlotCut(wsq)

    ws2d = mc.Slice(ws, '|Q|, 0, 10, 0.01', 'DeltaE, -5, 55, 0.5')
    mc.PlotSlice(ws2d)

2. Run an Example Script
########################

#. Run the script.
#. There should be two new windows with a slice plot and a cut plot

.. figure:: ../../../../docs/source/images/output_mslice_script.png
   :alt: output_mslice_script.png
   :align: center
   :width: 80%

3. Use the Jupyter QtConsole
############################

#. Repeat the same test by copying the script into the Jupyter QtConsole of the MSlice interface

.. figure:: ../../../../docs/source/images/mslice_jupyter_qtconsole.png
   :alt: mslice_jupyter_qtconsole.png
   :align: center
   :width: 80%

4. Run Another Example Script in the Mantid Editor
##################################################

#. Select the ``MAR21335_Ei60meV`` workspace in the ``Workspace Manager``, click ``Compose`` and then ``Scale``
#. Enter a scale factor of 1.0 and click ``Ok``
#. Select the ``MAR21335_Ei60meV`` workspace again and click ``Subtract``
#. Select the ``MAR21335_Ei60meV_scaled`` workspace and leave the self-shielding factor as 1.0, then click ``Ok``
#. Select the ``MAR21335_Ei60meV_subtracted`` workspace and click ``Display`` in the ``Slice`` tab
#. Verify that all values are zeros
#. Navigate to the ``File`` menu on the slice plot, select ``Generate Script to Clipboard`` and paste the script into the Mantid editor
#. Close the slice plot with all zeros
#. Run the script in the Mantid editor and verify that a slice plot with all zeros is reproduced


The Workspace Manager
---------------------

1. Check Scale and Subtract
###########################

#. Select the ``MAR21335_Ei60meV`` workspace in the ``Workspace Manager``, click on ``Save`` and select ``ASCII``
#. A file dialog opens and allows entering a name for saving the file
#. Verify that a txt file with the selected name has been created and contains ASCII data (the first line should be ``# X , Y , E``)
#. Select the ``MAR21335_Ei60meV`` workspace again, click on ``Rename`` and rename the workspace
#. In the ``Slice`` tab of the renamed workspace click on ``Display`` and verify that the original slice plot is displayed
#. Select the renamed workspace and click on ``Delete``
#. The renamed workspace should disappear and the ``Workspace Manager`` should be empty
#. Go to the ``Data Loading`` tab and select ``MAR21335_Ei60meV.nxs`` from the sample data.
#. Click ``Load Data``
#. Select the ``MAR21335_Ei60meV`` workspace again, click on ``Compose``, select ``Scale`` and enter a scale factor of 2, then click ``Ok``
#. A new workspace with the name ``MAR21335_Ei60meV_scaled`` appears
#. Select the ``MAR21335_Ei60meV_scaled`` workspace and click on ``Subtract``. Select ``MAR21335_Ei60meV`` in the dialog that opens and click ``Ok``.
#. A new workspace with the name ``MAR21335_Ei60meV_scaled_subtracted`` appears
#. In the ``Workspace Manager`` tab select the workspace ``MAR21335_Ei60meV``
#. Navigate to the ``Cut`` tab
#. In the row labelled ``along``, set the ``from`` value to ``0`` and the ``to`` value to ``10``
#. In the row labelled ``over``, set the ``from`` value to ``-5`` and the ``to`` value to ``5``
#. Click ``Plot``.
#. Follow the same steps for the workspaces ``MAR21335_Ei60meV_scaled`` and ``MAR21335_Ei60meV_scaled_subtracted`` but click ``Plot Over`` for these two
#. The cut plot window should now contain three differently coloured lines with corresponding legends. The line for ``MAR21335_Ei60meV`` will be exactly covered by the line for ``MAR21335_Ei60meV_scaled_subtracted``. The line for ``MAR21335_Ei60meV_scaled`` will be scaled by factor ``2.0``.

.. figure:: ../../../../docs/source/images/compare_mslice_ws.png
   :alt: compare_mslice_ws.png
   :align: center
   :width: 80%

2. Check Delete and Sum
#######################

#. Delete all workspaces apart from the ``MAR21335_Ei60meV`` workspace
#. Scale the ``MAR21335_Ei60meV`` workspace with a factor of ``1.0``
#. A new workspace with the name ``MAR21335_Ei60meV_scaled`` appears
#. Select the ``MAR21335_Ei60meV`` workspace again, click on ``Add`` and select ``MAR21335_Ei60meV_scaled``, then click ``Ok``
#. A new workspace with the name ``MAR21335_Ei60meV_sum`` appears
#. Delete the workspace with the name ``MAR21335_Ei60meV_scaled``
#. Scale the ``MAR21335_Ei60meV`` workspace with a factor of ``2.0``
#. A new workspace with the name ``MAR21335_Ei60meV_scaled`` appears
#. In the ``Workspace Manager`` tab select the workspace ``MAR21335_Ei60meV_scaled``
#. Navigate to the ``Cut`` tab
#. In the row labelled ``along``, set the ``from`` value to ``0`` and the ``to`` value to ``10``
#. In the row labelled ``over``, set the ``from`` value to ``-5`` and the ``to`` value to ``5``
#. Click ``Plot``.
#. Follow the same steps for the workspace ``MAR21335_Ei60meV_sum`` but click ``Plot Over``
#. There should be two differently coloured lines with corresponding legends that match exactly

.. figure:: ../../../../docs/source/images/compare_mslice_ws_2.png
   :alt: compare_mslice_ws_2.png
   :align: center
   :width: 80%


Interaction with ADS
--------------------
#. Create a few (maybe three or four) interactive cuts from a slice plot and click ``Save Cut to Workspace`` for each of them
#. Navigate to the ``MD Histo`` tab of the Workspace Manager and select all cuts
#. Click on ``Save to Workbench``
#. In Mantid, all selected cuts from the ``MD Histo`` tab are now visible in the Workspaces window
#. Select one of these and rename it
#. Check that the corresponding workspace in the ``MD Histo`` has been renamed accordingly
#. Select the renamed workspace in Mantid and click ``Delete``
#. Check that the corresponding workspace in the ``MD Histo`` tab is deleted as well
#. Now click ``Clear`` in the Workspaces window in Mantid and check that all workspace in the ``2D`` and ``MD Histo`` tabs in MSlice are deleted as well
