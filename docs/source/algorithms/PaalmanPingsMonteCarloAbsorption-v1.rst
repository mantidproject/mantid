.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the attenuation factors in the Paalman Pings formalism using a Monte Carlo simulation.

Three simple shapes are supported: *FlatPlate*, *Cylinder*, and *Annulus*. Each shape is defined by the corresponding set of geometric parameters.

If the *Preset* shape option is chosen, the algorithm will use the sample and container shapes defined in the workspace.

Inelastic, quasielastic and elastic measurements are supported, both for direct and indirect geometry instruments.

*Height* is a common property for the sample and container regardless the shape.

Below are the required properties for each specific shape:

FlatPlate
#########

- *SampleThickness*
- *SampleWidth*
- *SampleCenter*
- *SampleAngle*
- *ContainerFrontThickness*
- *ContainerBackThickness*

Cylinder
########

- *SampleRadius*
- *ContainerRadius* stands for the outer radius of the annular container, inner radius of which is the same as the radius of the sample.

Annulus
#######

- *ContainerInnerRadius*
- *SampleInnerRadius*
- *SampleOuterRadius*
- *ContainerOuterRadius*

The sample and container shapes and materials are set by :ref:`SetSample <algm-SetSample>`.

The actual calculations are performed using :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` for each individual correction term.

The corrections should be applied by the :ref:`ApplyPaalmanPingsCorrection <algm-ApplyPaalmanPingsCorrection>`, where you can find further documentation on the signification of the correction terms and the method.

.. note::

  When container is specified, it is assumed to be tightly wrapping the sample of corresponding shape; that is, there is no gap between the sample and the container.

.. note::

  All the shape arguments are supposed to be in centimeters.


Usage
-----

**Example - FlatPlate**

.. testcode:: FlatPlate

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

    corrections = PaalmanPingsMonteCarloAbsorption(
            InputWorkspace=sample_ws,
            Shape='FlatPlate',
            BeamHeight=2.0,
            BeamWidth=2.0,
            Height=2.0,
            SampleWidth=2.0,
            SampleThickness=0.1,
            SampleChemicalFormula='H2-O',
            SampleDensity=1.0,
            ContainerFrontThickness=0.02,
            ContainerBackThickness=0.02,
            ContainerChemicalFormula='V',
            ContainerDensity=6.0,
            CorrectionsWorkspace='flat_plate_corr'
        )

    ass_ws = corrections[0]
    assc_ws = corrections[1]
    acsc_ws = corrections[2]
    acc_ws = corrections[3]

    print("Y-Unit Label of " + str(ass_ws.getName()) + ": " + str(ass_ws.YUnitLabel()))
    print("Y-Unit Label of " + str(assc_ws.getName()) + ": " + str(assc_ws.YUnitLabel()))
    print("Y-Unit Label of " + str(acsc_ws.getName()) + ": " + str(acsc_ws.YUnitLabel()))
    print("Y-Unit Label of " + str(acc_ws.getName()) + ": " + str(acc_ws.YUnitLabel()))

.. testcleanup:: FlatPlate

    mtd.clear()

.. testoutput:: FlatPlate

    Y-Unit Label of flat_plate_corr_ass: Attenuation factor
    Y-Unit Label of flat_plate_corr_assc: Attenuation factor
    Y-Unit Label of flat_plate_corr_acsc: Attenuation factor
    Y-Unit Label of flat_plate_corr_acc: Attenuation factor

**Example - Cylinder**

.. testcode:: Cylinder

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

    corrections = PaalmanPingsMonteCarloAbsorption(
            InputWorkspace=sample_ws,
            Shape='Cylinder',
            BeamHeight=2.0,
            BeamWidth=2.0,
            Height=2.0,
            SampleRadius=0.2,
            SampleChemicalFormula='H2-O',
            SampleDensity=1.0,
            ContainerRadius=0.22,
            ContainerChemicalFormula='V',
            ContainerDensity=6.0,
            CorrectionsWorkspace='cylinder_corr'
        )

    ass_ws = corrections[0]
    assc_ws = corrections[1]
    acsc_ws = corrections[2]
    acc_ws = corrections[3]

    print("Y-Unit Label of " + str(ass_ws.getName()) + ": " + str(ass_ws.YUnitLabel()))
    print("Y-Unit Label of " + str(assc_ws.getName()) + ": " + str(assc_ws.YUnitLabel()))
    print("Y-Unit Label of " + str(acsc_ws.getName()) + ": " + str(acsc_ws.YUnitLabel()))
    print("Y-Unit Label of " + str(acc_ws.getName()) + ": " + str(acc_ws.YUnitLabel()))

.. testcleanup:: Cylinder

    mtd.clear()

.. testoutput:: Cylinder

    Y-Unit Label of cylinder_corr_ass: Attenuation factor
    Y-Unit Label of cylinder_corr_assc: Attenuation factor
    Y-Unit Label of cylinder_corr_acsc: Attenuation factor
    Y-Unit Label of cylinder_corr_acc: Attenuation factor

**Example - Annulus**

.. testcode:: Annulus

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

    corrections = PaalmanPingsMonteCarloAbsorption(
            InputWorkspace=sample_ws,
            Shape='Annulus',
            BeamHeight=2.0,
            BeamWidth=2.0,
            Height=2.0,
            SampleInnerRadius=0.2,
            SampleOuterRadius=0.4,
            SampleChemicalFormula='H2-O',
            SampleDensity=1.0,
            ContainerInnerRadius=0.19,
            ContainerOuterRadius=0.41,
            ContainerChemicalFormula='V',
            ContainerDensity=6.0,
            CorrectionsWorkspace='annulus_corr'
        )

    ass_ws = corrections[0]
    assc_ws = corrections[1]
    acsc_ws = corrections[2]
    acc_ws = corrections[3]

    print("Y-Unit Label of " + str(ass_ws.getName()) + ": " + str(ass_ws.YUnitLabel()))
    print("Y-Unit Label of " + str(assc_ws.getName()) + ": " + str(assc_ws.YUnitLabel()))
    print("Y-Unit Label of " + str(acsc_ws.getName()) + ": " + str(acsc_ws.YUnitLabel()))
    print("Y-Unit Label of " + str(acc_ws.getName()) + ": " + str(acc_ws.YUnitLabel()))

.. testcleanup:: Annulus

    mtd.clear()

.. testoutput:: Annulus

    Y-Unit Label of annulus_corr_ass: Attenuation factor
    Y-Unit Label of annulus_corr_assc: Attenuation factor
    Y-Unit Label of annulus_corr_acsc: Attenuation factor
    Y-Unit Label of annulus_corr_acc: Attenuation factor

**Example - Preset Shape**

.. testcode:: Preset

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

    # define example geometries for the Sample and Container
    SetSample(sample_ws, Geometry={'Shape': 'Cylinder', 'Height': 4.0, 'Radius': 2.0, 'Center': [0.,0.,0.]},
                Material={'ChemicalFormula': 'Ni', 'MassDensity': 7.0},
                ContainerGeometry={'Shape': 'HollowCylinder', 'Height': 4.0, 'InnerRadius': 2.0,
                'OuterRadius': 3.5},
                ContainerMaterial={'ChemicalFormula': 'V'})

    corrections = PaalmanPingsMonteCarloAbsorption(
            InputWorkspace=sample_ws,
            Shape='Preset',
            BeamHeight=2.0,
            BeamWidth=2.0,
            CorrectionsWorkspace='annulus_corr'
        )

    # alternatively, run the corrections with the sample material overridden but preset shape used
    corrections = PaalmanPingsMonteCarloAbsorption(
            InputWorkspace=sample_ws,
            Shape='Preset',
            BeamHeight=2.0,
            BeamWidth=2.0,
            SampleChemicalFormula='H2-O',
            SampleDensity=1.0,
            CorrectionsWorkspace='annulus_corr'
        )

    ass_ws = corrections[0]
    assc_ws = corrections[1]
    acsc_ws = corrections[2]
    acc_ws = corrections[3]

    print("Y-Unit Label of " + str(ass_ws.getName()) + ": " + str(ass_ws.YUnitLabel()))
    print("Y-Unit Label of " + str(assc_ws.getName()) + ": " + str(assc_ws.YUnitLabel()))
    print("Y-Unit Label of " + str(acsc_ws.getName()) + ": " + str(acsc_ws.YUnitLabel()))
    print("Y-Unit Label of " + str(acc_ws.getName()) + ": " + str(acc_ws.YUnitLabel()))

.. testcleanup:: Preset

    mtd.clear()

.. testoutput:: Preset

    Y-Unit Label of annulus_corr_ass: Attenuation factor
    Y-Unit Label of annulus_corr_assc: Attenuation factor
    Y-Unit Label of annulus_corr_acsc: Attenuation factor
    Y-Unit Label of annulus_corr_acc: Attenuation factor

References
----------

#. H. H. Paalman, and C. J. Pings. *Numerical Evaluation of X‐Ray
   Absorption Factors for Cylindrical Samples and Annular Sample Cells*,
   Journal of Applied Physics **33.8** (1962) 2635–2639
   `doi: 10.1063/1.1729034 <http://dx.doi.org/10.1063/1.1729034>`_

.. categories::

.. sourcelink::
