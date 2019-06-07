.. _02_plotting_in_workbench:

=====================
Plotting in Workbench
=====================

A new feature of Workbench is the Plots Toolbox, with this you can manage several plots at once from one window. to switch
to the Plots Toolbox click on the Plots tab in the bottom left had corner, this will switch out the Algorithms Toolbox.

.. figure:: /images/Workbench_algorithms_plots_tab.png
   :width: 700px
   :alt: click the plots tab to switch to the plots toolbox

The Plots Toolbox has a number of functions to help manage your plots:

* Show/Hide: Toggles the visibility of the selected plot, the same function can accessed by clicking the eye button next to the plot name.
* Delete: Closes the selected plot from the Toolbox, this is the same as closing the plot window or clicking the X on a plot in the Toolbox list.
* Select All: Selects all plots currently in the Toolbox
* Sort: Allows for plots in the Toolbox to be sorted in order of name, number or last opened.
* Export: Exports the selected plot as an .EPS, .PDF, .PNG or .SVG fromat.
* Rename: By clicking on the symbol of a square and a pencil (between the eye and the X) you can rename the selected plot, names default to "workspace"-"number"

.. figure:: /images/Workbench_Plots_Toolbox.png
   :width: 700px
   :alt: click the plots tab to switch to the plots toolbox
   
Matplotlib
==========

Workbench uses the Python Package `Matplotlib` to build it's graphs instad of `Qwt` which was used in MantidPlot. As such the plot window itself looks somewhat different.
Along with your plot you now have a taskbar with a number of options:

1. Reset view/Pan and zoom with mouse/Zoom to rectangle.
2. Toggle grid lines on and off.
3. Export to file or Print figure. (Print not yet implemented)
4. Configure plot options.
5. Open fit browser tool.

.. figure:: /images/workbench_plot_window.png
   :width: 700px
   :alt: The Workbench plot window
   
The plot can be modified from the plot options menu, from here the axis labels and titles can be changed along with the scale and range of the plot axes. 
You can also change the line specs and labels of indevidual curves including the colour by giving an 8 digit RGBA hex code (red, green, blue, alpha) or use the built in 
widget by clicking on the square button next to the text box.

.. figure:: /images/workbench_plot_options.png
   :width: 700px
   :alt: The Workbench plot options window

Currently the Workbench user interface is unable to do everything MantidPlot was able to, as it does not support 3D plots from the user interface (though you can generate them with python scripts) and currently lacks the spectrum viewer among others. 

To get the full use out of Matplotlib you can work from the IPython prompt or by writing script in the Workbench main window. If you have not used Matplotlib before it might 
help to consult the online `Matplotlib tutorial <https://matplotlib.org/tutorials/introductory/pyplot.html>`_.
   
