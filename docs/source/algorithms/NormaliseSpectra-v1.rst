.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------
Algorithm designed to normalise all the spectra in the input workspace so that the value of the resulting workspace has a maximum of 1.
This algorithm is designed to run on workspaces where every spectra contains at least one y value greater than 0.
This is the case because the data is being normalised to be a maximum of positive 1.


Usage
-----

**Example - NormaliseSpectra**

.. testcode:: NormaliseSpectraExample

  # Create Workspace
  data = '0,1,2,3,4,5'
  ws = CreateWorkspace(DataX=data, DataY=data, DataE=data, Nspec=1)
  
  # Execute algorithm
  out_ws = NormaliseSpectra(InputWorkspace=ws)
  
  # Print resulting y values
  print out_ws.readY(0)

Output:  
  
.. testoutput:: NormaliseSpectraExample
  :options: +NORMALIZE_WHITESPACE
  
  [ 0. 0.2  0.4  0.6  0.8  1. ]

.. categories::

.. sourcelink::
