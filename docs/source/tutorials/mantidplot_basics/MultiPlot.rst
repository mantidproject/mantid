.. _MultiPlotting:

==============
Multi Plotting
==============

There are several ways to visualize multiple 1D spectra together (overplotting or tiling).

Plotting multiple curves
------------------------

* Right click on the workspace name, click **Plot Spectrum** or **Plot Spectrum With Errors**, type in the list of workspace indices (e.g. `1-5`). You can choose to plot:
    - 1D: this will simply overplot all the spectra on the same x-axis
    - Waterfall: this will visually offset the axes to give the waterfall effect

    .. image:: /images/Training/MantidPlotBasics/Waterfall.png
      :alt: Waterfall plot
      :align: center

    - Tiled: this will plot each 1D spectrum individually on a separate canvas, and tile the canvases in one layer.

    .. image:: /images/Training/MantidPlotBasics/Tile.png
      :alt: Tiled plot
      :align: center

* In the matrix view, select multiple rows by dragging the mouse over the leftmost column of the desired spectra. Then right click and you will have three options in the context menu:
    - Plot spectra (values only)
    - Plot spectra (values and errors)
    - Waterfall plot

Adding a new curve on an existing plot
--------------------------------------

* If you have a plot already drawn you can drag and drop the workspace into the plot. You will be prompted to type the workspace indices and the plot type. By clicking OK it will overplot on the exsisting curve.

* If you have multiple plots drawn in individual windows, you can right click on one of them, and then choose **Add/Remove Curve**. This will open a dialog, where you can choose which curves to overplot.
