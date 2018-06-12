.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm can be used to change the time-of-flight bins of a
workspace by a specified amount (defined above as the Offset). A
possible use of this algorithm is to correct time bins that have been
recorded incorrectly.

Optionally, the range of spectra can be selected to apply this offset
selectively using the IndexMin and IndexMax properties.

The output workspace will be an exact copy of the input workspace except
for the changed time bins.

Usage
-----

.. testcode:: ExOffset

  import numpy as np

  # Create a workspace
  ws = CreateSampleWorkspace()

  # Offset the time bins by 1.0
  wsOffset = ChangeBinOffset(ws,Offset=1.0)

  # Check the offset
  x1 = ws.readX(0)
  x2 = wsOffset.readX(0)
  # Test that all elements of arrays x2 and x1 differ by 1.0
  print(np.all( x2 - x1 == 1.0 ))
  y1 = ws.readY(0)
  y2 = wsOffset.readY(0)
  # Test that arrays y2 and y1 are equal
  print(np.all( y2 == y1 ))

.. testoutput:: ExOffset

  True
  True

.. categories::

.. sourcelink::
