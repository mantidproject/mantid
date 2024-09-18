.. _01_basic_plot_scripting:

=========================
Basic 1D, 2D and 3D Plots
=========================


plotSpectrum and plotBin
========================

Right-clicking a workspace from the Workspaces Toolbox and selecting **Show Data** creates a Data Matrix, which displays the underlying data in a worksheet.

.. To do this from Python use the `importMatrixWorkspace("workspace-name")` function, giving the name of the workspace to import, e.g.

.. figure:: /images/Mantid_plotSpectrum.png
   :alt: Mantid_plotSpectrum
   :align: center


Right clicking on a row or a column allows a plot of that spectrum or time bin, which can also be achieved in Python using the `plotSpectrum()` or `plotBin()` functions, e.g.

.. code-block:: python

    RawData = Load("MAR11015")

    graph_spec = plotSpectrum(RawData, 0)
    graph_time = plotBin(RawData, 0)

To include error bars on a plot, there is an extra argument that can be set to 'True' to enable them, e.g.

.. code-block:: python

    graph_spec = plotSpectrum(RawData, 0, error_bars=True)
    graph_time = plotBin(RawData, 0, error_bars=True)

For setting scales, axis titles, plot titles etc. you can use:

.. code-block:: python

    RawData = Load("MAR11015")
    plotSpectrum(RawData,1, error_bars=True)

    # Get current axes of the graph
    fig, axes = plt.gcf(), plt.gca()

    plt.yscale('log')

    # Rescale the axis limits
    axes.set_xlim(0,5000)
    axes.set_ylim(0.001,1500)

    #C hange the y-axis label
    axes.set_ylabel(r'Counts ($\mu s$)$^{-1}$')

    # Add legend entries
    axes.legend(['Good Line'])

    # Give the graph a modest title
    plt.title("My Wonderful Plot", fontsize=20)

Multiple workspaces/spectra can be plotted by providing lists within the `plotSpectrum()` or `plotBin()` functions, e.g.

.. code-block:: python

    # Plot multiple spectra from a single workspace
    plotSpectrum(RawData, [0,1,3])

    # Here you need to load files into RawData1 and RawData2 before the next two commands

    # Plot a spectrum from different workspaces
    plotSpectrum([RawData1,RawData2], 0)

    # Plot multiple spectra across multiple workspaces
    plotSpectrum([RawData1,RawData2], [0,1,3])

To overplot on the same window:

.. code-block:: python

    RawData = Load("MAR11015")

    # Assign original plot to a window called graph_spce
    graph_spec = plotSpectrum(RawData, 0)

    # Overplot on that window, without clearing it
    plotSpectrum(RawData, 1, window=graph_spec, clearWindow=False)


2D Colourfill and Contour Plots
===============================

2D plots can be produced as an `image <https://matplotlib.org/3.2.1/api/_as_gen/matplotlib.pyplot.imshow.html>`_ or a `pseudocolormesh <https://matplotlib.org/3.2.1/api/_as_gen/matplotlib.pyplot.pcolormesh.html>`_ (for a non-regular grid):

.. plot::
   :include-source:

    ''' ----------- Image > imshow() ----------- '''

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    from matplotlib.colors import LogNorm

    data = Load('MAR11060')

    fig, axes = plt.subplots(subplot_kw={'projection':'mantid'})

    # IMPORTANT to set origin to lower
    c = axes.imshow(data, origin = 'lower', cmap='viridis', aspect='auto', norm=LogNorm())
    cbar=fig.colorbar(c)
    cbar.set_label('Counts ($\mu s$)$^{-1}$') #add text to colorbar
    #fig.show()


.. plot::
   :include-source:

    ''' ----------- Pseudocolormesh > pcolormesh() ----------- '''

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    from matplotlib.colors import LogNorm

    data = Load('CNCS_7860')
    data = ConvertUnits(InputWorkspace=data,Target='DeltaE', EMode='Direct', EFixed=3)
    data = Rebin(InputWorkspace=data, Params='-3,0.025,3', PreserveEvents=False)
    md = ConvertToMD(InputWorkspace=data,QDimensions='|Q|',dEAnalysisMode='Direct')
    sqw = BinMD(InputWorkspace=md,AlignedDim0='|Q|,0,3,100',AlignedDim1='DeltaE,-3,3,100')

    fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
    c = ax.pcolormesh(sqw, cmap='plasma', norm=LogNorm())
    cbar=fig.colorbar(c)
    cbar.set_label('Intensity (arb. units)') #add text to colorbar
    #fig.show()

`Contour lines <https://matplotlib.org/api/_as_gen/matplotlib.axes.Axes.contour.html>`_ can be overlayed on a 2D colorfill:

.. plot::
   :include-source:

    ''' ----------- Contour overlay ----------- '''

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    data = Load('SANSLOQCan2D.nxs')

    fig, axes = plt.subplots(subplot_kw={'projection':'mantid'})

    # IMPORTANT to set origin to lower
    c = axes.imshow(data, origin = 'lower', cmap='viridis', aspect='auto')

    # Overlay contours
    axes.contour(data, levels=np.linspace(10, 60, 6), colors='yellow', alpha=0.5)

    cbar=fig.colorbar(c)
    cbar.set_label('Counts ($\mu s$)$^{-1}$') #add text to colorbar
    #fig.show()


3D Surface and Wireframe Plots
==============================

`3D plots <https://matplotlib.org/mpl_toolkits/mplot3d/tutorial.html>`_ `Surface <https://matplotlib.org/mpl_toolkits/mplot3d/tutorial.html#surface-plots>`_ and `Wireframe <https://matplotlib.org/mpl_toolkits/mplot3d/tutorial.html#wireframe-plots>`_ plots can also be created:

.. plot::
   :include-source:

    ''' ----------- Surface plot ----------- '''

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt

    data = Load('MUSR00015189.nxs')
    data = mtd['data_1'] # Extract individual workspace from group

    fig, ax = plt.subplots(subplot_kw={'projection':'mantid3d'})
    ax.plot_surface(data, cmap='viridis')
    #fig.show()

.. plot::
   :include-source:

    ''' ----------- Wireframe plot ----------- '''

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt

    # This file can be found in the Usage Examples folder, available
    # here https://www.mantidproject.org/installation/index#sample-data
    data = Load('PG3_733.nxs')

    fig, ax = plt.subplots(subplot_kw={'projection':'mantid3d'})
    ax.plot_wireframe(data, color='green')
    #fig.show()


* See :ref:`here <plotting>` for custom color cycles and colormaps
