.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm uses numerical integration to approximate the sample correction factor for multiple scattering events
using the formula detailed in the concept page of Absorption and Multiple Scattering Corrections.

Here is a quick recap of the physical model used in the numerical integration.
The scattered neutron can bounce into another atom before leaving the sample when the sample is sufficiently thick, leading to
a double scattering event.

.. figure:: ../images/MultipleScatteringVolume.png
   :alt: MultipleScatteringVolume.png

With the assumption of elastic and isotropic scattering as well as the approximation where we assume the intensity ratio
between incoming and outgoing neutron are a fixed value, the multiple scattering intensity can be expressed as

.. math::
    :label: Im_final

    I_{m} = I_{total} \frac{\rho \sigma_{s} A_{2}}{4 \pi A_{1}}

where :math:`I_{total}` is the measured intensity, :math:`\rho` is the effective number density, and :math:`\sigma_{s}` is the
total scattering cross section.
The term :math:`A_{1}` is a function of wavelength, and can be defined as

.. math::
    :label: A1_final

    A_{1}(\lambda) = \int_V exp[-\mu(\lambda)(l_1 + l_2)] dV

where :math:`\mu` is the sample absorption coefficient (a function of wavelength), :math:`l_1` is the distance from source to
scattering volume one, and :math:`l_2` is the distance from volume two to the detector.
Similarly, :math:`A_{2}` can be defined as

.. math::
    :label: A2_final

    A_2(\lambda) = \int_V\int_V\frac{exp[-\mu(\lambda)(l_1 + l_{12} + l_2)]}{l_{12}^2}dVdV

where :math:`l_{12}` is the distance between volume one and volume two.

This algorithm uses a rasterizer internally to compute all the distances and angles, therefore it is crucial to have the instrument
and sample geometry properly defined within the input workspace.
Also, both :math:`l_1` and :math:`l_{12}` are pre-computed and cached as 1D vector, which tends to occupy a significant amount of
memory when the selected element size is below the standard 1 mm.
Some operating system will stop Mantid from claiming/reserving the memory, leading to a memory allocation error at the starting
stage.
This is a known limitation and we are actively searching for a solution.

By default, the algorithm uses the ``SampleOnly`` method where it ignores container's contribution as well as its interaction with sample.
When ``SampleAndContainer`` is selected, the algorithm will compute the distance within sample and container separately.
Consequently, the term :math:`A_{1}` now becomes

.. math::
    :label: A1_final_sampleAndContainer

    A_{1}(\lambda) = \int \rho \sigma \exp[ -\mu^\text{s} (l_\text{S1}^\text{s} + l_\text{1D}^\text{s}) -\mu^\text{c} (l_\text{S1}^\text{c} + l_\text{1D}^\text{c})] dV

where :math:`\mu^\text{s}` denotes the sample absorption coefficient and :math:`\mu^\text{c}` denotes the container absorption coefficient.
Notice that the number density :math:`\rho` as well as the scattering cross section :math:`\sigma` are inside the integral now.
This is because the material of the sample is often different from the material of the container, therefore we need to consider the material
while performing the integration, which is different from when working with a single material component.

Similarly, the term :math:`A_{2}` is now

.. math::
    :label: A2_final_sampleAndContainer

    A_2(\lambda) = \int \rho_1 \sigma_1
            \int \rho_2 \sigma_2
                 \dfrac{ \exp\left[
                          -\mu^\text{s}( l_\text{S1}^\text{s}
                                       + l_\text{12}^\text{s}
                                       + l_\text{2D}^\text{s})
                          -\mu^\text{c}( l_\text{S1}^\text{c}
                                       + l_\text{12}^\text{c}
                                       + l_\text{2D}^\text{c})
                         \right]
                      }{l_\text{12}^2}
            dV_2
        dV_1

where the distance within different material (sample and container) are summed independently.

Example
-------

The following Python script generate a synthetic data set (instrument and sample) to show case the interface

.. testcode:: SampleOnly

    def make_sample_workspace():
        # Create a fake workspace with TOF data
        sample_ws = CreateSampleWorkspace(Function='Powder Diffraction',
                                        NumBanks=4,
                                        BankPixelWidth=1,
                                        XUnit='TOF',
                                        XMin=1000,
                                        XMax=10000)
        # fake instrument
        EditInstrumentGeometry(sample_ws,
                            PrimaryFlightPath=5.0,
                            SpectrumIDs=[1, 2, 3, 4],
                            L2=[2.0, 2.0, 2.0, 2.0],
                            Polar=[10.0, 90.0, 170.0, 90.0],
                            Azimuthal=[0.0, 0.0, 0.0, 45.0],
                            DetectorIDs=[1, 2, 3, 4],
                            InstrumentName="Instrument")
        return sample_ws

    def add_cylinder_sample_to_workspace(
            ws,
            material,
            number_density,
            mass_density,
            center_bottom_base=[0.0, 0.0, 0.0],  # x,y,z of bottom base of cylinder
            height=0.1,  # in meter
            radius=0.1,  # in meter
    ):
        SetSample(
            ws,
            Geometry={
                "Shape": "Cylinder",
                "centre-of-bottom-base": {
                    "x": center_bottom_base[0],
                    "y": center_bottom_base[1],
                    "z": center_bottom_base[2],
                },
                "Height": height,
                "Radius": radius,
                "Axis": 1,
            },
            Material = {
                "ChemicalFormula": material,
                "SampleNumberDensity": number_density,
                "SampleMassDensity": mass_density,
            }
        )
        return ws

    # use Mutliple scattering correction
    def correction_multiple_scattering(sample_ws, unit="Wavelength"):
        ConvertUnits(InputWorkspace=sample_ws,
                    OutputWorkspace=sample_ws,
                    Target=unit,
                    EMode="Elastic")
        rst = MultipleScatteringCorrection(sample_ws)
        return rst

    # start
    ws = make_sample_workspace()
    ws = add_cylinder_sample_to_workspace(
        ws,
        "V",
        0.07261,
        6.11,
        [0.0, -0.0284, 0.0],
        0.00295,
        0.0568,
    )
    ms_multi = correction_multiple_scattering(ws)

