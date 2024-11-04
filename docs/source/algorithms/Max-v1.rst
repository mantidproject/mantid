.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm creates a new 1D workspace containing all maxima as well as their X boundaries and error.
This is used in particular for single crystal as a quick way to find strong peaks.

Usage
-----

.. testcode::

  # Create a workspace with two peaks in each spectrum
  ws = CreateSampleWorkspace(Function='Multiple Peaks')
  # Find the max values
  max_ws = Max(ws)

  # Check the returned values
  print('Maximum found at bin [ {} , {} ], value {}'.format(max_ws.readX(0)[0], max_ws.readX(0)[1], max_ws.readY(0)[0]))
  print('In original workspace')
  print('Bounds of bin 30     [ {} , {} ], value {}'.format(ws.readX(0)[30], ws.readX(0)[31], ws.readY(0)[30]))

  # Find another peak
  max_ws = Max(ws,RangeLower = 7000)

  # Check the returned values
  print('Maximum found at bin [ {} , {} ], value {}'.format(max_ws.readX(0)[0], max_ws.readX(0)[1], max_ws.readY(0)[0]))
  print('In original workspace')
  print('Bounds of bin 60     [ {} , {} ], value {}'.format(ws.readX(0)[60], ws.readX(0)[61], ws.readY(0)[60]))

Output
######

.. testoutput::

  Maximum found at bin [ 6000.0 , 6200.0 ], value 10.3
  In original workspace
  Bounds of bin 30     [ 6000.0 , 6200.0 ], value 10.3
  Maximum found at bin [ 12000.0 , 12200.0 ], value 8.3
  In original workspace
  Bounds of bin 60     [ 12000.0 , 12200.0 ], value 8.3

.. categories::

.. sourcelink::
