.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


Description
-----------

This algorithm calculates the absorption factors, required for the Paalman Pings method of absorption corrections, using a Monte Carlo procedure. Currently only the acc and ass factors are calculated.

CalculateMonteCarloAbsorption subsequently calls the :ref:`algm-SimpleShapeMonteCarloAbsorption` algorithm for the calculation of each absorption factor.

There are three existing Shape Options: *Flat Plate*, *Annulus* and *Cylinder*. Each shape is defined by a different set of geometric parameters.

Flat Plate parameters: *SampleThickness* and *SampleWidth* for the Sample, *ContainerFrontThickness* and *ContainerBackThickness* for the container.
Annulus parameters: *SampleInnerRadius* and *SampleOuterRadius* for the Sample, *ContainerInnerRadius* and *ContainerOuterRadius* for the container.
Cylinder parameters: *SampleRadius* for the sample, *ContainerInnerRadius* and *ContainerOuterRadius* for the container.

The location and orientation of the sample can be defined with *SampleCenter* and *SampleAngle*.


Workflow
--------

.. diagram:: CalculateMonteCarloAbsorption-v1_wkflw.dot


Usage
-----

**Example - CalculateMonteCarloAbsorption**

.. testcode:: QENSCalculateShapeMonteCarloAbsorption

    sample_ws = CreateSampleWorkspace(Function="Quasielastic",
                                      XUnit="Wavelength",
                                      XMin=-0.5,
                                      XMax=0.5,
                                      BinWidth=0.01)
    # Efixed is generally defined as part of the IDF for real data.
    # Fake it here
    inst_name = sample_ws.getInstrument().getName()
    SetInstrumentParameter(sample_ws, ComponentName=inst_name, 
        ParameterName='Efixed', ParameterType='Number', Value='4.1')

    container_ws = CreateSampleWorkspace(Function="Quasielastic",
                                         XUnit="Wavelength",
                                         XMin=-0.5,
                                         XMax=0.5,
                                         BinWidth=0.01)
    SetInstrumentParameter(container_ws, ComponentName=inst_name,
        ParameterName='Efixed', ParameterType='Number', Value='4.1')

    corrections = CalculateMonteCarloAbsorption(SampleWorkspace = sample_ws,
                                                SampleChemicalFormula = 'H2-O',
                                                SampleDensityType = 'Mass Density',
                                                SampleDensity = 1.0,
                                                ContainerWorkspace = container_ws,
                                                ContainerChemicalFormula = 'Al',
                                                ContainerDensityType = 'Mass Density',
                                                ContainerDensity = 1.0,
                                                EventsPerPoint = 200,
                                                BeamHeight = 3.5,
                                                BeamWidth = 4.0,
                                                Height = 2.0,
                                                Shape = 'FlatPlate',
                                                SampleWidth = 1.4,
                                                SampleThickness = 2.1,
                                                ContainerFrontThickness = 1.2,
                                                ContainerBackThickness = 1.1)

    ass_ws = corrections[0]
    acc_ws = corrections[1]

    print("Workspaces: " + str(ass_ws.getName()) + ", " + str(acc_ws.getName()))
    print("Y-Unit Label of " + str(ass_ws.getName()) + ": " + str(ass_ws.YUnitLabel()))
    print("Y-Unit Label of " + str(acc_ws.getName()) + ": " + str(acc_ws.YUnitLabel()))

.. testcleanup:: QENSCalculateShapeMonteCarloAbsorption

    DeleteWorkspace(sample_ws)
    DeleteWorkspace(container_ws)
    DeleteWorkspace(corrections)

**Output:**

.. testoutput:: QENSCalculateShapeMonteCarloAbsorption

    Workspaces: corrections_ass, corrections_acc
    Y-Unit Label of corrections_ass: Attenuation factor
    Y-Unit Label of corrections_acc: Attenuation factor

.. categories::

.. sourcelink::
