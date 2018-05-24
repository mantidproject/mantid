.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The SPB struct within an ISIS raw file defines 4 fields that describe
the basic geometry of the sample:

-  e_geom;
-  e_thick;
-  e_height;
-  e_width.

The meaning of the last three are dependent on the flag value *e_geom*,
which defines the sample shape as one of 4 basic shapes:

-  1 = cylinder: radius = e_thick = e_width, height = e_height;
-  2 = flat plate: as named;
-  3 = disc: radius = e_width, thickness = e_thick;
-  4 = single crystal.

The values are stored on the sample object within the workspace.

Usage
-----

**Example:**

.. testcode:: ExLoadDetails

    filename = "HRP39180.RAW"
    ws = Load(filename)
    LoadSampleDetailsFromRaw(ws,filename)

    s = ws.sample()
    print("Geometry flag {:.0f}".format(s.getGeometryFlag()))
    print("Dimensions H,W,D {:.0f},{:.0f},{:.0f}".format(s.getHeight(),s.getWidth(),s.getThickness()))

Output:

.. testoutput:: ExLoadDetails

    Geometry flag 2
    Dimensions H,W,D 20,15,11

.. categories::

.. sourcelink::
