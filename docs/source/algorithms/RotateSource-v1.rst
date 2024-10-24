
.. algorithm::

.. summary::

.. relatedalgorithms::

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

   def pos3D_as_str(pos, digits=1, tolerance=1e-7):
     """
     Produce a string with a human readable version of a V3D position (x, y, z),
     from a V3D object, using a fixed limited number of digits (for robust string
     comparisons).
     """
     def nz(value):
        """ Handles potential issues with +-0 (tiny values) """
        return 0.0 if abs(value) < tolerance else value

     precision = str(digits)
     format_str = '[{0:.'+precision+'f}, {1:.'+precision+'f}, {2:.'+precision+'f}]'
     result = format_str.format(nz(pos.getX()), nz(pos.getY()), nz(pos.getZ()))
     return result

   print("Original position of the sample: {0}".format(pos3D_as_str(samplePos)))
   print("Original position of the source: {0}".format(pos3D_as_str(sourcePos)))

   # Move (rotate) the source around X axis
   RotateSource(ws, -90)

   # New positions
   samplePos = ws.getInstrument().getSample().getPos()
   sourcePos = ws.getInstrument().getSource().getPos()
   print("New position of the sample: {0}".format(pos3D_as_str(samplePos)))
   print("New position of the source: {0}".format(pos3D_as_str(sourcePos)))

Output:

.. testoutput:: RotateSourceExample

   Original position of the sample: [0.0, 0.0, 0.0]
   Original position of the source: [0.0, 0.0, -10.0]
   New position of the sample: [0.0, 0.0, 0.0]
   New position of the source: [0.0, 10.0, 0.0]

.. categories::

.. sourcelink::
