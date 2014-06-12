.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The SPB struct within an ISIS raw file defines 4 fields that describe
the basic geometry of the sample:

-  e\_geom;
-  e\_thick;
-  e\_height;
-  e\_width.

The meaning of the last three are dependent on the flag value *e\_geom*,
which defines the sample shape as one of 4 basic shapes:

-  1 = cylinder: radius = e\_thick = e\_width, height = e\_height;
-  2 = flat plate: as named;
-  3 = disc: radius = e\_width, thickness = e\_thick;
-  4 = single crystal.

The values are stored on the
`sample <http://doxygen.mantidproject.org/classMantid_1_1API_1_1Sample.html#a07df5ce7be639c3cb67f33f5e1c7493f>`__
object.

Access in Python
----------------

To access these values in Python:

| ``sampleInfo = wksp.getSampleInfo()``
| ``print sampleInfo.getGeometryFlag()``
| ``print sampleInfo.getThickness()``
| ``print sampleInfo.getHeight()``
| ``print sampleInfo.getWidth()``

where wksp is a handle to a Mantid workspace.

.. categories::
