# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    DataProcessorAlgorithm,
    AlgorithmFactory,
    PropertyMode,
    WorkspaceGroupProperty,
    SpectraAxis,
    WorkspaceGroup,
    WorkspaceProperty,
    Sample,
)
from mantid.kernel import (
    VisibleWhenProperty,
    PropertyCriterion,
    StringListValidator,
    IntBoundedValidator,
    FloatBoundedValidator,
    Direction,
    LogicOperator,
    EnabledWhenProperty,
)
from mantid.simpleapi import config, logger, mtd
import math
import numpy as np
import os.path


class PaalmanPingsMonteCarloAbsorption(DataProcessorAlgorithm):
    # General variables
    _input_ws = None
    _emode = None
    _efixed = None
    _general_kwargs = None
    _shape = None
    _height = None
    _isis_instrument = None
    _has_container = None

    # Sample variables
    _sample_angle = None
    _sample_center = None
    _sample_chemical_formula = None
    _sample_density = None
    _sample_density_type = None
    _sample_inner_radius = None
    _sample_outer_radius = None
    _sample_radius = None
    _sample_thickness = None
    _sample_unit = None
    _sample_width = None
    _sample_shape = None

    # Container variables
    _container_angle = None
    _container_center = None
    _container_chemical_formula = None
    _container_density = None
    _container_density_type = None
    _container_radius = None
    _container_inner_radius = None
    _container_outer_radius = None
    _container_thickness = None
    _container_width = None
    _sample_env = None

    # Output workspaces
    _ass_ws = None
    _acc_ws = None
    _assc_ws = None
    _acsc_ws = None
    _output_ws = None

    def category(self):
        return "Workflow\\Inelastic;CorrectionFunctions\\AbsorptionCorrections;Workflow\\MIDAS"

    def seeAlso(self):
        return ["MonteCarloAbsorption", "ApplyPaalmanPingsCorrection", "PaalmanPingsAbsorptionCorrection"]

    def summary(self):
        return (
            "Calculates absorption corrections in Paalman & Pings formalism for a given sample "
            "and optionally its environment using a Monte Carlo simulation."
        )

    def checkGroups(self):
        return False

    def PyInit(self):
        # Sample Input
        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", direction=Direction.Input),
            doc="Workspace with the measurement of the sample [in a container].",
        )

        # Monte Carlo Options
        self.declareProperty(name="EventsPerPoint", defaultValue=1000, validator=IntBoundedValidator(0), doc="Number of neutron events")
        self.declareProperty(
            name="Interpolation", defaultValue="Linear", validator=StringListValidator(["Linear", "CSpline"]), doc="Type of interpolation"
        )
        self.declareProperty(
            name="MaxScatterPtAttempts",
            defaultValue=5000,
            validator=IntBoundedValidator(0),
            doc="Maximum number of tries made to generate a scattering point",
        )
        self.declareProperty(
            name="SparseInstrument", defaultValue=False, doc="Whether to spatially approximate the instrument for faster calculation."
        )
        self.declareProperty(
            name="NumberOfDetectorRows",
            defaultValue=3,
            validator=IntBoundedValidator(lower=3),
            doc="Number of detector rows in the detector grid of the sparse instrument.",
        )
        self.declareProperty(
            name="NumberOfDetectorColumns",
            defaultValue=2,
            validator=IntBoundedValidator(lower=2),
            doc="Number of detector columns in the detector grid of the sparse instrument.",
        )

        sparse_condition = EnabledWhenProperty("SparseInstrument", PropertyCriterion.IsNotDefault)
        self.setPropertySettings("NumberOfDetectorRows", sparse_condition)
        self.setPropertySettings("NumberOfDetectorColumns", sparse_condition)

        self.setPropertyGroup("SparseInstrument", "Monte Carlo Options")
        self.setPropertyGroup("NumberOfDetectorRows", "Monte Carlo Options")
        self.setPropertyGroup("NumberOfDetectorColumns", "Monte Carlo Options")
        self.setPropertyGroup("EventsPerPoint", "Monte Carlo Options")
        self.setPropertyGroup("Interpolation", "Monte Carlo Options")
        self.setPropertyGroup("MaxScatterPtAttempts", "Monte Carlo Options")

        # Beam Options
        self.declareProperty(name="BeamHeight", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Height of the beam (cm)")
        self.declareProperty(name="BeamWidth", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Width of the beam (cm)")

        self.setPropertyGroup("BeamHeight", "Beam Options")
        self.setPropertyGroup("BeamWidth", "Beam Options")

        # Shape Options
        self.declareProperty(
            name="Shape",
            defaultValue="Preset",
            validator=StringListValidator(["Preset", "FlatPlate", "Cylinder", "Annulus"]),
            doc="Geometric shape of the sample environment",
        )

        not_preset_condition = EnabledWhenProperty("Shape", PropertyCriterion.IsNotEqualTo, "Preset")
        flat_plate_condition = VisibleWhenProperty("Shape", PropertyCriterion.IsEqualTo, "FlatPlate")
        cylinder_condition = VisibleWhenProperty("Shape", PropertyCriterion.IsEqualTo, "Cylinder")
        annulus_condition = VisibleWhenProperty("Shape", PropertyCriterion.IsEqualTo, "Annulus")

        # show flat plate as visible but disabled if preset shape chosen, to avoid empty group on alg dialogue
        preset_condition = VisibleWhenProperty("Shape", PropertyCriterion.IsEqualTo, "Preset")
        flat_plate_visible = VisibleWhenProperty(preset_condition, flat_plate_condition, LogicOperator.Or)

        # height is common to all shapes, and should be the same for sample and container
        self.declareProperty("Height", defaultValue=0.0, validator=FloatBoundedValidator(0.0), doc="Height of the sample environment (cm)")
        self.setPropertySettings("Height", not_preset_condition)

        self.setPropertyGroup("Shape", "Shape Options")
        self.setPropertyGroup("Height", "Shape Options")

        # Sample Shape
        # Flat Plate
        self.declareProperty(
            name="SampleWidth", defaultValue=0.0, validator=FloatBoundedValidator(0.0), doc="Width of the sample environment (cm)"
        )
        self.declareProperty(
            name="SampleThickness", defaultValue=0.0, validator=FloatBoundedValidator(0.0), doc="Thickness of the sample environment (cm)"
        )
        self.declareProperty(name="SampleCenter", defaultValue=0.0, doc="Center of the sample environment")
        self.declareProperty(
            name="SampleAngle",
            defaultValue=0.0,
            validator=FloatBoundedValidator(-180.0, 180.0),
            doc="Angle of the sample environment with respect to the beam (degrees)",
        )

        self.setPropertySettings("SampleWidth", flat_plate_visible)
        self.setPropertySettings("SampleThickness", flat_plate_visible)
        self.setPropertySettings("SampleCenter", flat_plate_visible)
        self.setPropertySettings("SampleAngle", flat_plate_visible)
        self.setPropertySettings("SampleWidth", not_preset_condition)
        self.setPropertySettings("SampleThickness", not_preset_condition)
        self.setPropertySettings("SampleCenter", not_preset_condition)
        self.setPropertySettings("SampleAngle", not_preset_condition)

        self.setPropertyGroup("SampleWidth", "Sample Shape")
        self.setPropertyGroup("SampleThickness", "Sample Shape")
        self.setPropertyGroup("SampleCenter", "Sample Shape")
        self.setPropertyGroup("SampleAngle", "Sample Shape")

        # Cylinder
        self.declareProperty(
            name="SampleRadius", defaultValue=0.0, validator=FloatBoundedValidator(0.0), doc="Radius of the sample environment (cm)"
        )

        self.setPropertySettings("SampleRadius", cylinder_condition)
        self.setPropertyGroup("SampleRadius", "Sample Shape")

        # Annulus
        self.declareProperty(
            name="SampleInnerRadius",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="Inner radius of the sample environment (cm)",
        )
        self.declareProperty(
            name="SampleOuterRadius",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="Outer radius of the sample environment (cm)",
        )

        self.setPropertySettings("SampleInnerRadius", annulus_condition)
        self.setPropertySettings("SampleOuterRadius", annulus_condition)

        self.setPropertyGroup("SampleInnerRadius", "Sample Shape")
        self.setPropertyGroup("SampleOuterRadius", "Sample Shape")

        # Sample Material
        self.declareProperty(name="SampleChemicalFormula", defaultValue="", doc="Chemical formula for the sample material")
        self.declareProperty(
            name="SampleCoherentXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The coherent cross-section for the sample material in barns. To be used instead of " "Chemical Formula.",
        )
        self.declareProperty(
            name="SampleIncoherentXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The incoherent cross-section for the sample material in barns. To be used instead of " "Chemical Formula.",
        )
        self.declareProperty(
            name="SampleAttenuationXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The absorption cross-section for the sample material in barns. To be used instead of " "Chemical Formula.",
        )
        self.declareProperty(
            name="SampleDensityType",
            defaultValue="Mass Density",
            validator=StringListValidator(["Mass Density", "Number Density"]),
            doc="Use of Mass density or Number density for the sample.",
        )
        self.declareProperty(
            name="SampleNumberDensityUnit",
            defaultValue="Atoms",
            validator=StringListValidator(["Atoms", "Formula Units"]),
            doc="Choose which units SampleDensity refers to. Allowed values: [Atoms, Formula Units]",
        )
        self.declareProperty(
            name="SampleDensity",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The value for the sample Mass density (g/cm^3) or Number density (1/Angstrom^3).",
        )

        self.setPropertyGroup("SampleChemicalFormula", "Sample Material")
        self.setPropertyGroup("SampleCoherentXSection", "Sample Material")
        self.setPropertyGroup("SampleIncoherentXSection", "Sample Material")
        self.setPropertyGroup("SampleAttenuationXSection", "Sample Material")
        self.setPropertyGroup("SampleDensityType", "Sample Material")
        self.setPropertyGroup("SampleNumberDensityUnit", "Sample Material")
        self.setPropertyGroup("SampleDensity", "Sample Material")

        # Flat Plate
        self.declareProperty(
            name="ContainerFrontThickness",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="Front thickness of the container environment (cm)",
        )
        self.declareProperty(
            name="ContainerBackThickness",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="Back thickness of the container environment (cm)",
        )

        self.setPropertySettings("ContainerFrontThickness", flat_plate_condition)
        self.setPropertySettings("ContainerBackThickness", flat_plate_condition)
        self.setPropertySettings("ContainerFrontThickness", not_preset_condition)
        self.setPropertySettings("ContainerBackThickness", not_preset_condition)
        self.setPropertyGroup("ContainerFrontThickness", "Container Shape")
        self.setPropertyGroup("ContainerBackThickness", "Container Shape")

        # Cylinder
        self.declareProperty(
            name="ContainerRadius",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="Outer radius of the sample environment (cm)",
        )

        self.setPropertySettings("ContainerRadius", cylinder_condition)
        self.setPropertyGroup("ContainerRadius", "Container Shape")

        # Annulus
        self.declareProperty(
            name="ContainerInnerRadius",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="Inner radius of the container environment (cm)",
        )
        self.declareProperty(
            name="ContainerOuterRadius",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="Outer radius of the container environment (cm)",
        )

        self.setPropertySettings("ContainerInnerRadius", annulus_condition)
        self.setPropertySettings("ContainerOuterRadius", annulus_condition)

        self.setPropertyGroup("ContainerInnerRadius", "Container Shape")
        self.setPropertyGroup("ContainerOuterRadius", "Container Shape")

        # Container Material
        self.declareProperty(name="ContainerChemicalFormula", defaultValue="", doc="Chemical formula for the container material")
        self.declareProperty(
            name="ContainerCoherentXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The coherent cross-section for the can material in barns. To be used instead of " "Chemical Formula.",
        )
        self.declareProperty(
            name="ContainerIncoherentXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The incoherent cross-section for the can material in barns. To be used instead of " "Chemical Formula.",
        )
        self.declareProperty(
            name="ContainerAttenuationXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The absorption cross-section for the can material in barns. To be used instead of " "Chemical Formula.",
        )
        self.declareProperty(
            name="ContainerDensityType",
            defaultValue="Mass Density",
            validator=StringListValidator(["Mass Density", "Number Density"]),
            doc="Use of Mass density or Number density for the container.",
        )
        self.declareProperty(
            name="ContainerNumberDensityUnit",
            defaultValue="Atoms",
            validator=StringListValidator(["Atoms", "Formula Units"]),
            doc="Choose which units ContainerDensity refers to. Allowed values: [Atoms, Formula Units]",
        )
        self.declareProperty(
            name="ContainerDensity",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The value for the container Mass density (g/cm^3) or Number density (1/Angstrom^3).",
        )

        self.setPropertyGroup("ContainerChemicalFormula", "Container Material")
        self.setPropertyGroup("ContainerCoherentXSection", "Container Material")
        self.setPropertyGroup("ContainerIncoherentXSection", "Container Material")
        self.setPropertyGroup("ContainerAttenuationXSection", "Container Material")
        self.setPropertyGroup("ContainerDensityType", "Container Material")
        self.setPropertyGroup("ContainerNumberDensityUnit", "Container Material")
        self.setPropertyGroup("ContainerDensity", "Container Material")
        self.setPropertySettings("ContainerChemicalFormula", not_preset_condition)
        self.setPropertySettings("ContainerCoherentXSection", not_preset_condition)
        self.setPropertySettings("ContainerIncoherentXSection", not_preset_condition)
        self.setPropertySettings("ContainerAttenuationXSection", not_preset_condition)
        self.setPropertySettings("ContainerDensityType", not_preset_condition)
        self.setPropertySettings("ContainerNumberDensityUnit", not_preset_condition)
        self.setPropertySettings("ContainerDensity", not_preset_condition)

        # Output Workspace Group
        self.declareProperty(
            WorkspaceGroupProperty(
                name="CorrectionsWorkspace", defaultValue="corrections", direction=Direction.Output, optional=PropertyMode.Optional
            ),
            doc="Name of the workspace group to save correction factors",
        )

    def PyExec(self):
        progess_steps = 1.0 if not self._has_can else 0.25
        input_wave_ws = self._convert_to_wavelength(self._input_ws)
        self._set_beam(input_wave_ws)

        self._sample_shape = input_wave_ws.sample().getShape()
        if input_wave_ws.sample().hasEnvironment():
            self._sample_env = input_wave_ws.sample().getEnvironment()
        # make sure there is no container defined at this point
        self._set_sample(input_wave_ws, ["Sample"])
        monte_carlo_alg = self.createChildAlgorithm("MonteCarloAbsorption", enableLogging=True, startProgress=0, endProgress=progess_steps)
        self._set_algorithm_properties(monte_carlo_alg, self._monte_carlo_kwargs)
        monte_carlo_alg.setProperty("InputWorkspace", input_wave_ws)
        monte_carlo_alg.setProperty("OutputWorkspace", self._ass_ws_name)
        monte_carlo_alg.setProperty("SimulateScatteringPointIn", "SampleOnly")
        monte_carlo_alg.execute()
        ass_ws = monte_carlo_alg.getProperty("OutputWorkspace").value
        ass_ws = self._convert_from_wavelength(ass_ws)
        mtd.addOrReplace(self._ass_ws_name, ass_ws)
        self._output_ws = self._group_ws([ass_ws])

        if self._has_can:
            self._set_sample(input_wave_ws, ["Sample", "Container"])
            monte_carlo_alg_ssc = self.createChildAlgorithm(
                "MonteCarloAbsorption", enableLogging=True, startProgress=progess_steps, endProgress=2 * progess_steps
            )
            self._set_algorithm_properties(monte_carlo_alg_ssc, self._monte_carlo_kwargs)
            monte_carlo_alg_ssc.setProperty("InputWorkspace", input_wave_ws)
            monte_carlo_alg_ssc.setProperty("OutputWorkspace", self._assc_ws_name)
            monte_carlo_alg_ssc.setProperty("SimulateScatteringPointIn", "SampleOnly")
            monte_carlo_alg_ssc.execute()
            assc_ws = monte_carlo_alg_ssc.getProperty("OutputWorkspace").value
            assc_ws = self._convert_from_wavelength(assc_ws)
            mtd.addOrReplace(self._assc_ws_name, assc_ws)

            self._set_sample(input_wave_ws, ["Container"])
            monte_carlo_alg_cc = self.createChildAlgorithm(
                "MonteCarloAbsorption", enableLogging=True, startProgress=2 * progess_steps, endProgress=3 * progess_steps
            )
            self._set_algorithm_properties(monte_carlo_alg_cc, self._monte_carlo_kwargs)
            monte_carlo_alg_cc.setProperty("InputWorkspace", input_wave_ws)
            monte_carlo_alg_cc.setProperty("OutputWorkspace", self._acc_ws_name)
            monte_carlo_alg_cc.setProperty("SimulateScatteringPointIn", "EnvironmentOnly")
            monte_carlo_alg_cc.execute()
            acc_ws = monte_carlo_alg_cc.getProperty("OutputWorkspace").value
            acc_ws = self._convert_from_wavelength(acc_ws)
            mtd.addOrReplace(self._acc_ws_name, acc_ws)

            self._set_sample(input_wave_ws, ["Sample", "Container"])
            monte_carlo_alg_csc = self.createChildAlgorithm(
                "MonteCarloAbsorption", enableLogging=True, startProgress=3 * progess_steps, endProgress=1.0
            )
            self._set_algorithm_properties(monte_carlo_alg_csc, self._monte_carlo_kwargs)
            monte_carlo_alg_csc.setProperty("InputWorkspace", input_wave_ws)
            monte_carlo_alg_csc.setProperty("OutputWorkspace", self._acsc_ws_name)
            monte_carlo_alg_csc.setProperty("SimulateScatteringPointIn", "EnvironmentOnly")
            monte_carlo_alg_csc.execute()
            acsc_ws = monte_carlo_alg_csc.getProperty("OutputWorkspace").value
            acsc_ws = self._convert_from_wavelength(acsc_ws)
            mtd.addOrReplace(self._acsc_ws_name, acsc_ws)

            self._output_ws = self._group_ws([ass_ws, assc_ws, acsc_ws, acc_ws])

        self.setProperty("CorrectionsWorkspace", self._output_ws)

    def _set_beam(self, ws):
        set_beam_alg = self.createChildAlgorithm("SetBeam", enableLogging=False)
        set_beam_alg.setProperty("InputWorkspace", ws)
        set_beam_alg.setProperty("Geometry", {"Shape": "Slit", "Width": self._beam_width, "Height": self._beam_height})
        set_beam_alg.execute()

    def _set_sample(self, ws, components):
        ws.setSample(Sample())

        sample_geometry = dict()
        sample_material = dict()
        container_geometry = dict()
        container_material = dict()

        if "Sample" in components:
            if self._shape == "Preset":
                ws.sample().setShape(self._sample_shape)
            else:
                sample_geometry["Height"] = self._height

                if self._shape == "FlatPlate":
                    sample_geometry["Shape"] = "FlatPlate"
                    sample_geometry["Width"] = self._sample_width
                    sample_geometry["Thick"] = self._sample_thickness
                    sample_geometry["Center"] = [0.0, 0.0, self._sample_center]
                    sample_geometry["Angle"] = self._sample_angle

                if self._shape == "Cylinder":
                    sample_geometry["Shape"] = "Cylinder"
                    sample_geometry["Radius"] = self._sample_radius
                    sample_geometry["Center"] = [0.0, 0.0, 0.0]

                if self._shape == "Annulus":
                    sample_geometry["Shape"] = "HollowCylinder"
                    sample_geometry["InnerRadius"] = self._sample_inner_radius
                    sample_geometry["OuterRadius"] = self._sample_outer_radius
                    sample_geometry["Center"] = [0.0, 0.0, 0.0]

            # may want to override material even if shape=Preset
            sample_material = self._set_material_dict("sample")

        if "Container" in components:
            if self._shape == "Preset":
                ws.sample().setEnvironment(self._sample_env)
            else:
                container_geometry["Height"] = self._height

                if self._shape == "FlatPlate":
                    container_geometry["Shape"] = "FlatPlateHolder"
                    container_geometry["Width"] = self._sample_width
                    # we need to know the thickness and the centre of the sample as well,
                    # in order to calculate the centre offsets for the container panels
                    container_geometry["Thick"] = self._sample_thickness
                    container_geometry["Center"] = [0.0, 0.0, self._sample_center]
                    container_geometry["Angle"] = self._sample_angle
                    container_geometry["FrontThick"] = self._container_front_thickness
                    container_geometry["BackThick"] = self._container_back_thickness

                if self._shape == "Cylinder":
                    container_geometry["Shape"] = "HollowCylinder"
                    container_geometry["InnerRadius"] = self._sample_radius
                    container_geometry["OuterRadius"] = self._container_radius
                    container_geometry["Center"] = [0.0, 0.0, 0.0]

                if self._shape == "Annulus":
                    container_geometry["Shape"] = "HollowCylinderHolder"
                    container_geometry["InnerRadius"] = self._container_inner_radius
                    container_geometry["InnerOuterRadius"] = self._sample_inner_radius
                    container_geometry["OuterInnerRadius"] = self._sample_outer_radius
                    container_geometry["OuterRadius"] = self._container_outer_radius
                    container_geometry["Center"] = [0.0, 0.0, 0.0]

                container_material = self._set_material_dict("container")

        if sample_geometry or sample_material or container_geometry or container_material:
            set_sample_alg = self.createChildAlgorithm("SetSample", enableLogging=False)
            set_sample_alg.setProperty("InputWorkspace", ws)
            set_sample_alg.setProperty("Geometry", sample_geometry)
            set_sample_alg.setProperty("Material", sample_material)
            set_sample_alg.setProperty("ContainerGeometry", container_geometry)
            set_sample_alg.setProperty("ContainerMaterial", container_material)
            set_sample_alg.execute()

    def _set_material_dict(self, name):
        def get_attribute(attr_name):
            return getattr(self, "_" + name + "_" + attr_name)

        material_dict = dict()
        if get_attribute("chemical_formula"):
            material_dict["ChemicalFormula"] = get_attribute("chemical_formula")
        elif (
            get_attribute("attenuation_cross_section") != 0
            or get_attribute("coherent_cross_section") != 0
            or get_attribute("incoherent_cross_section") != 0
        ):
            material_dict["CoherentXSection"] = get_attribute("coherent_cross_section")
            material_dict["IncoherentXSection"] = get_attribute("incoherent_cross_section")
            material_dict["AttenuationXSection"] = get_attribute("attenuation_cross_section")
            material_dict["ScatteringXSection"] = get_attribute("coherent_cross_section") + get_attribute("incoherent_cross_section")
        else:
            return material_dict

        if get_attribute("density_type") == "Mass Density" and get_attribute("density") != 0:
            material_dict["SampleMassDensity"] = get_attribute("density")
        elif get_attribute("density_type") == "Number Density":
            if get_attribute("density") != 0:
                material_dict["SampleNumberDensity"] = get_attribute("density")
            material_dict["NumberDensityUnit"] = get_attribute("number_density_unit")

        return material_dict

    def _setup(self):
        self._input_ws = self.getProperty("InputWorkspace").value
        sample_is_group = isinstance(self._input_ws, WorkspaceGroup)

        # We cannot support WorkspaceGroups as inputs, since the output of the algorithm itself is a group
        # and it is currently not possible to override processGroups in python
        if sample_is_group:
            raise RuntimeError("WorkspaceGroup inputs are currently not supported. " "Please select the workspace items themselves.")

        self._beam_height = self.getProperty("BeamHeight").value
        self._beam_width = self.getProperty("BeamWidth").value

        self._monte_carlo_kwargs = {
            "EventsPerPoint": self.getProperty("EventsPerPoint").value,
            "Interpolation": self.getProperty("Interpolation").value,
            "MaxScatterPtAttempts": self.getProperty("MaxScatterPtAttempts").value,
            "SparseInstrument": self.getProperty("SparseInstrument").value,
            "NumberOfDetectorRows": self.getProperty("NumberOfDetectorRows").value,
            "NumberOfDetectorColumns": self.getProperty("NumberOfDetectorColumns").value,
        }

        self._sample_unit = self._input_ws.getAxis(0).getUnit().unitID()
        if self._sample_unit == "dSpacing":
            self._emode = "Elastic"
        else:
            self._emode = str(self._input_ws.getEMode())
        if self._emode == "Indirect" or self._emode == "Direct":
            self._efixed = self._get_efixed()

        self._sample_chemical_formula = self.getPropertyValue("SampleChemicalFormula")
        self._sample_coherent_cross_section = self.getProperty("SampleCoherentXSection").value
        self._sample_incoherent_cross_section = self.getProperty("SampleIncoherentXSection").value
        self._sample_attenuation_cross_section = self.getProperty("SampleAttenuationXSection").value
        self._sample_density_type = self.getPropertyValue("SampleDensityType")
        self._sample_number_density_unit = self.getPropertyValue("SampleNumberDensityUnit")
        self._sample_density = self.getProperty("SampleDensity").value

        self._container_chemical_formula = self.getPropertyValue("ContainerChemicalFormula")
        self._container_coherent_cross_section = self.getProperty("ContainerCoherentXSection").value
        self._container_incoherent_cross_section = self.getProperty("ContainerIncoherentXSection").value
        self._container_attenuation_cross_section = self.getProperty("ContainerAttenuationXSection").value
        self._container_density_type = self.getPropertyValue("ContainerDensityType")
        self._container_number_density_unit = self.getPropertyValue("ContainerNumberDensityUnit")
        self._container_density = self.getProperty("ContainerDensity").value

        self._shape = self.getProperty("Shape").value

        if self._shape != "Preset":
            self._height = self.getProperty("Height").value
            if self._shape == "FlatPlate":
                self._sample_width = self.getProperty("SampleWidth").value
                self._sample_thickness = self.getProperty("SampleThickness").value
                self._sample_angle = self.getProperty("SampleAngle").value
                self._sample_center = self.getProperty("SampleCenter").value
                self._container_front_thickness = self.getProperty("ContainerFrontThickness").value
                self._container_back_thickness = self.getProperty("ContainerBackThickness").value

            if self._shape == "Cylinder":
                self._sample_radius = self.getProperty("SampleRadius").value
                self._container_radius = self.getProperty("ContainerRadius").value

            if self._shape == "Annulus":
                self._sample_inner_radius = self.getProperty("SampleInnerRadius").value
                self._sample_outer_radius = self.getProperty("SampleOuterRadius").value
                self._container_inner_radius = self.getProperty("ContainerInnerRadius").value
                self._container_outer_radius = self.getProperty("ContainerOuterRadius").value

            self._set_sample_method = "Chemical Formula" if self._sample_chemical_formula != "" else "Cross Sections"
            self._set_can_method = "Chemical Formula" if self._container_chemical_formula != "" else "Cross Sections"

            self._has_can = self._input_has_container()
        else:
            self._has_can = self._ws_has_container()

        self._output_ws = self.getProperty("CorrectionsWorkspace").value
        output_ws_name = self.getPropertyValue("CorrectionsWorkspace")
        self._ass_ws_name = output_ws_name + "_ass"
        self._acc_ws_name = output_ws_name + "_acc"
        self._assc_ws_name = output_ws_name + "_assc"
        self._acsc_ws_name = output_ws_name + "_acsc"
        self._transposed = False
        self._indirect_elastic = False

    def validateInputs(self):
        issues = dict()
        try:
            self._setup()
        except Exception as err:
            issues["InputWorkspace"] = str(err)
            return issues

        if self._shape == "Annulus":
            if self._sample_inner_radius >= self._sample_outer_radius:
                issues["SampleOuterRadius"] = (
                    "Must be greater than SampleInnerRadius ("
                    + str(self._sample_inner_radius)
                    + "). Current value "
                    + str(self._sample_outer_radius)
                )
        if self._input_has_container():
            if self._shape == "Cylinder":
                if self._container_radius <= self._sample_radius:
                    issues["ContainerRadius"] = "Must be greater than SampleRadius"

            if self._shape == "Annulus":
                if self._container_inner_radius >= self._sample_inner_radius:
                    issues["ContainerInnerRadius"] = "Must be less than SampleInnerRadius"
                if self._container_outer_radius <= self._sample_outer_radius:
                    issues["ContainerOuterRadius"] = "Must be greater than SampleOuterRadius"

        return issues

    def _input_has_container(self):
        if self._shape == "FlatPlate":
            return self._container_front_thickness and self._container_back_thickness
        elif self._shape == "Cylinder":
            return bool(self._container_radius)
        elif self._shape == "Annulus":
            return self._container_inner_radius and self._container_outer_radius

    def _ws_has_container(self):
        ws = self._input_ws
        return ws.sample().hasEnvironment()

    def _get_efixed(self):
        """
        Returns the efixed value relating to the specified workspace
        """
        inst = self._input_ws.getInstrument()

        if inst.hasParameter("Efixed"):
            return inst.getNumberParameter("Efixed")[0]

        if inst.hasParameter("analyser"):
            analyser_comp = inst.getComponentByName(inst.getStringParameter("analyser")[0])

            if analyser_comp is not None and analyser_comp.hasParameter("Efixed"):
                return analyser_comp.getNumberParameter("EFixed")[0]

        # Direct instruments don't use the Efixed instrument parameter
        # The GetEi algorithm calculates and saves the Ei value to this sample log
        if self._input_ws.run().hasProperty("Ei"):
            return self._input_ws.getRun().getProperty("Ei").value

        raise ValueError("No Efixed parameter found")

    # ------------------------------- Converting to/from wavelength -------------------------------

    def _convert_to_wavelength(self, workspace):
        """
        Converts the specified workspace to units of wavelength.

        :param workspace:   The workspace to convert.
        :return:
        """

        x_unit = workspace.getAxis(0).getUnit().unitID()
        y_unit = workspace.getAxis(1).getUnit().unitID()

        # ----- Quick Conversions (Wavelength and DeltaE) -----

        if x_unit == "Wavelength":
            return self._clone_ws(workspace)
        elif y_unit == "Wavelength":
            self._transposed = True
            return self._tranpose_ws(workspace)
        elif x_unit == "DeltaE":
            return self._convert_units(workspace, "Wavelength", self._emode, self._efixed)
        elif y_unit == "DeltaE":
            self._transposed = True
            workspace = self._tranpose_ws(workspace)
            return self._convert_units(workspace, "Wavelength", self._emode, self._efixed)

        # ----- Indirect Elastic Conversions -----

        if self._emode == "Indirect":
            if x_unit == "MomentumTransfer":
                self._transposed = True
                return self._create_waves_indirect_elastic(self._tranpose_ws(workspace))
            else:
                return self._create_waves_indirect_elastic(self._clone_ws(workspace))

        # ----- Direct Conversions -----

        return self._convert_units(workspace, "Wavelength", self._emode)

    def _convert_from_wavelength(self, workspace):
        """
        Converts the specified workspace into units of wavelength.

        :param workspace:   The workspace whose units to convert.
        :return:            A workspace with units of wavelength, created from
                            converting the specified workspace.
        """

        if self._transposed:
            workspace = self._tranpose_ws(workspace)

        if self._sample_unit == "Label" and not self._isis_instrument:
            # This happens for E/I Fixed Window Scans for IN16B at ILL
            # In this case we want to keep the correction workspace in wavelength and the vertical axis as in the input
            return workspace
        elif self._indirect_elastic:
            return self._convert_units(workspace, "MomentumTransfer", self._emode, self._efixed)
        elif self._emode == "Indirect":
            return self._convert_units(workspace, self._sample_unit, self._emode, self._efixed)
        elif self._sample_unit != "Wavelength":
            return self._convert_units(workspace, self._sample_unit, self._emode)
        else:
            return workspace

    # ------------------------------- Converting IndirectElastic to wavelength ------------------------------
    def _create_waves_indirect_elastic(self, workspace):
        """
        Creates a wavelength workspace, from the workspace with the specified input workspace
        name, using an Elastic instrument definition file. E-Mode must be Indirect and the y-axis
        of the input workspace must be in units of Q.

        :param workspace:   The input workspace.
        :return:            The output wavelength workspace.
        """
        self._indirect_elastic = True
        self._q_values = workspace.getAxis(1).extractValues()
        instrument_name = workspace.getInstrument().getName()
        self._isis_instrument = instrument_name == "IRIS" or instrument_name == "OSIRIS"

        # ---------- Load Elastic Instrument Definition File ----------

        if self._isis_instrument:
            idf_name = instrument_name + "_elastic_Definition.xml"

            idf_path = os.path.join(config.getInstrumentDirectory(), idf_name)
            logger.information("IDF = %s" % idf_path)

            load_alg = self.createChildAlgorithm("LoadInstrument", enableLogging=True)
            load_alg.setProperty("Workspace", workspace)
            load_alg.setProperty("Filename", idf_path)
            load_alg.setProperty("RewriteSpectraMap", True)
            load_alg.execute()

        e_fixed = float(self._efixed)
        logger.information("Efixed = %f" % e_fixed)

        # ---------- Set Instrument Parameters ----------

        sip_alg = self.createChildAlgorithm("SetInstrumentParameter", enableLogging=False)
        sip_alg.setProperty("Workspace", workspace)
        sip_alg.setProperty("ParameterName", "EFixed")
        sip_alg.setProperty("ParameterType", "Number")
        sip_alg.setProperty("Value", str(e_fixed))
        sip_alg.execute()

        # ---------- Calculate Wavelength ----------

        wave = math.sqrt(81.787 / e_fixed)
        logger.information("Wavelength = %f" % wave)
        workspace.getAxis(0).setUnit("Wavelength")

        # ---------- Format Input Workspace ---------

        convert_alg = self.createChildAlgorithm("ConvertToHistogram", enableLogging=False)
        convert_alg.setProperty("InputWorkspace", workspace)
        convert_alg.execute()

        workspace = self._crop_ws(convert_alg.getProperty("OutputWorkspace").value)

        # --------- Set wavelengths as X-values in Output Workspace ----------

        waves = (0.01 * np.arange(-1, workspace.blocksize())) + wave
        logger.information("Waves for the dummy workspace: " + str(waves))
        nhist = workspace.getNumberHistograms()
        for idx in range(nhist):
            workspace.setX(idx, waves)

        if self._isis_instrument:
            workspace.replaceAxis(1, SpectraAxis.create(workspace))
            self._update_instrument_angles(workspace, self._q_values, wave)

        return workspace

    def _update_instrument_angles(self, workspace, q_values, wave):
        """
        Updates the instrument angles in the specified workspace, using the specified wavelength
        and the specified Q-Values. This is required when calculating absorption corrections for
        indirect elastic. This is used only for ISIS instruments.

        :param workspace:   The workspace whose instrument angles to update.
        :param q_values:    The extracted Q-Values (MomentumTransfer)
        :param wave:        The wavelength
        """
        work_dir = config["defaultsave.directory"]
        k0 = 4.0 * math.pi / wave
        theta = 2.0 * np.degrees(np.arcsin(q_values / k0))  # convert to angle

        filename = "Elastic_angles.txt"
        path = os.path.join(work_dir, filename)
        logger.information("Creating angles file : " + path)
        handle = open(path, "w")
        head = "spectrum,theta"
        handle.write(head + " \n")
        for n in range(0, len(theta)):
            handle.write(str(n + 1) + "   " + str(theta[n]) + "\n")
        handle.close()

        update_alg = self.createChildAlgorithm("UpdateInstrumentFromFile", enableLogging=False)
        update_alg.setProperty("Workspace", workspace)
        update_alg.setProperty("Filename", path)
        update_alg.setProperty("MoveMonitors", False)
        update_alg.setProperty("IgnorePhi", True)
        update_alg.setProperty("AsciiHeader", head)
        update_alg.setProperty("SkipFirstNLines", 1)

    def _crop_ws(self, workspace):
        """
        Crops the specified workspace to the XMin and XMax values specified in
        it's first and last X-Values.

        :param workspace:   The workspace to crop.
        :return:            The cropped workspace.
        """
        x = workspace.dataX(0)
        xmin = x[0]
        xmax = x[1]
        crop_alg = self.createChildAlgorithm("CropWorkspace", enableLogging=False)
        crop_alg.setProperty("InputWorkspace", workspace)
        crop_alg.setProperty("XMin", xmin)
        crop_alg.setProperty("XMax", xmax)
        crop_alg.execute()
        return crop_alg.getProperty("OutputWorkspace").value

    def _clone_ws(self, input_ws):
        """
        Clones the specified input workspace.

        :param input_ws:    The workspace to clone.
        :return:            A clone of the specified workspace.
        """
        clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
        clone_alg.setProperty("InputWorkspace", input_ws)
        clone_alg.execute()
        return clone_alg.getProperty("OutputWorkspace").value

    def _group_ws(self, workspaces):
        """
        Groups the specified input workspaces.

        :param input_ws:    A list of the workspaces to group together.
        :return:            A WorkspaceGroup containing the specified workspaces.
        """
        group_alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        group_alg.setProperty("InputWorkspaces", workspaces)
        group_alg.execute()
        return group_alg.getProperty("OutputWorkspace").value

    def _convert_units(self, workspace, target_unit, emode, efixed=None):
        """
        Converts the units of the specified workspace to the target unit, given
        the specified EMode, and potentially EFixed value.

        :param workspace:   The workspace whose units to convert.
        :param target_unit: The unit to convert to.
        :param emode:       The EMode to use in conversion.
        :param efixed:      The EFixed value to use in conversion.
        :return:            A workspace created from converting the units of the
                            specified workspace to the specified target unit.
        """
        convert_units_alg = self.createChildAlgorithm("ConvertUnits", enableLogging=False)
        convert_units_alg.setProperty("InputWorkspace", workspace)
        convert_units_alg.setProperty("Target", target_unit)
        convert_units_alg.setProperty("EMode", emode)
        # Check if EFixed was defined
        if efixed is not None:
            convert_units_alg.setProperty("EFixed", efixed)
        convert_units_alg.execute()
        return convert_units_alg.getProperty("OutputWorkspace").value

    def _convert_spectra_axis(self, workspace, target, emode, efixed):
        """
        Convert the units of the spectra axis of the specified workspace to the
        specified target unit, given the specified EMode and EFixed value.

        :param workspace:   The workspace whose spectra axis to convert.
        :param target:      The target unit to convert to.
        :param emode:       The EMode to use in conversion.
        :param efixed:      The EFixed value to use in conversion.
        :return:            A workspace created from converting the units of the
                            spectra axis of the specified workspace to the specified
                            target unit.
        """
        convert_axis_alg = self.createChildAlgorithm("ConvertSpectraAxis", enableLogging=False)
        convert_axis_alg.setProperty("InputWorkspace", workspace)
        convert_axis_alg.setProperty("EMode", emode)
        convert_axis_alg.setProperty("Target", target)
        convert_axis_alg.setProperty("EFixed", efixed)
        convert_axis_alg.execute()
        return convert_axis_alg.getProperty("OutputWorkspace").value

    def _tranpose_ws(self, workspace):
        """
        Tranposes the specified workspace.

        :param workspace:   The workspace to transpose.
        :return:            The transpose of the specified workspace.
        """
        transpose_alg = self.createChildAlgorithm("Transpose", enableLogging=False)
        transpose_alg.setProperty("InputWorkspace", workspace)
        transpose_alg.execute()
        return transpose_alg.getProperty("OutputWorkspace").value

    # ------------------------------- Utility algorithms -------------------------------
    @staticmethod
    def _set_algorithm_properties(algorithm, properties):
        """
        Sets the specified algorithm's properties using the given properties.

        :param algorithm:   The algorithm whose properties to set.
        :param properties:  The dictionary of properties to set.
        """
        for key, value in properties.items():
            algorithm.setProperty(key, value)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(PaalmanPingsMonteCarloAbsorption)
