.. _plotting:

.. contents:: Table of contents
    :local:

====================================
Introduction to Matplotlib in Mantid
====================================

Mantid now can use `Matplotlib <https://matplotlib.org/>`_ to produce figures. 
There are several advantages of using this software package:

* it is python based, so it can easily be incorporated into Mantid scripts
* there is a large user community, and therefore excellent documentation and examples are available
* it is easy to change from plotting on the screen to produce publication quality plots in various image formats 

While Matplotlib is using data arrays for inputs in the plotting routines,
it is now possible to also use several types on Mantid workspaces instead.
For a detailed list of functions that use workspaces, see the documentaion
of the :ref:`mantid.plots <mantid.plots>` module. 

This page is intended to provide examples about how to use different
Matplotlib commands for several types of common task that Mantid users are interested in.

To understand the matplotlib vocabulary, a useful tool is the `"anatomy of a figure"
<https://matplotlib.org/examples/showcase/anatomy.html>`_, also shown below.

.. plot::
   
   # This figure shows the name of several matplotlib elements composing a figure
   import numpy as np
   import matplotlib.pyplot as plt
   from matplotlib.ticker import AutoMinorLocator, MultipleLocator, FuncFormatter

   np.random.seed(19680801)

   X = np.linspace(0.5, 3.5, 100)
   Y1 = 3+np.cos(X)
   Y2 = 1+np.cos(1+X/0.75)/2
   Y3 = np.random.uniform(Y1, Y2, len(X))

   fig = plt.figure(figsize=(8, 8))
   ax = fig.add_subplot(1, 1, 1, aspect=1)

   def minor_tick(x, pos):
       if not x % 1.0:
           return ""
       return "%.2f" % x

   ax.xaxis.set_major_locator(MultipleLocator(1.000))
   ax.xaxis.set_minor_locator(AutoMinorLocator(4))
   ax.yaxis.set_major_locator(MultipleLocator(1.000))
   ax.yaxis.set_minor_locator(AutoMinorLocator(4))
   ax.xaxis.set_minor_formatter(FuncFormatter(minor_tick))

   ax.set_xlim(0, 4)
   ax.set_ylim(0, 4)

   ax.tick_params(which='major', width=1.0)
   ax.tick_params(which='major', length=10)
   ax.tick_params(which='minor', width=1.0, labelsize=10)
   ax.tick_params(which='minor', length=5, labelsize=10, labelcolor='0.25')

   ax.grid(linestyle="--", linewidth=0.5, color='.25', zorder=-10)

   ax.plot(X, Y1, c=(0.25, 0.25, 1.00), lw=2, label="Blue signal", zorder=10)
   ax.plot(X, Y2, c=(1.00, 0.25, 0.25), lw=2, label="Red signal")
   ax.plot(X, Y3, linewidth=0,
           marker='o', markerfacecolor='w', markeredgecolor='k')

   ax.set_title("Anatomy of a figure", fontsize=20, verticalalignment='bottom')
   ax.set_xlabel("X axis label")
   ax.set_ylabel("Y axis label")

   ax.legend()


   def circle(x, y, radius=0.15):
       from matplotlib.patches import Circle
       from matplotlib.patheffects import withStroke
       circle = Circle((x, y), radius, clip_on=False, zorder=10, linewidth=1,
                       edgecolor='black', facecolor=(0, 0, 0, .0125),
                       path_effects=[withStroke(linewidth=5, foreground='w')])
       ax.add_artist(circle)


   def text(x, y, text):
       ax.text(x, y, text, backgroundcolor="white",
               ha='center', va='top', weight='bold', color='blue')


   # Minor tick
   circle(0.50, -0.10)
   text(0.50, -0.32, "Minor tick label")

   # Major tick
   circle(-0.03, 4.00)
   text(0.03, 3.80, "Major tick")

   # Minor tick
   circle(0.00, 3.50)
   text(0.00, 3.30, "Minor tick")

   # Major tick label
   circle(-0.15, 3.00)
   text(-0.15, 2.80, "Major tick label")

   # X Label
   circle(1.80, -0.27)
   text(1.80, -0.45, "X axis label")

   # Y Label
   circle(-0.27, 1.80)
   text(-0.27, 1.6, "Y axis label")

   # Title
   circle(1.60, 4.13)
   text(1.60, 3.93, "Title")

   # Blue plot
   circle(1.75, 2.80)
   text(1.75, 2.60, "Line\n(line plot)")

   # Red plot
   circle(1.20, 0.60)
   text(1.20, 0.40, "Line\n(line plot)")

   # Scatter plot
   circle(3.20, 1.75)
   text(3.20, 1.55, "Markers\n(scatter plot)")

   # Grid
   circle(3.00, 3.00)
   text(3.00, 2.80, "Grid")

   # Legend
   circle(3.70, 3.80)
   text(3.70, 3.60, "Legend")

   # Axes
   circle(0.5, 0.5)
   text(0.5, 0.3, "Axes")

   # Figure
   circle(-0.3, 0.65)
   text(-0.3, 0.45, "Figure")

   color = 'blue'
   ax.annotate('Spines', xy=(4.0, 0.35), xycoords='data',
               xytext=(3.3, 0.5), textcoords='data',
               weight='bold', color=color,
               arrowprops=dict(arrowstyle='->',
                               connectionstyle="arc3",
                               color=color))

   ax.annotate('', xy=(3.15, 0.0), xycoords='data',
               xytext=(3.45, 0.45), textcoords='data',
               weight='bold', color=color,
               arrowprops=dict(arrowstyle='->',
                               connectionstyle="arc3",
                               color=color))

   ax.text(4.0, -0.4, "Made with http://matplotlib.org",
           fontsize=10, ha="right", color='.5')

   fig.show()


