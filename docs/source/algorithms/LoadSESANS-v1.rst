.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads the given file in the SESANS text format. The file begins with a
number of compulsory headers, the first of which must be
'FileFormatVersion'. There are four compulsory data columns -
SpinEchoLength, Depolarisation, Depolarisation_error and
Wavelength. The output workspace has X values of SpinEchoLength and Y
values of depolarisation.

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
   SaveSESANS(InputWorkspace=out_ws, Filename=file_path, ThetaZMax=1,ThetaYMax=1, EchoConstant=1, Sample="Sample", OverrideSampleThickness=True)
   LoadSESANS(Filename=file_path, OutputWorkspace="in_ws")

   # Retrieve loaded workspace from ADS
   in_ws = mtd["in_ws"]
   print("Y values of loaded workspace = {}".format(in_ws.readY(0)))

.. testcleanup:: LoadSESANSRoundTrip

   os.remove(file_path)

Output:

.. testoutput:: LoadSESANSRoundTrip

   Y values of loaded workspace = [... ... ... ...]

.. categories::

.. sourcelink::
