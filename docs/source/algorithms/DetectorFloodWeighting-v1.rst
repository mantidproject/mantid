.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
This algorithm is used to calculate the detector flood weighting workspace use for pixel flood corrections. It was originally developed for the ANSTO Bilby instrument.

This algorithm crops the data over the specified wavelength region, and sums it. The algorithm will then perform a solid angle correction on each spectra via :ref:`algm-SolidAngle` if specified, and divides through by the provided transmission workspace if provided. The result is divided by the mean spectrum value in the previous result.

Usage
-----

**Example - Simple Generation**

.. testcode:: DetectorFloodWeightingExample

   import numpy as np 
   # create histogram workspace
   dataX = range(0,10) # or use dataX=range(0,10)
   dataY = [1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2] # or use dataY=[1]*9
   ws = CreateWorkspace(dataX, dataY, NSpec=2, UnitX="Wavelength")
   
   out_ws = DetectorFloodWeighting(InputWorkspace=ws, Bands=[0,10], SolidAngleCorrection=False)
   
   print('Number Histograms {}'.format(out_ws.getNumberHistograms()))
   print('Min X: {} Max X: {}'.format(out_ws.readX(0)[0], out_ws.readX(0)[1]))

Output:

.. testoutput:: DetectorFloodWeightingExample

   Number Histograms 2
   Min X: 0.0 Max X: 10.0

**Example - With Solid Angle Correction**

.. testcode:: DetectorFloodWeightingExampleWithCorrection

   ws = CreateSimulationWorkspace(Instrument='LOQ', BinParams=[1,1,10], UnitX="Wavelength")
   out_ws = DetectorFloodWeighting(InputWorkspace=ws, Bands=[0,10], SolidAngleCorrection=True)

   print('Number Histograms {}'.format(out_ws.getNumberHistograms()))
   print('Number of Bins {}'.format(out_ws.blocksize()))
   print('X units {}'.format(out_ws.getAxis(0).getUnit().unitID()))

Output:

.. testoutput:: DetectorFloodWeightingExampleWithCorrection

   Number Histograms 17776
   Number of Bins 1
   X units Wavelength

.. categories::

.. sourcelink::
