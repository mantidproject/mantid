
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

The sample shape would then be rotated by the goniometer before being used in the
calculation of various attenuation corrections. This algorithm work for both CSG shapes
(e.g. cylinders, flat plates etc.) and Mesh files.


Usage
-----
**Example - RotateSampleShape for sample with a CSG shape**

.. testcode:: RotateSampleShape

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

Output:

.. testoutput:: RotateSampleShape

    <?xml version="1.0" ?>
    <type name="userShape">

        <cylinder id="sample-shape">

            <centre-of-bottom-base x="0" y="-0.02" z="0"/>

            <axis x="0" y="1" z="0"/>
            <height val="0.04"/>

            <radius val="0.01"/>
        </cylinder>

        <goniometer a11="0.786566" a12="0.362372" a13="0.500000" a21="-0.079459" a22="0.862372" a23="-0.500000" a31="-0.612372" a32="0.353553" a33="0.707107"/>
    </type>

.. testcleanup:: RotateSampleShape

    DeleteWorkspace(ws)

.. categories::

.. sourcelink::

