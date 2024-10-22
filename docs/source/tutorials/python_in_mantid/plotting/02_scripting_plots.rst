.. _02_scripting_plots:

==============================
Formatting Plots with a script
==============================

**Sometimes the easiest way to find out how to control part of a plot with Matplotlib is to search online for their** `documentation <https://matplotlib.org/3.2.1/index.html>`_ **! Below are some useful commands and a handful of links**

.. contents:: Table of contents
    :local:


General
=======

Required imports:

.. code-block:: python

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt

Access a workspace,loaded in the Workspace Toolbox, inside a script:

.. code-block:: python

    ws = mtd['ws']

    #or you could use:
    from mantid.api import AnalysisDataService as ADS
    ws = ADS.retrieve('ws')

Create a Figure and access its Axes for plotting:

.. code-block:: python

    fig, axes = plt.subplots(subplot_kw={'projection': 'mantid'})

(including the Mantid projection allows for plotting in the Mantid way, such as selecting a SpectrumNumber)

**Actually plot** the 1st spectrum of the workspace "ws" and control `many options <https://matplotlib.org/3.2.1/api/_as_gen/matplotlib.axes.Axes.plot.html>`_:

.. code-block:: python

    axes.plot(ws, specNum=1, color='red', label='spec 1 - ws', linewidth=1.0, linestyle='--', drawstyle='steps', marker = 'x')

Add a `legend <https://matplotlib.org/3.2.1/api/_as_gen/matplotlib.pyplot.legend.html>`_ containing the plotted data labels:

.. code-block:: python

    plt.legend()

Adjust the `scale to logarithmic <https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.pyplot.yscale.html>`_, or the `axis limits <https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.axes.Axes.set_xlim.html>`_:

.. code-block:: python

    axes.set_yscale('log')
    axes.set_xlim(0.0, 80.)
    # x and y can be swapped to alter the other axis

Add a title:

.. code-block:: python

    axes.set_title("My Wonderful Plot", fontsize=20, verticalalignment='bottom')

Add axis labels:

.. code-block:: python

    axes.set_xlabel(r'Time-of-flight ($\mu s$)'), axes.set_ylabel(r'Counts ($\mu s$)$^{-1}$')


Plotting with Errorbars
=======================

Simply use "`errorbar <https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.pyplot.errorbar.html>`_" instead of "plot":

.. code-block:: python

    axes.errorbar(exp_decay,specNum=3, capsize=2.0, label='spec 3', linewidth=1.0)


Tick Marks and Grid lines
=========================

Add `minor tick marks <https://matplotlib.org/3.2.1/gallery/ticks_and_spines/major_minor_demo.html>`_, here to the x-axis:

.. code-block:: python

    from matplotlib.ticker import (MultipleLocator, AutoMinorLocator)
    axes.xaxis.set_minor_locator(MultipleLocator(5)) # minor ticks every 5 units
    #axes.xaxis.set_minor_locator(AutoMinorLocator()) # python decides for you

Edit `tick options <https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.axes.Axes.tick_params.html>`_ such as direction in/out:

.. code-block:: python

   axes.tick_params(which='minor', width = 0.5, length=4, color='b', direction='in', top='on')

Even add `gridlines <https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.pyplot.grid.html>`_ :

.. code-block:: python

   axes.grid(True, which = 'both', axis = 'both') # major/minor, x/y

Notice how `gridlines are linked to the axis ticks <https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.axes.Axes.tick_params.html>`_:

.. code-block:: python

    axes.tick_params(which='minor', grid_color='r', grid_alpha=0.5)
    axes.tick_params(which='major', grid_color='b')


Fonts
=====

Alter the `font <https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.pyplot.text.html#matplotlib.pyplot.text>`_ on labels, axes, titles:

.. code-block:: python

    from matplotlib import rc
    rc('font',**{'family':'sans-serif','sans-serif':['Helvetica']})

    axes.ylabel(fontsize = 12, fontstyle = 'italic')

Alternatively, you can set a title or label to have certain `font properties <https://matplotlib.org/3.1.1/api/text_api.html#matplotlib.text.Text>`_:

.. code-block:: python

   axes.set_xlabel(r'Time-of-flight ($\mu s$)', fontsize = 12, fontstyle = 'italic', fontweight = 'bold', fontfamily='serif')

Here's `how to find available fonts <http://jonathansoma.com/lede/data-studio/matplotlib/list-all-fonts-available-in-matplotlib-plus-samples/>`_ .


Subplots and Inset plots
========================

