:orphan:

.. testcode:: mwTest_Running_Algorithms_With_Python[2]

   from mantid.simpleapi import *


.. testcode:: mwTest_Running_Algorithms_With_Python[9]

   from mantid.simpleapi import *
   # This example just has .RAW extension but it is able to load all 
   # file types that Mantid is aware of.
   # run = Load('filename.nxs')
   run = Load('HRP39182.RAW')


.. testsetup:: mwTest_Running_Algorithms_With_Python[39]

   run = Load('HRP39182.RAW')

.. testcode:: mwTest_Running_Algorithms_With_Python[39]

   # Mixture of positional and keyword arguments:
   run = ConvertUnits(run, Target='dSpacing')


.. testsetup:: mwTest_Running_Algorithms_With_Python[51]

   run = Load('HRP39182.RAW')

.. testcode:: mwTest_Running_Algorithms_With_Python[51]

   # All arguments provided as pure positional arguments:
   run = ConvertUnits(run, 'dSpacing','Direct',85,False)


.. testsetup:: mwTest_Running_Algorithms_With_Python[60]

   run = Load('HRP39182.RAW')

.. testcode:: mwTest_Running_Algorithms_With_Python[60]

   run = ConvertUnits(run, Target='dSpacing',EMode='Direct',EFixed=85,AlignBins=False) # keywords


.. testcode:: mwTest_Running_Algorithms_With_Python[71]

   import numpy as np
   
   def gaussian(x, mu, sigma):
     """Creates a gaussian peak centered on mu and with width sigma."""
     return (1/ sigma * np.sqrt(2 * np.pi)) * np.exp( - (x-mu)**2  / (2*sigma**2))
   
   #create two histograms with a single peak in each one
   x1 = np.arange(-1, 1, 0.02)
   x2 = np.arange(0.4, 1.6, 0.02)
   ws1 = CreateWorkspace(UnitX="1/q", DataX=x1, DataY=gaussian(x1[:-1], 0, 0.1)+1)
   ws2 = CreateWorkspace(UnitX="1/q", DataX=x2, DataY=gaussian(x2[:-1], 1, 0.05)+1)
   
   #stitch the histograms together
   stitched, scale = Stitch1D(LHSWorkspace=ws1, RHSWorkspace=ws2, StartOverlap=0.4, EndOverlap=0.6, Params=0.02)
   print type(scale)
   print type(stitched)

.. testoutput:: mwTest_Running_Algorithms_With_Python[71]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   <type 'float'>
   <class 'mantid.dataobjects._dataobjects.Workspace2D'>


