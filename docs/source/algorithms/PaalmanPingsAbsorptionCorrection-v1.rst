.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm was adapted from :ref:`algm-AbsorptionCorrection`. This
algorithm uses a numerical integration method to calculate
Paalman-Ping attenuation factors resulting from absorption and single
scattering in a sample and container with the material properties
given. Factors are calculated for each spectrum (i.e. detector
position) and wavelength point, as defined by the input workspace. The
sample is first bounded by a cuboid, which is divided up into small
cubes. The cubes whose centres lie within the sample make up the set
of integration elements (so you have a kind of 'Lego' model of the
sample) and path lengths through the sample and container are
calculated for the centre-point of each element, and a numerical
integration is carried out using these path lengths over the volume
elements.

The output workspace will be a :ref:`WorkspaceGroup <WorkspaceGroup>`
containing workpsace with the attenuation factors for :math:`A_{s,s}`,
:math:`A_{s,sc}`, :math:`A_{c,c}` and :math:`A_{c,sc}`. This output
can be applied as the :literal:`CorrectionsWorkspace` input of
:ref:`algm-ApplyPaalmanPingsCorrection`.

Note that the duration of this algorithm is strongly dependent on the
element size chosen, and that too small an element size can cause the
algorithm to fail because of insufficient memory.

Assumptions
###########

This algorithm assumes that the (parallel) beam illuminates the entire
sample **unless** a 'gauge volume' has been defined using the
:ref:`algm-DefineGaugeVolume` algorithm (or by otherwise
adding a valid XML string :ref:`defining a
shape <HowToDefineGeometricShape>` to a :ref:`Run <Run>` property called
"GaugeVolume"). In this latter case only scattering within this volume
(and the sample) is integrated, because this is all the detector can
'see'. The full sample is still used for the neutron paths.

It is worth pointing out that the parameter ``ContainerElementSize`` has very
limited impact on the final absorption correction factor, therefore it is generally
safe to use the same element size for both sample and container regardless of
the actual geometry difference when running this algorithm.

Usage
-----

**Example: A cylinder sample in can**

.. testcode:: Example

    ws = CreateWorkspace(DataX=[1.7981, 1.7983], DataY=[0.0]*4, NSpec=4, UnitX="Wavelength", YUnitLabel="Counts")
    EditInstrumentGeometry(ws,
        PrimaryFlightPath=5.0,
        SpectrumIDs=[1, 2, 3, 4],
        L2=[2.0, 2.0, 2.0, 2.0],
        Polar=[10.0, 90.0, 170.0, 90.0],
        Azimuthal=[0.0, 0.0, 0.0, 45.0],
        DetectorIDs=[1, 2, 3, 4],
        InstrumentName="Instrument")
    SetSample(ws,
        Geometry={"Shape": "Cylinder", "Height": 5.68, "Radius": 0.295},
        Material={"ChemicalFormula": "La-(B11)5.94-(B10)0.06", "SampleNumberDensity": 0.1},
        ContainerGeometry={"Shape": 'HollowCylinder', 'Height': 5.68, 'InnerRadius': 0.295, 'OuterRadius': 0.315},
        ContainerMaterial={"ChemicalFormula": "V", "SampleNumberDensity": 0.0721})
    abs = PaalmanPingsAbsorptionCorrection(ws)
    for a in ["ass", "assc", "acc", "acsc"]:
        print("{:4} {:.4f}(θ=10) {:.4f}(θ=90) {:.4f}(θ=170) {:.4f}(θ=90,φ=45)".format(a, *(mtd["abs_"+a].readY(i)[0] for i in range(4))))

Output:

.. testoutput:: Example

    ass  0.1298(θ=10) 0.1708(θ=90) 0.2119(θ=170) 0.1332(θ=90,φ=45)
    assc 0.1253(θ=10) 0.1650(θ=90) 0.2048(θ=170) 0.1280(θ=90,φ=45)
    acc  0.9418(θ=10) 0.9416(θ=90) 0.9422(θ=170) 0.9311(θ=90,φ=45)
    acsc 0.3258(θ=10) 0.4221(θ=90) 0.5782(θ=170) 0.4014(θ=90,φ=45)

.. categories::

.. sourcelink::
