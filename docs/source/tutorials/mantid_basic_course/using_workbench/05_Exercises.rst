.. _05_Exercises:

=========
Exercises
=========

It is possible to replicate plots you've made previously in MantidPlot entirely using python script in Workbench. 
For all these exercises do not make any changes to the default imports given by the Workbench script editor.
You can execute the script after each step to make sure that there are no errors raised.

Part 1:
=======

#. Load the File "GEM38370_Focussed_Legacy.nxs" using the algorithm: ``GEM38370_Focussed_Legacy = Load("GEM38370_Focussed_Legacy")``
   
#. Before plotting from workspaces you will need to set your subplot up with the Mantid projection so it can interpret them. To do this include: 
   
   ``fig,ax = plt.subplots(subplot_kw = {"projection": "mantid"})``
   
   This will let you plot spectra from workspaces by including a spectrum number ``specNum`` or workspace index ``wkspIndex`` in the plot command.
   
   ``ax.plot(RawData, specNum = 2)`` or ``ax.plot(RawData, wkspIndex = 3)``
   
#. Plot the other spectra 3-7.

   Now that the subplot is set up to plot spectra you can plot other curves in the same window using the same command with a different ``specNum``
   or ``wkspIndex``, by modifying the script you have written already you can use a ``for`` loop to plot all spectra in sequence.
   
   .. code-block:: python
   
     for i in range(6):
         ax.plot(GEM38370_Focussed_Legacy, wkspIndex = i)

#. change the d-spacing axis range to 0-10 angstroms and set the x-axis to a logarithmic scale

   Since ``ax`` is a matplotlib subplot we can us the built-in matplotlib attributes to change the the axis range ``ax.set_xlim([0.0,10.0])`` and 
   scale ``ax.set_xscale('symlog')``. You can then use the ``fig.show()`` function to show your plot
   
By following these steps you should end up with code that looks something like this:

.. code-block:: python
   
   # The following line helps with future compatibility with Python 3
   # print must now be used as a function, e.g print('Hello','World')
   from __future__ import (absolute_import, division, print_function, unicode_literals)
   
   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   
   import matplotlib.pyplot as plt
   
   import numpy as np
   
   GEM38370_Focussed_Legacy = Load("GEM38370_Focussed_Legacy")
   fig,ax = plt.subplots(subplot_kw = {"projection": "mantid"})
   for i in range(6):
       ax.plot(GEM38370_Focussed_Legacy, wkspIndex = i)
   ax.set_xlim([0.0,10.0])
   ax.set_xscale('symlog')
   fig.show()


That will produce a figure looking like this: 

.. figure:: /images/exerciseWorkbench_part_1_figure.png
   :width: 700px
   :alt: The resulting figure from part 1.
   
Part 2
======

#. Load the EventWorkspace HYS_11388_event.nxs

#. using the ``SumSpectra()`` algorithm sum across each spectra in the workspace, assigning the result to a new workspace ``Sum`` 

#. using the ``Rebin()`` algorithm rebin your new ``Sum`` workspace with bin width of 100 microseconds and events being preserved and assign it to another new workspace ``binned``.

   *Hint: your* ``Params`` *should be set to 100 and* ``PreserveEvents`` *should equal* ``True``
   
#. plot the spectrum of ``binned`` in the same was the previous part.

#. Filter out events recored before 4000 microseconds using the ``FilterByTime()`` algorithm into a workspace called ``FilteredByTime``, and plot ``FilteredByTime`` to the same figure.

#. Using the Matplotlib function add a legend to your plot ``ax.legend()`` and then show your figure.

By following these steps you should end up with code that looks something like this:

.. code-block:: python

   # The following line helps with future compatibility with Python 3
   # print must now be used as a function, e.g print('Hello','World')
   from __future__ import (absolute_import, division, print_function, unicode_literals)

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *

   import matplotlib.pyplot as plt

   import numpy as np

   HYS_11388_event = Load("HYS_11388_event")
   Sum = SumSpectra(HYS_11388_event)
   binned = Rebin(Sum,Params = 100,PreserveEvents=True)
   fig,ax = plt.subplots(subplot_kw = {"projection": "mantid"})
   ax.plot(binned, wkspIndex = 0)
   FilteredByTime = FilterByTime(binned, StartTime = 4000) 
   ax.plot(FilteredByTime, wkspIndex = 0)
   ax.legend()
   fig.show()
   

That will produce a figure looking like this: 

.. figure:: /images/exerciseWorkbench_part_2_figure.png
   :width: 700px
   :alt: The resulting figure from part 2.
