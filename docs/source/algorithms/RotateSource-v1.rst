
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm corrects the source's position by rotating it around an axis centered at the sample.
The rotation axis is perpendicular to the plane determined by the beam direction and the *up* direction.
The handedness of the coordinate system is considered to determine whether a positive/negative angle
corresponds to a clockwise or counterclockwise rotation.


Usage
-----

**Example - RotateSource**

.. testcode:: RotateSourceExample

   # Create a workspace with a simple instrument
   ws = CreateSampleWorkspace()

   # Original positions
   samplePos = ws.getInstrument().getSample().getPos()
   sourcePos = ws.getInstrument().getSource().getPos()
   print "Original position of the sample: [%.1f, %.1f, %.1f]" % (samplePos.X(), samplePos.Y(), samplePos.Z())
   print "Original position of the source: [%.1f, %.1f, %.1f]" % (sourcePos.X(), sourcePos.Y(), sourcePos.Z())

   # Move (rotate) the source around X axis
   RotateSource(ws, -90)

   # New positions
   samplePos = ws.getInstrument().getSample().getPos()
   sourcePos = ws.getInstrument().getSource().getPos()
   print "New position of the sample: [%.1f, %.1f, %.1f]" % (samplePos.X(), samplePos.Y(), samplePos.Z())
   print "New position of the source: [%.1f, %.1f, %.1f]" % (sourcePos.X(), sourcePos.Y(), sourcePos.Z())

Output:

.. testoutput:: RotateSourceExample

   Original position of the sample: [0.0, 0.0, 0.0]
   Original position of the source: [0.0, 0.0, -10.0]
   New position of the sample: [0.0, 0.0, 0.0]
   New position of the source: [0.0, 10.0, -0.0]

.. categories::

.. sourcelink::

