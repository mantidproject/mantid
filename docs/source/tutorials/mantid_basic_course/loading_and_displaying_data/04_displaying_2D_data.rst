.. _04_displaying_2D_data:

==================
Displaying 2D Data 
==================

Plotting All Spectra
====================

We have previously seen how to plot one or more rows from a dataset.
Here we will show how to visually inspect entire datasets.

#. Reload the MAR11060 dataset, but this time with no SpectraMin/Max limits.
#. Right click the workspace in the workspace list and select "Plot > 
   Colourfill". This will create a fairly boring purple display of the
   data, because one spectrum contains much higher counts than any of
   the other spectra, and the color scale has adjusted accordingly.
#. To make the colour fill plot more sensitive to its smaller features,
   open the Plot Options Menu by clicking on the Gear Icon. Click on the Images Tab of this menu and adjust the Scale to "Logarithmic" and click "Apply". (Note you could have changed the scale by right-clciking on the plot image and setting "Color bar" to "Log")
#. While you are here feel free to change the other Figure options to see what is possible! I personally like to change the Colormap to "twilight".

.. figure:: /images/ColourFillPlotMar11060.png
   :alt: ColourFillPlotMar11060
   :align: center

If you would like to create your own colormap, there is `more information here <`https://docs.mantidproject.org/nightly/plotting/index.html#custom-colormap-mantidworkbench>`_.


Slice Viewer
===============

This is a useful tool to investigate 2D image data. You
can rapidly look through the spectrum and bin data for any point in the
2D map. This is one of the features of Mantid we are currently developing and will gain more functionality in the future.

#. Launch the Spectrum Viewer just right-click on the MAR11060
   Workspace and select 'Show Slice Viewer'.
#. Change the scale of the colorbar in the drop-down menu to SymmetricLog10.
#. Toggle the Lineplots on with the graphical menu button.
#. Change the Colorbar Maximum to 200
#. Move your cursor around the plot to see the corresponding lineplots

.. figure:: /images/600px-ImageViewer.png
   :alt: Slice Viewer
   :align: center

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Displaying_data|Mantid_Basic_Course|MBC_Displaying_data_in_multiple_workspaces}}
