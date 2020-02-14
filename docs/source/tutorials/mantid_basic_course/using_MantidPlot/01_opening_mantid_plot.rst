.. _01_Opening_Mantid_Plot:

========================
Opening Mantid Plot
========================

MantidPlot was built
on Qt4, but as Qt4 is no longer being supported, the Mantid Workbench was made using Matplotlib as the underlying tool for generating plots.

Currently Mantid Workbench does not have all the functionality of MantidPlot, but it is getting very close and Workbench will replace MantidPlot ~ 2021. 
At that stage MantidPlot will no longer be distributed. Currently all development work is on Workbench and we only solve minor bug fixes in Plot.

In this chapter we highlight some of the differences, as well as the features 
from MantidPlot that are not yet integrated into Workbench.   

Launching Mantid Plot
==========================

.. figure:: /images/mantid_folder.png
   :width: 700px
   :alt: Mantid applications in windows start menu

Currently Mantid Plot comes with any install of Mantid.

MantidPlot and the Mantid Workbench are visually different. Mantid Workbench has the Workspaces and Algorithms Toolboxes on the left, and MantidPlot has them on the right. Workbench has the script Editor in the centre and Plot has the grey empty space in the middle. These layout differences can be altered from their defaults (by dragging the Toolboxes).

.. figure:: /images/MantidPlot_example.png
   :width: 700px
   :alt: The example of MantidPlot

.. figure:: /images/MantidWorkbench_example.png
   :width: 700px
   :alt: The example of the Mantid Workbench

You can tell the difference by the name in the top left corner, and the icon on the taskbar.

Most of the basic functionality you have covered in Workbench is available to you in Plot, with a few extras in Plot. 
You still load workspaces as you did before but the workspaces right-click menu is different.

.. figure:: /images/Workbench_workspace_context_menu.png
   :width: 700px
   :alt: Context menus for MantidPlot and Workbench