Here are some of the highlights:

* **Figure** is the main container in matplotlib. You can think of it as the page
* **Axes** is the coordinate system. It contains most of the figure elements, such as Axis, Line2D, Text. 
  One can have multiple Axes objects in one Figure
* **Axis** is the container for the ticks and labels for the x and y axis of the plot

============
Simple plots
============

For matrix workspaces, if we use the `mantid` projection, one can plot the data in a similar
fashion as the plotting of arrays in matplotlib. Moreover, one can combine the two in the same figure

.. plot::
   :include-source:
      
   import numpy as np
   import matplotlib.pyplot as plt
   from mantid import plots
   from mantid.simpleapi import CreateWorkspace
   
   # Create a workspace that has a Gaussian peak
   x = np.arange(20)
   y0 = 10.+50*np.exp(-(x-10)**2/20)
   err=np.sqrt(y0)
   y = 10.+50*np.exp(-(x-10)**2/20)
   for i in range(20):
       y[i] += err[i]*np.random.normal()
   err = np.sqrt(y)
   w = CreateWorkspace(DataX=x, DataY=y, DataE=err, NSpec=1, UnitX='DeltaE')
   
   # Plot - note that the projection='mantid' keyword is passed to all axes
   fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
   ax.errorbar(w,'rs') # plot the workspace with errorbars, using red squares
   ax.plot(x,y0,'k-', label='Initial guess') # plot the initial guess with black line
   ax.legend() # show the legend
   fig.show()

Some data should be visualized as two dimensional colormaps

.. plot::
   :include-source:

   import matplotlib.pyplot as plt
   from mantid import plots
   from mantid.simpleapi import Load, ConvertToMD, BinMD, ConvertUnits, Rebin
   from matplotlib.colors import LogNorm

   # generate a nice 2D multi-dimensional workspace
   data = Load('CNCS_7860')
   data = ConvertUnits(InputWorkspace=data,Target='DeltaE', EMode='Direct', EFixed=3)
   data = Rebin(InputWorkspace=data, Params='-3,0.025,3')
   md = ConvertToMD(InputWorkspace=data,
                    QDimensions='|Q|',
                    dEAnalysisMode='Direct')
   sqw = BinMD(InputWorkspace=md,
               AlignedDim0='|Q|,0,3,100',
               AlignedDim1='DeltaE,-3,3,100')
    
   #2D plot
   fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
   c = ax.pcolormesh(sqw, norm=LogNorm())
   cbar=fig.colorbar(c)
   cbar.set_label('Intensity (arb. units)') #add text to colorbar
   fig.show()

One can then change properties of the plot. Here is an example that
changes the label of the data, changes the label of the x and y axis,
changes the limits for the y axis, adds a title, change tick orientations,
and adds a grid


