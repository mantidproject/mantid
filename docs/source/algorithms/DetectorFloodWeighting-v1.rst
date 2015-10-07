.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------
This algorithm is used to calculate the detector flood weighting workspace use for pixel flood corrections. It was originally developed for the ANSTO Bilby instrument.

This algorithm crops the data over the specified wavelength region, then normalizes each spectrum to the workspace spectrum maxima.

Usage
-----

**Example - Simple Generation **

.. testcode:: DetectorFloodWeightingExample

   import numpy as np 
   # create histogram workspace
   dataX = range(0,10) # or use dataX=range(0,10)
   dataY = [1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2] # or use dataY=[1]*9
   ws = CreateWorkspace(dataX, dataY, NSpec=2, UnitX="Wavelength")
   
   out_ws = DetectorFloodWeighting(InputWorkspace=ws, Bands=[0,10])
   
   print 'Number Histograms',out_ws.getNumberHistograms()
   print 'Min X:', out_ws.readX(0)[0], 'Max X:', out_ws.readX(0)[1]  
   y_data = out_ws.extractY()
   print  'Min Y:', np.amin(y_data), 'Max Y:', np.amax(y_data)   

Output:

.. testoutput:: DetectorFloodWeightingExample
   Number Histograms 2
   Min X: 0.0 Max X: 10.0
   Min Y: 0.5 Max Y: 1.0


.. categories::

.. sourcelink::
