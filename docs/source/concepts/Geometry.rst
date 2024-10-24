.. _Geometry:

Geometry
========

What is it?
-----------

Geometry is the description of the physical shape (volume) of an object
within a Mantid :ref:`instrument <Instrument>` and the distances and
rotations between them.

Geometry in Mantid
------------------

In Mantid we separate the :ref:`Geometry of the shape <Geometry of Shape>`
of an object from the :ref:`Geometry of it's
position <Geometry of Position>`. This is done primarily to save on
memory usage but also to improve performance. Many operations within
Mantid need to know where for example a detector is, but do not need to
know what shape it is. By keeping the Geometry and Position separate we
can keep the performance high. Also while in any one instrument we may
have over 10,000 detector pixels all with different locations they will
all have the same shape, therefore by keeping the shape separate we can
greatly reduce the amount of memory required.

Basics of Geometry
------------------

Both of the forms of Geometry share certain basic concepts. These are
that co-ordinates are stored as `3D
Vectors <http://en.wikipedia.org/wiki/Vector_(spatial)>`__ (the class in
Mantid is called V3D), directions are described as similar unit 3D
Vectors, and rotations are described using
`quaternions <http://en.wikipedia.org/wiki/Quaternion>`__.



.. categories:: Concepts