.. plot::
   :include-source:
      
   import numpy as np
   import matplotlib.pyplot as plt
   from mantid import plots
   from mantid.simpleapi import CreateWorkspace
   
   # Create a workspace that has a Gaussian peak
   x = np.arange(20)
   y0 = 10.+50*np.exp(-(x-10)**2/20)
   err=np.sqrt(y0)
   y = 10.+50*np.exp(-(x-10)**2/20)
   for i in range(20):
       y[i] += err[i]*np.random.normal()
   err = np.sqrt(y)
   w = CreateWorkspace(DataX=x, DataY=y, DataE=err, NSpec=1, UnitX='DeltaE')
   
   # Plot - note that the projection='mantid' keyword is passed to all axes
   fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
   ax.errorbar(w,'rs', label='Data')
   ax.plot(x,y0,'k-', label='Initial guess')
   ax.legend()
   ax.set_xlabel('Better energy estimate ($10^3\mu eV$)')
   ax.set_ylabel('Neutron counts')
   ax.set_ylim(-10,100)
   ax.set_title('Gaussian example')
   ax.tick_params(axis='x', direction='in')
   ax.tick_params(axis='y', direction='out')
   ax.grid(True)
   fig.show()


Let's create now a figure with two panels. In the upper part we show the workspace as
above, but we add a fit, In the bottom part we add the difference.

.. plot::
   :include-source:
      
   import numpy as np
   import matplotlib.pyplot as plt
   from mantid import plots
   from mantid.simpleapi import CreateWorkspace, Fit
   
   # Create a workspace that has a Gaussian peak
   x = np.arange(20)
   y0 = 10.+50*np.exp(-(x-10)**2/20)
   err=np.sqrt(y0)
   y = 10.+50*np.exp(-(x-10)**2/20)
   for i in range(20):
       y[i] += err[i]*np.random.normal()
   err = np.sqrt(y)
   w = CreateWorkspace(DataX=x, DataY=y, DataE=err, NSpec=1, UnitX='DeltaE')
   result = Fit(Function='name=LinearBackground,A0=10,A1=0.;name=Gaussian,Height=60.,PeakCentre=10.,Sigma=3.', 
                InputWorkspace='w', 
                Output='w', 
                OutputCompositeMembers=True)
   # The handle to the output workspace is result.OutputWorkspace. The first spectrum is the data,
   # the second is the fit, the third is the difference. Subsequent spectra are the calculated
   # functions of each of the components of the fit (here LinearBackground and Gaussian)
   # Note that the difference spectrum has 0 errors. One can copy the errors from data
   result.OutputWorkspace.setE(2,result.OutputWorkspace.readE(0))

   #do the plotting   
   fig, [ax_top, ax_bottom] = plt.subplots(figsize=(9, 6),
                                           nrows=2, 
                                           ncols=1, 
                                           sharex=True,
                                           gridspec_kw={'height_ratios':[2,1]},
                                           subplot_kw={'projection':'mantid'})

   ax_top.errorbar(result.OutputWorkspace,'rs',wkspIndex=0, label='Data')
   ax_top.plot(result.OutputWorkspace,'b-',wkspIndex=1, label='Fit')
   ax_top.legend()
   ax_top.set_xlabel('')
   ax_top.set_ylabel('Neutron counts')
   ax_top.tick_params(axis='both', direction='in')
   ax_bottom.errorbar(result.OutputWorkspace,'ko',wkspIndex=2)
   ax_bottom.tick_params(axis='both', direction='in')
   ax_bottom.set_ylabel('Difference')
   fig.tight_layout()
   fig.show()
   

====================
Plotting Sample Logs
====================

The :func:`mantid.plots.MantidAxes.plot<mantid.plots.MantidAxes.plot>` function can show sample logs. By default,
the time axis represents the time since the first proton charge pulse (the
beginning of the run), but one can also plot absolute time using `FullTime=True`

.. plot::
   :include-source:
   
   import matplotlib.pyplot as plt
   from mantid import plots
   from mantid.simpleapi import Load
   
   w=Load('CNCS_7860')
   fig = plt.figure()
   ax1 = fig.add_subplot(211,projection='mantid')
   ax2 = fig.add_subplot(212,projection='mantid')
   ax1.plot(w, LogName='ChopperStatus5')
   ax1.set_title('From run start')
   ax2.plot(w, LogName='ChopperStatus5',FullTime=True)
   ax2.set_title('Absolute time')
   fig.tight_layout()
   fig.show()
   

Note that the parasite axes in matplotlib do not accept the projection keyword.
So one needs to use :func:`mantid.plots.plotfunctions.plot<mantid.plots.plotfunctions.plot>` instead.

.. plot::
   :include-source:
   
   import matplotlib.pyplot as plt
   from mantid import plots
   from mantid.simpleapi import Load
   w=Load('CNCS_7860')
   fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
   ax.plot(w,LogName='ChopperStatus5')
   axt=ax.twiny()
   plots.plotfunctions.plot(axt,w,LogName='ChopperStatus5', FullTime=True)
   fig.show()


