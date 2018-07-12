
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Set properties of the beam on a given workspace. Current support is limited to
specifying the beam geometry and only supports a slit geometry with given
width and height.

Geometry Flags
--------------

The following `Geometry` flags are recognised by the algorithm:

- `Shape`: A string indicating the geometry type. Only supports `Slit`.
- `Height`: Height of the slit in centimeters
- `Width`: Width of the slit in centimeters

Usage
-----

.. testcode:: SetBeamExample

   ws = CreateSampleWorkspace()
   SetBeam(ws, Geometry={'Shape': 'Slit', 'Width': 1.0, 'Height': 0.75})

.. categories::

.. sourcelink::

