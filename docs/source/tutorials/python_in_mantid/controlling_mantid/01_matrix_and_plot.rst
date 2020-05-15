.. _01_matrix_and_plot:

=======================
Matrix and Plot Control
=======================


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
    graph_time = plotBin(RawData, 0)

 For setting scales, axis titles, plot titles etc. you can use:

.. code-block:: python

    RawData = Load("MAR11015")
    plotSpectrum(RawData,1, error_bars=True)

    # Get current axes of the graph
    fig, axes = plt.gcf(), plt.gca()

    plt.yscale('log')

    #Rescale the axis limits
    axes.set_xlim(0,5000)

    #Change the y-axis label
    axes.set_ylabel('Counts ($\mu s$)$^{-1}$')

    #Add legend entries
    axes.legend(['Good Line'])

    #Give the graph a modest title
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

2D plots can be produced as an `image <https://matplotlib.org/3.2.1/api/_as_gen/matplotlib.pyplot.imshow.html>`_ or a `pseudocolormesh <https://matplotlib.org/3.2.1/api/_as_gen/matplotlib.pyplot.pcolormesh.html>`_ (for a non-regular grid):

.. plot::
   :include-source:

    ''' ----------- Image > imshow() ----------- '''

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    from matplotlib.colors import LogNorm

    data = Load('MAR11060')

    fig, axes = plt.subplots(subplot_kw={'projection':'mantid'})
    c = axes.imshow(data, cmap='twilight_r', aspect='auto', norm=LogNorm())
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
    c = ax.pcolormesh(sqw, cmap='afmhot', norm=LogNorm())
    cbar=fig.colorbar(c)
    cbar.set_label('Intensity (arb. units)') #add text to colorbar
    #fig.show()


`3D plots <https://matplotlib.org/mpl_toolkits/mplot3d/tutorial.html>`_ `Surface <https://matplotlib.org/mpl_toolkits/mplot3d/tutorial.html#surface-plots>`_ and `Contour <https://matplotlib.org/mpl_toolkits/mplot3d/tutorial.html#contour-plots>`_ plots can also be created:

.. plot::
   :include-source:

    ''' ----------- Surface plot ----------- '''

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt

    data = Load('MUSR00015189.nxs')
    data = mtd['data_1']

    fig, ax = plt.subplots(subplot_kw={'projection':'mantid3d'})
    ax.plot_surface(data)
    #fig.show()

.. plot::
   :include-source:

    ''' ----------- Contour plot ----------- '''
    
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt

    data = Load('MUSR00015189.nxs')
    data = mtd['data_1']

    fig, ax = plt.subplots(subplot_kw={'projection':'mantid3d'})
    ax.contour(data, cmap ='summer')
    #fig.show()


* See :ref:`here <plotting>` for custom color cycles and colormaps 