=============
Complex plots
=============

Plotting dispersion curves can also be done using matplotlib

.. plot::
   :include-source:
   
   import matplotlib.pyplot as plt
   import numpy as np
   from matplotlib.gridspec import GridSpec
   from mantid.simpleapi import CreateMDHistoWorkspace
   from mantid import plots
   
   # Generate nice (fake) dispersion data
   q = np.arange(0,0.333,0.01)
   e = np.arange(0,60)
   x,y = np.meshgrid(q,e)
   def omega_hh(q):
       return 20.* np.sin(np.pi*q)
   def I(q):
       return np.exp(-q*5.)
   signal = I(x) * np.exp(-(y-omega_hh(x))**2)
   ws1=CreateMDHistoWorkspace(Dimensionality=2,
                              Extents='0,0.3333,0,60',
                              SignalInput=signal,
                              ErrorInput=np.sqrt(signal),
                              NumberOfBins='{0},{1}'.format(len(q),len(e)),
                              Names='Dim1,Dim2',
                              Units='MomentumTransfer,EnergyTransfer')
   def omega_hm2h(q):
       return 20*np.sin(np.pi*(q*2.+1./3))
   signal = np.exp(-(y-omega_hm2h(x))**2)
   ws2=CreateMDHistoWorkspace(Dimensionality=2,
                              Extents='0.3333,0.5,0,60',
                              SignalInput=signal,
                              ErrorInput=np.sqrt(signal),
                              NumberOfBins='{0},{1}'.format(len(q),len(e)),
                              Names='Dim1,Dim2',
                              Units='MomentumTransfer,EnergyTransfer')
   
   
   d=6.7
   a=2.454
   #Gamma is (0,0,0)
   #A is (0,0,1/2)
   #K is (1/3,1/3,0)
   #M is (1/2,0,0)
   gamma_a=np.pi/d
   gamma_m=2.*np.pi/np.sqrt(3.)/a
   m_k=2.*np.pi/3/a
   gamma_k=4.*np.pi/3/a
   
   fig=plt.figure(figsize=(12,5))
   gs = GridSpec(1, 4,
                 width_ratios=[gamma_k,m_k,gamma_m,gamma_a],
                 wspace=0)
   
   ax1 = plt.subplot(gs[0],projection='mantid')
   ax2 = plt.subplot(gs[1],sharey=ax1,projection='mantid')
   ax3 = plt.subplot(gs[2],sharey=ax1)
   ax4 = plt.subplot(gs[3],sharey=ax1)
   
   ax1.pcolormesh(ws1)
   ax2.pcolormesh(ws2)
   ax3.plot([0,0.5],[0,20])
   ax4.plot([0,0.5],[0,10])
   
   
   #Adjust plotting parameters
   
   ax1.set_ylabel('E (meV)')
   ax1.set_xlim(0,1./3)
   ax1.set_ylim(0.,40.)
   ax1.set_title(r'$[\epsilon,\epsilon,0], 0 \leq \epsilon \leq 1/3$') 
   ax1.set_xticks([0,1./3])
   ax1.set_xticklabels(['$\Gamma$','$K$'])
   #ax1.spines['right'].set_visible(False)
   ax1.tick_params(direction='in')
   
   ax2.get_yaxis().set_visible(False)
   ax2.set_xlim(1./3,1./2)
   ax2.set_title(r'$[1/3+\epsilon,1/3-2\epsilon,0], 0 \leq \epsilon \leq 1/6$') 
   ax2.set_xticks([1./2])
   ax2.set_xticklabels(['$M$'])
   #ax2.spines['left'].set_visible(False)
   ax2.tick_params(direction='in')
   
   #invert axis
   ax3.set_xlim(1./2,0)
   ax3.get_yaxis().set_visible(False)
   ax3.set_title(r'$[\epsilon,0,0], 1/2 \geq \epsilon \geq 0$') 
   ax3.set_xticks([0])
   ax3.set_xticklabels(['$\Gamma$'])
   ax3.tick_params(direction='in')
   
   ax4.set_xlim(0,1./2)
   ax4.get_yaxis().set_visible(False)
   ax4.set_title(r'$[0,0,\epsilon], 0 \leq \epsilon \leq 1/2$') 
   ax4.set_xticks([1./2])
   ax4.set_xticklabels(['$A$'])
   ax4.tick_params(direction='in')
   fig.show()
