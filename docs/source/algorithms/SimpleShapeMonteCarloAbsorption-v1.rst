
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Sets up a sample shape, along with the required material properties, and runs
the :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption-v1>` algorithm. This algorithm merely
serves as a simpler interface to define the shape & material of the sample without having
to resort to the more complex :ref:`CreateSampleShape <algm-CreateSampleShape-v1>` & :ref:`SetSampleMaterial <algm-SetSampleMaterial-v1>`
algorithms. The computational part is all taken care of by :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption-v1>`. Please see that
documentation for more details.
Currently the shape geometries supported are:

* Flat Plate
* Cylinder
* Annulus

Workflow
--------

.. diagram:: SimpleShapeMonteCarloAbsorption-v1_wkflw.dot

Usage
-----

**Example**

.. testcode:: QENSSimpleShapeMonteCarloAbsorption

    qens_ws = CreateSampleWorkspace(Function="Quasielastic",
                                    XUnit="Wavelength",
                                    XMin=-0.5,
                                    XMax=0.5,
                                    BinWidth=0.01)

    corrected = SimpleShapeMonteCarloAbsorption(InputWorkspace = qens_ws,
                                                ChemicalFormula = 'H2-O',
                                                DensityType = 'Mass Density',
                                                Density = 1.0,
                                                EventsPerPoint = 200,
                                                BeamHeight = 3.5,
                                                BeamWidth = 4.0,
                                                Height = 2.0,
                                                Shape = 'FlatPlate',
                                                Width = 1.4,
                                                Thickness = 2.1)

    print("y-axis label: {}".format(corrected.YUnitLabel()))

.. testcleanup:: QENSSimpleShapeMonteCarloAbsorption

    DeleteWorkspace(qens_ws)
    DeleteWorkspace(corrected)

**Output:**

.. testoutput:: QENSSimpleShapeMonteCarloAbsorption

    y-axis label: Attenuation factor

.. categories::

.. sourcelink::
