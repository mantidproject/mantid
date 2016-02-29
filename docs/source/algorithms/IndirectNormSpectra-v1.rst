.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------
Algorithm designed to normalise all the spectra in the input workspace so that the maximum y value is 1.

Usage
-----

**Example - ConvolutionFitSequential**

.. testcode:: ConvolutionFitSequentialExample

  # Create Workspace
  data = '1,2,3,4,5'
  ws = CreateWorkspace(DataX=data, DataY=data, DataE=data, Nspec=1)
  
  # Execute algorithm
  out_ws = IndirectNormSpectra(InputWorkspace=ws)
  
  # Print resulting y values
  print out_ws.readY(0)

Output:  
  
.. testoutput:: ConvolutionFitSequentialExample
  :options: +NORMALIZE_WHITESPACE
  
  [ 0.2  0.4  0.6  0.8  1. ]

.. categories::

.. sourcelink::
