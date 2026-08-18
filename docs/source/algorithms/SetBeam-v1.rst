
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

The following optional flags describe the divergence of the beam. They apply to the
`Slit` setting and must be given together to take effect:

- `HorizontalDivergence`: Angular divergence of the beam in the horizontal plane, in degrees.
- `VerticalDivergence`: Angular divergence of the beam in the vertical plane, in degrees.
- `SlitDistance`: Distance from the defining slit to the sample, in centimeters.

Without these the beam is treated as perfectly collimated, so the illuminated region has
hard edges at the slit aperture. With them the beam spreads as it travels, so the edges are
blurred by an amount that grows with the distance from the slit. The intensity across the
aperture then follows the error function profile of equations 13 and 14 of
Creek, Santisteban & Edwards (2005), with a width of `SlitDistance * tan(divergence)`.

Note that `SlitDistance` is measured from the slit that defines the beam, which is *not*
the source position recorded in the instrument definition - on a long-flight-path instrument
the two can be tens of metres apart.

Usage
-----

.. testcode:: SetBeamExample

   wsSlit = CreateSampleWorkspace()
   SetBeam(wsSlit, Geometry={'Shape': 'Slit', 'Width': 1.0, 'Height': 0.75})

   wsCircle = CreateSampleWorkspace()
   SetBeam(wsCircle, Geometry={'Shape': 'Circle', 'Radius': 1.0})

   wsDivergent = CreateSampleWorkspace()
   SetBeam(wsDivergent, Geometry={'Shape': 'Slit', 'Width': 0.4, 'Height': 0.4,
                                  'HorizontalDivergence': 0.84, 'VerticalDivergence': 0.84,
                                  'SlitDistance': 5.0})

.. categories::

.. sourcelink::
