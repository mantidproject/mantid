.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm multiplies the Y values in a range of bins in a workspace by the factor given.

Usage
-----

.. testcode::

  # Create a workspace with 100 bins
  ws = CreateSampleWorkspace()
  # Multiply the values in bins 25 to 75 by 3
  res = MultiplyRange(ws,StartBin=25,EndBin=75,Factor=3.0)

  # Check the result
  # Get the Y values from the first spectra of the input and output workspaces
  y = ws.readY(0)
  yres = res.readY(0)

  # Print out the ratios yres[i] / y[i] for bins with indices 20 to 30
  print(yres[20:30] / y[20:30])
  # Print out the ratios yres[i] / y[i] for bins with indices 70 to 80
  print(yres[70:80] / y[70:80])

Output
######

.. testoutput::

  [1. 1. 1. 1. 1. 3. 3. 3. 3. 3.]
  [3. 3. 3. 3. 3. 3. 1. 1. 1. 1.]

.. categories::

.. sourcelink::
