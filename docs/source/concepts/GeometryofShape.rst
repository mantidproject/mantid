.. _Geometry of Shape:

Geometry of Shape
=================

What is it?
-----------

In Mantid we use `constructive solid geometry
(CSG) <http://en.wikipedia.org/wiki/Constructive_solid_geometry>`__ to
describe the shape of an object. This involves the creation of more
complex shapes by the union, complement or intersection of simple
primitive surfaces.

Why did we use CSG
------------------

Defining our object shape using CSG was selected for a number of
reasons:

#. Using Surfaces based on mathematical equations rather than meshes of
   vertices give much better accuracy when tracking the interaction of
   particles through objects.
#. Scientists think in the shape of objects this way, for example if
   they have a sample that is a sphere radius 0.03m with a conical
   extrusion on top then that is exactly how they describe it in CSG.
   Otherwise they would need to be able to describe the co-ordinates for
   each vertex of the surface.

What shapes can be constructed
------------------------------

Mantid has direct support for creating various shapes directly,
including

-  Sphere
-  Infinite Cylinder
-  Cylinder (finite height)
-  Slice of cylinder ring
-  Infinite Plane
-  Cuboid
-  Infinite Cone

Some of these shapes are infinite surfaces (the infinite plane, cone and
cylinder) these are therefore not very useful on there own, but in
combination with other shapes they can be capped as required.

For example if you cap and infinite Cylinder with two infinite planes
you get a finite capped cylinder. This is in fact how the Cylinder shape
is defined internally within Mantid.

For more on this see
:ref:`HowToDefineGeometricShape <HowToDefineGeometricShape>`.



.. categories:: Concepts