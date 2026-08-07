
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Set properties of the beam on a given workspace. Current support is limited to
specifying the beam geometry, which sets a Slit (rectangular) or Circular
profile, with properties of either width and height, or radius, respectively.

Geometry Flags
--------------

The following `Geometry` flags are recognised by the algorithm:

- `Shape`: A string indicating the geometry type. Supports `Slit` and `Circle`.
- `Height`: Height of the slit in centimeters. Required for the Slit setting.
- `Width`: Width of the slit in centimeters. Required for the Slit setting.
- `Radius`: Radius of the circle in centimeters. Required for the Circle setting.

Usage
-----

.. testcode:: SetBeamExample

   wsSlit = CreateSampleWorkspace()
   SetBeam(wsSlit, Geometry={'Shape': 'Slit', 'Width': 1.0, 'Height': 0.75})

   wsCircle = CreateSampleWorkspace()
   SetBeam(wsCircle, Geometry={'Shape': 'Circle', 'Radius': 1.0})

.. categories::

.. sourcelink::
