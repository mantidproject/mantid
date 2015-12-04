
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm corrects the source's position by rotating it around an axis centered at the sample.
The rotation axis is perpendicular to the plane determined by the beam direction and the *up* direction.


Usage
-----

**Example - RotateSource**

.. testcode:: RotateSourceExample

   # Create a workspace with a simple instrument
   ws = CreateSampleWorkspace()

   # Original positions
   print "Original position of the sample", ws.getInstrument().getSample().getPos()
   print "Original position of the source", ws.getInstrument().getSource().getPos()

   # Move (rotate) the source around X axis
   RotateSource(ws, 90)

   # New positions
   print "New position of the sample", ws.getInstrument().getSample().getPos()
   print "New position of the source", ws.getInstrument().getSource().getPos()

Output:

.. testoutput:: RotateSourceExample

   Original position of the sample [0,0,0]
   Original position of the source [0,0,-10]
   New position of the sample [0,0,0]
   New position of the source [0,10,0]

.. categories::

.. sourcelink::