Create a `tiled plot <https://matplotlib.org/devdocs/gallery/subplots_axes_and_figures/subplots_demo.html>`_ (subplot)

.. code-block:: python

    fig, axes = plt.subplots(ncols=2, nrows=2, subplot_kw={'projection': 'mantid'})
    # You've created 2x2 arrangement of plots, now plot in them:
    axes[0][0].plot(exp_decay,specNum=1)
    axes[0][1].plot(exp_decay,specNum=2)
    axes[1][0].plot(exp_decay,specNum=3)
    axes[1][1].plot(exp_decay,specNum=5)
    #for subplots it is useful to include the following line
    plt.tight_layout()

Add an `inset plot using the mantid projection <https://matplotlib.org/3.2.1/api/_as_gen/matplotlib.figure.Figure.html?highlight=add_axes#matplotlib.figure.Figure.add_axes>`_ (`without it <https://matplotlib.org/3.1.1/api/_as_gen/mpl_toolkits.axes_grid1.inset_locator.inset_axes.html>`_ ):

.. code-block:: python

    ax_sub = fig.add_axes([0.50, 0.50, 0.3, 0.25],projection='mantid') #[left, bottom, width, height]
    ax_sub.plot(exp_decay, specNum=5)


Generate a Script
=================

On a 1D or tiled plot in workbench, click the generate a script button |GenerateAScript.png| which will give more insight into the options for plotting from a script.


**NOTE** *It is very possible that the Generate a Script and Figure Options buttons on the plot toolbar will not work when that plot has been produced by a complex script*.


Useful links
============

For further info, including code for producing 2D colorfill plots see:

* `Mantid Plotting Examples <https://docs.mantidproject.org/nightly/plotting/index.html>`_
* `Matplotlib Gallery <https://matplotlib.org/3.1.1/gallery/index.html>`_
* `Mantid Script Plotting <https://docs.mantidproject.org/nightly/api/python/mantid/plots/index.html>`_


Example Script
==============

.. plot::
   :include-source:

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    from matplotlib.ticker import (MultipleLocator, AutoMinorLocator)

    #Example data
    exp_decay = CreateSampleWorkspace(Function='User Defined',
                                      UserDefinedFunction='\
                                      name=ExpDecay,Lifetime = 20,Height = 200;name=Gaussian,\
                                      PeakCentre=50, Height=10, Sigma=3',
                                      XMax=100, BinWidth=2)

    #Create figure and axes
    fig, axes = plt.subplots(ncols=2,nrows=1,subplot_kw={'projection': 'mantid'})

    # Plot with errorbars on the left set of axes
    axes[0].plot(exp_decay, specNum=1, color='red', label='400 K', linewidth=1.0, drawstyle='steps')
    axes[0].set_title('Steps and Grids')
    axes[0].xaxis.set_minor_locator(AutoMinorLocator())
    axes[0].grid(True, which = 'both', axis = 'both') # major/minor, x/y

    # Add an inset, use trial and error to find the right location
    inset = fig.add_axes([0.77, 0.70, 0.18, 0.18],projection='mantid') #[left, bottom, width, height]
    inset.plot(exp_decay, specNum=5, color='blue', label='Log Peak', marker ='.')
    plt.yscale('log') #only affects the most recently called axes
    inset.set_xlim(40, 60), inset.set_ylim(5, 20)
    inset.xaxis.set_minor_locator(AutoMinorLocator()) #inserts 5 minor b/w each major
    inset.tick_params(which='minor', width = 0.5, length=4, color='b', direction='in', top='on')

    #Plot on the right set of axes
    axes[1].errorbar(exp_decay, specNum=1, capsize=2.0, errorevery=5, color='green', label='spec 1', linestyle='--')
    axes[1].set_xlabel('Time-of-flight ($\mu s$)', fontsize = 12, fontstyle = 'italic', fontweight = 'bold')
    axes[1].set_ylabel('Counts ($\mu s$)$^{-1}$')
    axes[1].set_title('Errorbars and Insets')

    #Use tight layout for subplots and create a legend
    plt.tight_layout()
    fig.legend(loc='center right')

    #fig.show()


**Other Plotting Documentation**

* :ref:`plotting`
* :ref:`06_formatting_plots`
* `Matplotlib Keyboard Shortcuts <https://matplotlib.org/3.1.1/users/navigation_toolbar.html#navigation-keyboard-shortcuts>`_

.. |GenerateAScript.png| image:: /images/GenerateAScript.png
   :width: 30px
