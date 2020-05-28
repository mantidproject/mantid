.. _03_pim_sol:

============================
Python in Mantid: Exercise 3
============================

A - plotSpectrum with ISIS Data
===============================

.. plot::
   :include-source:

    # import mantid algorithms and matplotlib
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt

    '''Processing'''

    # Load the spectral range of the interest of the data
    ws=Load(Filename="GEM40979.raw", SpectrumMin=431, SpectrumMax=750)

    # Convert to dSpacing
    ws=ConvertUnits(InputWorkspace=ws, Target= "dSpacing")

    # Smooth the data
    ws=SmoothData(InputWorkspace=ws, NPoints=20)

    '''Plotting'''

    # Plot three spectra
    PLOT_WINDOW = plotSpectrum(ws, [0,1,2])

    # Get Figure and Axes for formatting
    fig, axes = plt.gcf(), plt.gca()

    # Set the scales on the x- and y-axes
    axes.set_xlim(4, 6)
    axes.set_ylim(0, 5e3)

    # Plot index 5
    plotSpectrum(ws,5, window= PLOT_WINDOW, clearWindow= False)

    # Alter the Y label
    axes.set_ylabel('Neutron Counts ($\AA$)$^{-1}$')

    # Optionally rename the curve titles
    axes.legend(["bank2 sp-431", "bank2 sp-432", "bank2 sp-433", "bank2 sp-436"])

    # Give the plot a title
    plt.title("Oscillations", fontsize=20)


B - Direct Matplotlib with SNS Data
===================================

.. plot::
   :include-source:

    # import mantid algorithms and matplotlib
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt

    # Load the data
    run = Load('Training_Exercise3a_SNS.nxs')

    fig, axes = plt.subplots(subplot_kw={'projection': 'mantid'})

    # Choose legend labels and colors for each curve
    labels = ("sp-1", "sp-2", "sp-3", "sp-4", "sp-5")
    colors = ('#FFCFC4', '#FE886F','#FE4A23', '#B82405', '#6A1300')

    # Plot the first 5 spectra
    for i in range(5):
        axes.plot(run, wkspIndex=i, color=colors[i], label=labels[i])

    # Plot the 9th spectrum with errorbars
    axes.errorbar(run, specNum=9, capsize=2.0, color='blue', label='Peak of Interest', linewidth=1.0)

    # Set the X-axis limts and the Y-axis scale
    axes.set_xlim(-1.5, 1.8)
    plt.yscale('log')

    # Give the plot a title
    plt.title("Peak Evolution", fontsize=20)

    # Add a legend with the chosen labels and show the plot
    axes.legend()
    #plt.show() #uncomment to show the plot

    # Note with the Direct Matplotlib method,
    # there are many more options for formatting the plot


C - 2D and 3D Plot ILL Data
===========================


# import mantid algorithms, numpy and matplotlib
from mantid.simpleapi import *
import matplotlib.pyplot as plt

# Load the data and extract the region of interest
in6=Load('164198.nxs')
in6=ExtractSpectra(in6, XMin=3100, XMax=3300, StartWorkspaceIndex=199, EndWorkspaceIndex=209)

'''2D Colorfill Plotting'''

# Get a figure and axes 
figC,axesC = plt.subplots(subplot_kw={'projection':'mantid'})

# Plot the data as a 2D colorfill
c=axesC.imshow(in6,cmap='jet', aspect='auto')

# Change the title
axesC.set_title("2D - Colorfill")

# Add a Colorbar with a label
cbar=figC.colorbar(c)
cbar.set_label('Counts ($\mu s$)$^{-1}$')

'''3D Plotting - Contour, Surface and Wireframe'''

# Get a different set of figure and axes with 3 subplots for 3D plotting
fig,axes = plt.subplots(ncols=3, subplot_kw={'projection':'mantid3d'}, figsize = (12,3))

# 3D plot the data, and choose colormaps and colors
axes[0].contour(in6, cmap = 'spring')
axes[1].plot_surface(in6, cmap='summer')
axes[2].plot_wireframe(in6, color='darkmagenta')

# Add titles to the 3D plots
axes[0].set_title("Contour")
axes[1].set_title("Surface")
axes[2].set_title("Wireframe")

#plt.show # uncomment to show the plots



