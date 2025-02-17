# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, Progress
from mantid.kernel import (
    VisibleWhenProperty,
    EnabledWhenProperty,
    Property,
    PropertyCriterion,
    StringListValidator,
    IntBoundedValidator,
    FloatBoundedValidator,
    Direction,
)


class SimpleShapeMonteCarloAbsorption(DataProcessorAlgorithm):
    # basic sample variables
    _input_ws_name = None
    _chemical_formula = None
    _density_type = None
    _density = None
    _shape = None
    _height = None

    # general variables
    _events = None
    _interpolation = None
    _output_ws = None

    # beam variables
    _beam_height = None
    _beam_width = None

    # flat plate variables
    _width = None
    _thickness = None
    _center = None
    _angle = None

    # cylinder variables
    _radius = None

    # annulus variables
    _inner_radius = None
    _outer_radius = None

    def category(self):
        return "Workflow\\Inelastic;CorrectionFunctions\\AbsorptionCorrections;Workflow\\MIDAS"

    def seeAlso(self):
        return ["CalculateMonteCarloAbsorption", "MonteCarloAbsorption"]

    def summary(self):
        return "Calculates absorption corrections for a given sample shape."

    def PyInit(self):
        # basic sample options

        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input), doc="Input workspace")

        self.declareProperty(
            name="MaterialAlreadyDefined", defaultValue=False, doc="Select this option if the material has already been defined"
        )

        material_defined_prop = EnabledWhenProperty("MaterialAlreadyDefined", PropertyCriterion.IsDefault)

        self.declareProperty(name="ChemicalFormula", defaultValue="", doc="Chemical formula of sample")
        self.setPropertySettings("ChemicalFormula", material_defined_prop)

        self.declareProperty(
            name="CoherentXSection",
            defaultValue=Property.EMPTY_DBL,
            doc="The coherent cross section of the sample in barns. It can be used instead of the Chemical Formula.",
        )
        self.setPropertySettings("CoherentXSection", material_defined_prop)

        self.declareProperty(
            name="IncoherentXSection",
            defaultValue=Property.EMPTY_DBL,
            doc="The incoherent cross section of the sample in barns. It can be used instead of the Chemical Formula.",
        )
        self.setPropertySettings("IncoherentXSection", material_defined_prop)

        self.declareProperty(
            name="AttenuationXSection",
            defaultValue=Property.EMPTY_DBL,
            doc="The absorption cross section of the sample in barns. It can be used instead of the Chemical Formula.",
        )
        self.setPropertySettings("AttenuationXSection", material_defined_prop)

        self.declareProperty(
            name="DensityType",
            defaultValue="Mass Density",
            validator=StringListValidator(["Mass Density", "Number Density"]),
            doc="Use of Mass density or Number density.",
        )
        self.setPropertySettings("DensityType", material_defined_prop)

        self.declareProperty(
            name="Density", defaultValue=0.1, doc="The value for the Mass density (g/cm^3) or Number density (1/Angstrom^3)."
        )
        self.setPropertySettings("Density", material_defined_prop)

        self.declareProperty(
            name="NumberDensityUnit",
            defaultValue="Atoms",
            validator=StringListValidator(["Atoms", "Formula Units"]),
            doc="Choose which units Density refers to. Allowed values: [Atoms, Formula Units]",
        )
        self.setPropertySettings("NumberDensityUnit", material_defined_prop)

        # -------------------------------------------------------------------------------------------

        # Monte Carlo options
        self.declareProperty(
            name="NumberOfWavelengthPoints", defaultValue=10, validator=IntBoundedValidator(1), doc="Number of wavelengths for calculation"
        )

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

        # -------------------------------------------------------------------------------------------

        # Beam size
        self.declareProperty(name="BeamHeight", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Height of the beam (cm)")

        self.declareProperty(name="BeamWidth", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Width of the beam (cm)")

        # -------------------------------------------------------------------------------------------

        # set up shape options

        self.declareProperty(
            name="Shape",
            defaultValue="FlatPlate",
            validator=StringListValidator(["FlatPlate", "Cylinder", "Annulus"]),
            doc="Geometry of sample environment. Options are: FlatPlate, Cylinder, Annulus",
        )

        flat_plate_condition = VisibleWhenProperty("Shape", PropertyCriterion.IsEqualTo, "FlatPlate")
        cylinder_condition = VisibleWhenProperty("Shape", PropertyCriterion.IsEqualTo, "Cylinder")
        annulus_condition = VisibleWhenProperty("Shape", PropertyCriterion.IsEqualTo, "Annulus")

        # height is common to all options

        self.declareProperty(
            name="Height", defaultValue=0.0, validator=FloatBoundedValidator(0.0), doc="Height of the sample environment (cm)"
        )

        # flat plate options

        self.declareProperty(
            name="Width", defaultValue=0.0, validator=FloatBoundedValidator(0.0), doc="Width of the FlatPlate sample environment (cm)"
        )
        self.setPropertySettings("Width", flat_plate_condition)

        self.declareProperty(
            name="Thickness", defaultValue=0.0, validator=FloatBoundedValidator(), doc="Thickness of the FlatPlate sample environment (cm)"
        )
        self.setPropertySettings("Thickness", flat_plate_condition)

        self.declareProperty(name="Center", defaultValue=0.0, doc="Center of the FlatPlate sample environment")
        self.setPropertySettings("Center", flat_plate_condition)

        self.declareProperty(
            name="Angle",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="Angle of the FlatPlate sample environment with respect to the beam (degrees)",
        )
        self.setPropertySettings("Angle", flat_plate_condition)

        # cylinder options

        self.declareProperty(
            name="Radius", defaultValue=0.0, validator=FloatBoundedValidator(0.0), doc="Radius of the Cylinder sample environment (cm)"
        )
        self.setPropertySettings("Radius", cylinder_condition)

        # annulus options

        self.declareProperty(
            name="OuterRadius",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="Outer radius of the Annulus sample environment (cm)",
        )
        self.setPropertySettings("OuterRadius", annulus_condition)

        self.declareProperty(
            name="InnerRadius",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="Inner radius of the Annulus sample environment (cm)",
        )
        self.setPropertySettings("InnerRadius", annulus_condition)

        # -------------------------------------------------------------------------------------------

        # Output options
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="The output corrected workspace."
        )

    def PyExec(self):
        self.log().warning("SimpleShapeMonteCarloAbsorption is deprecated, please use PaalmanPingsMonteCarloAbsorption instead.")

        # setup progress reporting
        prog = Progress(self, 0.0, 1.0, 3)

        prog.report("Setting up sample environment")

        # set the beam shape
        set_beam_alg = self.createChildAlgorithm("SetBeam", enableLogging=False)
        set_beam_alg.setProperty("InputWorkspace", self._input_ws)
        set_beam_alg.setProperty("Geometry", {"Shape": "Slit", "Width": self._beam_width, "Height": self._beam_height})
        set_beam_alg.execute()

        # set the sample geometry
        sample_geometry = dict()
        sample_geometry["Height"] = self._height

        if self._shape == "FlatPlate":
            sample_geometry["Shape"] = "FlatPlate"
            sample_geometry["Width"] = self._width
            sample_geometry["Thick"] = self._thickness
            sample_geometry["Center"] = [0.0, 0.0, self._center]
            sample_geometry["Angle"] = self._angle

        if self._shape == "Cylinder":
            sample_geometry["Shape"] = "Cylinder"
            sample_geometry["Radius"] = self._radius
            sample_geometry["Center"] = [0.0, 0.0, 0.0]

        if self._shape == "Annulus":
            sample_geometry["Shape"] = "HollowCylinder"
            sample_geometry["InnerRadius"] = self._inner_radius
            sample_geometry["OuterRadius"] = self._outer_radius
            sample_geometry["Center"] = [0.0, 0.0, 0.0]
            sample_geometry["Axis"] = 1

        set_sample_alg = self.createChildAlgorithm("SetSample", enableLogging=False)
        set_sample_alg.setProperty("InputWorkspace", self._input_ws)
        set_sample_alg.setProperty("Geometry", sample_geometry)

        # set sample
        if self._material_defined:
            # set sample without sample material
            set_sample_alg.execute()

        else:
            # set the sample material
            sample_material = dict()
            if self._set_sample_method == "Chemical Formula":
                sample_material["ChemicalFormula"] = self._chemical_formula
            else:
                sample_material["CoherentXSection"] = float(self._coherent_cross_section)
                sample_material["IncoherentXSection"] = float(self._incoherent_cross_section)
                sample_material["AttenuationXSection"] = float(self._attenuation_cross_section)
                sample_material["ScatteringXSection"] = float(self._coherent_cross_section) + float(self._incoherent_cross_section)

            if self._density_type == "Mass Density":
                sample_material["SampleMassDensity"] = self._density
            if self._density_type == "Number Density":
                sample_material["SampleNumberDensity"] = self._density
                sample_material["NumberDensityUnit"] = self._number_density_unit

            set_sample_alg.setProperty("Material", sample_material)

            set_sample_alg.execute()

        prog.report("Calculating sample corrections")

        monte_carlo_alg = self.createChildAlgorithm("MonteCarloAbsorption", enableLogging=True)
        monte_carlo_alg.setProperty("InputWorkspace", self._input_ws)
        monte_carlo_alg.setProperty("OutputWorkspace", self._output_ws)
        monte_carlo_alg.setProperty("EventsPerPoint", self._events)
        monte_carlo_alg.setProperty("NumberOfWavelengthPoints", self._number_wavelengths)
        monte_carlo_alg.setProperty("Interpolation", self._interpolation)
        monte_carlo_alg.setProperty("MaxScatterPtAttempts", self._max_scatter_attempts)
        monte_carlo_alg.execute()

        output_ws = monte_carlo_alg.getProperty("OutputWorkspace").value

        prog.report("Recording Sample Logs")

        log_names = ["beam_height", "beam_width"]
        log_values = [self._beam_height, self._beam_width]

        # add sample geometry to sample logs
        for key, value in sample_geometry.items():
            log_names.append("sample_" + key.lower())
            log_values.append(value)

        add_sample_log_alg = self.createChildAlgorithm("AddSampleLogMultiple", enableLogging=False)
        add_sample_log_alg.setProperty("Workspace", output_ws)
        add_sample_log_alg.setProperty("LogNames", log_names)
        add_sample_log_alg.setProperty("LogValues", log_values)
        add_sample_log_alg.execute()

        self.setProperty("OutputWorkspace", output_ws)

    def _setup(self):
        # basic options
        self._input_ws = self.getProperty("InputWorkspace").value
        self._material_defined = self.getProperty("MaterialAlreadyDefined").value
        self._chemical_formula = self.getPropertyValue("ChemicalFormula")
        self._coherent_cross_section = self.getPropertyValue("CoherentXSection")
        self._incoherent_cross_section = self.getPropertyValue("IncoherentXSection")
        self._attenuation_cross_section = self.getPropertyValue("AttenuationXSection")
        self._density_type = self.getPropertyValue("DensityType")
        self._density = self.getProperty("Density").value
        self._number_density_unit = self.getPropertyValue("NumberDensityUnit")
        self._shape = self.getPropertyValue("Shape")

        self._number_wavelengths = self.getProperty("NumberOfWavelengthPoints").value
        self._events = self.getProperty("EventsPerPoint").value
        self._interpolation = self.getProperty("Interpolation").value
        self._max_scatter_attempts = self.getProperty("MaxScatterPtAttempts").value

        self._set_sample_method = "Chemical Formula" if self._chemical_formula != "" and not self._material_defined else "Cross Sections"

        # beam options
        self._beam_height = self.getProperty("BeamHeight").value
        self._beam_width = self.getProperty("BeamWidth").value

        # shape options
        self._height = self.getProperty("Height").value

        # flat plate
        self._width = self.getProperty("Width").value
        self._thickness = self.getProperty("Thickness").value
        self._center = self.getProperty("Center").value
        self._angle = self.getProperty("Angle").value

        # cylinder
        self._radius = self.getProperty("Radius").value

        # annulus
        self._inner_radius = self.getProperty("InnerRadius").value
        self._outer_radius = self.getProperty("OuterRadius").value

        # output
        self._output_ws = self.getPropertyValue("OutputWorkspace")

    def validateInputs(self):
        self._setup()
        issues = dict()

        if (
            (not self._material_defined)
            and (not self._chemical_formula)
            and (
                self._coherent_cross_section == Property.EMPTY_DBL
                or self._incoherent_cross_section == Property.EMPTY_DBL
                or self._attenuation_cross_section == Property.EMPTY_DBL
            )
        ):
            issues["ChemicalFormula"] = "Please enter a chemical formula or cross sections."

        if not self._height:
            issues["Height"] = "Please enter a non-zero number for height"

        if self._shape == "FlatPlate":
            if not self._width:
                issues["Width"] = "Please enter a non-zero number for width"
            if not self._thickness:
                issues["Thickness"] = "Please enter a non-zero number for thickness"

        if self._shape == "Cylinder":
            if not self._radius:
                issues["Radius"] = "Please enter a non-zero number for radius"

        if self._shape == "Annulus":
            if not self._inner_radius:
                issues["InnerRadius"] = "Please enter a non-zero number for inner radius"
            if not self._outer_radius:
                issues["OuterRadius"] = "Please enter a non-zero number for outer radius"

            # Geometry validation: outer radius > inner radius
            if not self._outer_radius > self._inner_radius:
                issues["OuterRadius"] = "Must be greater than InnerRadius"

        return issues


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SimpleShapeMonteCarloAbsorption)