.. testcleanup:: SampleOnly

The following code generates a synthetic data set with instrument, sample and container.
Notice that the output workspace group now contains ``_containerOnly`` and ``_sampleAndContainer`` workspaces.

.. testcode:: SampleAndContainer

    # Create a fake workspace with TOF data
    ws = CreateSampleWorkspace(Function='Powder Diffraction',
                                NumBanks=2,
                                BankPixelWidth=1,
                                XUnit='TOF',
                                XMin=1000,
                                XMax=1500,)
    # Fake instrument
    EditInstrumentGeometry(ws,
                            PrimaryFlightPath=5.0,
                            SpectrumIDs=[1, 2],
                            L2=[2.0, 2.0],
                            Polar=[10.0, 90.0],
                            Azimuthal=[0.0, 45.0],
                            DetectorIDs=[1, 2],
                            InstrumentName="Instrument")
    # set sample and container
    SetSample(
            ws,
            Geometry={
                "Shape": "Cylinder",
                "Center": [0., 0., 0.],
                "Height": 1.0,
                "Radius": 0.2,
            },
            Material = {
                "ChemicalFormula": "La-(B11)5.94-(B10)0.06",
                "SampleNumberDensity": 0.1,
            },
            ContainerMaterial = {
                "ChemicalFormula": "V",
                "SampleNumberDensity": 0.0721,
            },
            ContainerGeometry = {
                "Shape": "HollowCylinder",
                "Height": 1.0,
                "InnerRadius": 0.2,
                "OuterRadius": 0.3,
                "Center": [0., 0., 0.],
            }
        )
    # to wavelength
    ConvertUnits(InputWorkspace=ws,
                    OutputWorkspace=ws,
                    Target="Wavelength",
                    EMode="Elastic")
    # Run the multiple scattering correction
    rst = MultipleScatteringCorrection(
        InputWorkspace = ws,
        Method="SampleAndContainer",
        ElementSize=0.5,  # mm
    )

.. testcleanup:: SampleAndContainer

The following code demonstrates a workaround to deal with thin container walls where a separate
element size for container is needed.

.. testcode:: SampleAndContainerWithDifferentContainerElementSize

    # ----------------------- #
    # make an empty workspace #
    # ----------------------- #
    # Create a fake workspace with TOF data
    ws = CreateSampleWorkspace(Function='Powder Diffraction',
                                NumBanks=2,
                                BankPixelWidth=1,
                                XUnit='TOF',
                                XMin=1000,
                                XMax=10000,)
    # Fake instrument
    EditInstrumentGeometry(ws,
                            PrimaryFlightPath=5.0,
                            SpectrumIDs=[1, 2],
                            L2=[2.0, 2.0],
                            Polar=[10.0, 90.0],
                            Azimuthal=[0.0, 45.0],
                            DetectorIDs=[1, 2],
                            InstrumentName="Instrument")
    # set sample and container
    SetSample(
            ws,
            Geometry={
                "Shape": "Cylinder",
                "Center": [0., 0., 0.],
                "Height": 1.0,  # cm
                "Radius": 0.3,  # cm
            },
            Material = {
                "ChemicalFormula": "La-(B11)5.94-(B10)0.06",
                "SampleNumberDensity": 0.1,
            },
            ContainerMaterial = {
                "ChemicalFormula": "V",
                "SampleNumberDensity": 0.0721,
            },
            ContainerGeometry = {
                "Shape": "HollowCylinder",
                "Height": 1.0,  # cm
                "InnerRadius": 0.3,  # cm
                "OuterRadius": 0.35,  # cm
                "Center": [0., 0., 0.],
            }
        )
    # to wavelength
    ConvertUnits(InputWorkspace=ws,
                    OutputWorkspace=ws,
                    Target="Wavelength",
                    EMode="Elastic")
    # call
    MultipleScatteringCorrection(
        InputWorkspace = ws,
        Method="SampleAndContainer",
        ElementSize=0.5,
        ContainerElementSize=0.45,
        OutputWorkspace="rst",
    )

.. testcleanup:: SampleAndContainerWithDifferentContainerElementSize

.. categories::

.. sourcelink::
