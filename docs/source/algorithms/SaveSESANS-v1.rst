.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves the given workspace to a file which will be formatted in the
SESANS data format. A workspace with a single spectrum is expected,
where the X values are wavelength and the Y values are polarisation

Usage
-----

**Example - Loading a file**

.. testcode:: LoadSESANSRoundTrip

   import os
   
   # Create dummy workspace
   dataX = [1,2,3,4,5]
   dataY = [6,1,9,14]
   dataE = [1,1,4,5]
   out_ws = CreateWorkspace(dataX, dataY, dataE)
   out_ws.setTitle("Dummy workspace")
   
   file_path = os.path.join(config["defaultsave.directory"], "example.ses")
   
   # Do a 'roundtrip' of the data
   SaveSESANS(InputWorkspace=out_ws, Filename=file_path, ThetaZMax=1,ThetaYMax=1, EchoConstant=1, Sample="Sample")
   LoadSESANS(Filename=file_path, OutputWorkspace="in_ws")
   
   # Retrieve loaded workspace from ADS
   in_ws = mtd["in_ws"]
   print("Y values of loaded workspace = " + str(in_ws.readY(0)))
   
.. testcleanup:: LoadSESANSRoundTrip

   os.remove(file_path)

Output:

.. testoutput:: LoadSESANSRoundTrip

   Y values of loaded workspace = [ 0.796338  0.        0.179365  0.130324]

.. categories::

.. sourcelink::   
