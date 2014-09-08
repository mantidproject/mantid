.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Masks detectors that are contained within a user defined 3 dimensional
shape within the instrument.

The algorithm places the user defined geometric shape within the virtual
instrument and masks any detector detectors that in contained within it.
A detector is considered to be contained it its central location point
is contained within the shape.

:ref:`This page <HowToDefineGeometricShape>`
provides a description of the syntax of ShapeXML string.

ChildAlgorithms used
####################

MaskDetectorsInShape runs the following algorithms as child algorithms:

-  :ref:`algm-FindDetectorsInShape` - To determine the
   detectors that are contained in the user defined shape.
-  :ref:`algm-MaskDetectors` - To mask the detectors found.


Usage
-----

.. testcode::

  # Create a workspace
  ws = CreateSampleWorkspace()

  # Define an infinite cylinder with its axis parallel to the Z-axis
  # and the radius of 0.04
  shapeXML = \
  """
  <infinite-cylinder id="A" >
      <centre x="0" y="0" z="0" />
      <axis x="0" y="0" z="1" />
      <radius val="0.04" />
  </infinite-cylinder>
  """
  # Mask all detectors inside this cylinder
  MaskDetectorsInShape( ws, shapeXML )

  #Check the result
  masked_dets =  []
  inside_dets = []
  # Collect separately all masked detector IDs and IDs of detectors
  # that are inside the cylinder defined by shapeXML
  R2 = 0.04**2 # cylinder radius sqared
  for i in range(ws.getNumberHistograms()):
    det = ws.getDetector(i)
    if det.isMasked():
      masked_dets.append( det.getID() )
    r = det.getPos()
    if r.X()**2 + r.Y()**2 <= R2:
      inside_dets.append( det.getID() )

  # Print out the IDs
  print masked_dets
  print inside_dets

  # Check that the two arrays are equal
  print masked_dets == inside_dets

Output
######

.. testoutput::

  [100, 101, 102, 103, 104, 105, 110, 111, 112, 113, 114, 120, 121, 122, 123, 124, 130, 131, 132, 133, 134, 140, 141, 142, 143, 150, 200, 201, 202, 203, 204, 205, 210, 211, 212, 213, 214, 220, 221, 222, 223, 224, 230, 231, 232, 233, 234, 240, 241, 242, 243, 250]
  [100, 101, 102, 103, 104, 105, 110, 111, 112, 113, 114, 120, 121, 122, 123, 124, 130, 131, 132, 133, 134, 140, 141, 142, 143, 150, 200, 201, 202, 203, 204, 205, 210, 211, 212, 213, 214, 220, 221, 222, 223, 224, 230, 231, 232, 233, 234, 240, 241, 242, 243, 250]
  True

.. categories::
