
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Use this algorithm to define the initial orientation of the sample with respect
to the beam and instrument by giving the axes, angle and directions of rotations.
Enter each axis in the order of rotation, starting with the one farthest from the
sample similar to :ref:`algm-SetGoniometer`

You may enter up to 6 axes, for each of which you must define 5 values as below separated by
commas:

-  The angle of rotation in degrees.
-  The X, Y, Z components of the vector of the axis of rotation.
   Right-handed coordinates with +Z=beam direction; +Y=Vertically up
   (against gravity); +X to the left.
-  The sense of rotation as 1 or -1: 1 for counter-clockwise, -1 for
   clockwise rotation.

The rotation is applied to the sample shape within its own frame. Any goniometer on the
workspace's run is left alone, to be applied by whatever goes on to use the shape; this
algorithm does not enact it. Successive calls compose, so rotating by 90 degrees twice
leaves the sample turned by 180. This algorithm works for both CSG shapes (e.g. cylinders,
flat plates etc.) and Mesh files.

.. note::

   Before release 7.0 this algorithm also multiplied in the run's goniometer, so the shape
   ended up rotated by the requested rotation *and* the goniometer, and for a CSG shape a
   second call replaced the first rather than adding to it. Scripts that relied on either
   behaviour will need updating.


Usage
-----
**Example - RotateSampleShape for sample with a CSG shape**

.. code-block:: python

    from mantid.simpleapi import *
    import xml.dom.minidom as md

    ws = CreateSampleWorkspace()
    SetSample(ws,
    Geometry={'Shape': 'Cylinder', 'Height': 4.0,
                'Radius': 1.0,
                'Center': [0.,0.,0.]},
    Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6',
                'NumberDensity': 0.1})

    RotateSampleShape(Workspace=ws, Axis0="45,1,1,0,1", Axis1="15,0,0,1,-1")

    print(md.parseString(ws.sample().getShape().getShapeXML()).toprettyxml())

.. categories::

.. sourcelink::
