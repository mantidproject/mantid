# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-many-locals,too-many-instance-attributes,too-many-arguments,invalid-name
import math
import numpy as np
from mantid.simpleapi import (
    AddSampleLogMultiple,
    CreateWorkspace,
    DeleteWorkspace,
    ExtractSingleSpectrum,
    GroupWorkspaces,
    SplineInterpolation,
)
from mantid.api import (
    mtd,
    PythonAlgorithm,
    AlgorithmFactory,
    PropertyMode,
    MatrixWorkspace,
    MatrixWorkspaceProperty,
    WorkspaceGroupProperty,
    InstrumentValidator,
    Progress,
)
from mantid.kernel import StringListValidator, IntBoundedValidator, FloatBoundedValidator, Direction, logger


def set_material_density(set_material_alg, density_type, density, number_density_unit):
    if density_type == "Mass Density":
        set_material_alg.setProperty("SampleMassDensity", density)
    else:
        set_material_alg.setProperty("SampleNumberDensity", density)
        set_material_alg.setProperty("NumberDensityUnit", number_density_unit)
    return set_material_alg


class CylinderPaalmanPingsCorrection(PythonAlgorithm):
    # Sample variables
    _sample_ws_name = None
    _use_sample_mass_density = None
    _sample_inner_radius = None
    _sample_outer_radius = None
    _sample_density_type = None
    _sample_density = None

    # Container variables
    _use_can = False
    _can_ws_name = None
    _can_density_type = None
    _can_density = None
    _can_outer_radius = None

    _number_can = 1
    _ms = 1
    _number_wavelengths = 10
    _emode = None
    _efixed = 0.0
    _step_size = None
    _output_ws_name = None
    _beam = list()
    _angles = list()
    _waves = list()
    _elastic = 0.0
    _fixed = 0.0
    _sig_s = None
    _sig_a = None
    _density = None
    _radii = None
    _interpolate = False

    # ------------------------------------------------------------------------------

    def version(self):
        return 2

    def category(self):
        return "Workflow\\MIDAS;CorrectionFunctions\\AbsorptionCorrections"

    def summary(self):
        return "Calculates absorption corrections for a cylindrical or annular sample using Paalman & Pings format."

    # ------------------------------------------------------------------------------

    def PyInit(self):
        ws_validator = InstrumentValidator()

        self.declareProperty(
            MatrixWorkspaceProperty("SampleWorkspace", "", validator=ws_validator, direction=Direction.Input),
            doc="Name for the input Sample workspace.",
        )

        self.declareProperty(name="SampleChemicalFormula", defaultValue="", doc="Sample chemical formula")

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
            name="SampleDensity", defaultValue=0.1, doc="The value for the sample Mass density (g/cm^3) or Number density (1/Angstrom^3)."
        )

        self.declareProperty(name="SampleInnerRadius", defaultValue=0.05, validator=FloatBoundedValidator(0.0), doc="Sample inner radius")

        self.declareProperty(name="SampleOuterRadius", defaultValue=0.1, validator=FloatBoundedValidator(0.0), doc="Sample outer radius")

        self.declareProperty(
            MatrixWorkspaceProperty("CanWorkspace", "", optional=PropertyMode.Optional, validator=ws_validator, direction=Direction.Input),
            doc="Name for the input Can workspace.",
        )

        self.declareProperty(name="CanChemicalFormula", defaultValue="", doc="Can chemical formula")

        self.declareProperty(
            name="CanCoherentXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The coherent cross-section for the can material in barns. To be used instead of " "Chemical Formula.",
        )

        self.declareProperty(
            name="CanIncoherentXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The incoherent cross-section for the can material in barns. To be used instead of " "Chemical Formula.",
        )

        self.declareProperty(
            name="CanAttenuationXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The absorption cross-section for the can material in barns. To be used instead of " "Chemical Formula.",
        )

        self.declareProperty(
            name="CanDensityType",
            defaultValue="Mass Density",
            validator=StringListValidator(["Mass Density", "Number Density"]),
            doc="Use of Mass density or Number density for the can.",
        )

        self.declareProperty(
            name="CanNumberDensityUnit",
            defaultValue="Atoms",
            validator=StringListValidator(["Atoms", "Formula Units"]),
            doc="Choose which units CanDensity refers to. Allowed values: [Atoms, Formula Units]",
        )

        self.declareProperty(
            name="CanDensity", defaultValue=0.1, doc="The value for the can Mass density (g/cm^3) or Number density (1/Angstrom^3)."
        )

        self.declareProperty(name="CanOuterRadius", defaultValue=0.15, validator=FloatBoundedValidator(0.0), doc="Can outer radius")

        self.declareProperty(name="BeamHeight", defaultValue=3.0, validator=FloatBoundedValidator(0.0), doc="Beam height")

        self.declareProperty(name="BeamWidth", defaultValue=2.0, validator=FloatBoundedValidator(0.0), doc="Beam width")

        self.declareProperty(name="StepSize", defaultValue=0.002, validator=FloatBoundedValidator(0.0), doc="Step size")

        self.declareProperty(
            name="Interpolate", defaultValue=True, doc="Interpolate the correction workspaces to match the sample workspace"
        )

        self.declareProperty(
            name="NumberWavelengths", defaultValue=10, validator=IntBoundedValidator(1), doc="Number of wavelengths for calculation"
        )

        self.declareProperty(
            name="Emode",
            defaultValue="Elastic",
            validator=StringListValidator(["Elastic", "Indirect", "Direct", "Efixed"]),
            doc="Energy transfer mode.",
        )

        self.declareProperty(
            name="Efixed",
            defaultValue=0.0,
            doc="Analyser energy (mev). By default will be read from the instrument parameters. "
            "Specify manually to override. This is used in energy transfer modes other than Elastic.",
        )

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output), doc="The output corrections workspace group"
        )

    # ------------------------------------------------------------------------------

    def validateInputs(self):
        self._setup()
        issues = dict()

        sample_ws_name = self.getPropertyValue("SampleWorkspace")
        can_ws_name = self.getPropertyValue("CanWorkspace")

        if (self._radii[1] - self._radii[0]) < 1e-4:
            issues["SampleOuterRadius"] = "Sample outer radius must be bigger than inner radius"

        logger.information("Sample : inner radius = %f ; outer radius = %f" % (self._radii[0], self._radii[1]))

        if self._use_can:
            self._radii[2] = self._can_outer_radius
            if (self._radii[2] - self._radii[1]) < 1e-4:
                issues["CanOuterRadius"] = "Can outer radius must be bigger than sample outer radius"
            else:
                logger.information("Can : inner radius = %f ; outer radius = %f" % (self._radii[1], self._radii[2]))

        # Ensure that a can chemical formula is given when using a can workspace
        if self._use_can:
            can_chemical_formula = self.getPropertyValue("CanChemicalFormula")
            can_coherent_cross_section = self.getPropertyValue("CanCoherentXSection")
            can_incoherent_cross_section = self.getPropertyValue("CanIncoherentXSection")
            can_attenuation_cross_section = self.getPropertyValue("CanAttenuationXSection")
            if can_chemical_formula == "" and (
                can_coherent_cross_section == 0.0 and can_incoherent_cross_section == 0.0 and can_attenuation_cross_section == 0.0
            ):
                issues["CanChemicalFormula"] = "Must provide a chemical formula or cross sections when providing a " "can workspace."

        # Ensure there are enough steps
        number_steps = int((self._sample_outer_radius - self._sample_inner_radius) / self._step_size)
        if number_steps < 20:
            issues["StepSize"] = "Number of steps (%d) should be >= 20" % number_steps
        logger.information("Sample : ms = %i " % self._ms)

        if self._emode != "Efixed":
            if not isinstance(mtd[sample_ws_name], MatrixWorkspace):
                issues["SampleWorkspace"] = "The SampleWorkspace must be a MatrixWorkspace."
            elif mtd[sample_ws_name].getAxis(0).getUnit().unitID() != "Wavelength":
                # require both sample and can ws have wavelength as x-axis
                issues["SampleWorkspace"] = "The SampleWorkspace must have units of wavelength."

            if self._use_can:
                if not isinstance(mtd[can_ws_name], MatrixWorkspace):
                    issues["CanWorkspace"] = "The CanWorkspace must be a MatrixWorkspace."
                elif mtd[can_ws_name].getAxis(0).getUnit().unitID() != "Wavelength":
                    issues["CanWorkspace"] = "The CanWorkspace must have units of wavelength."

        return issues

    # ------------------------------------------------------------------------------

    def PyExec(self):
        self._setup()
        self._sample()
        self._wave_range()
        self._get_angles()
        self._transmission()

        dataA1 = []
        dataA2 = []
        dataA3 = []
        dataA4 = []

        data_prog = Progress(self, start=0.1, end=0.85, nreports=len(self._angles))
        for angle in self._angles:
            (A1, A2, A3, A4) = self._cyl_abs(angle)
            logger.information("Angle : %f * successful" % angle)
            data_prog.report("Appending data for angle %f" % angle)
            dataA1 = np.append(dataA1, A1)
            dataA2 = np.append(dataA2, A2)
            dataA3 = np.append(dataA3, A3)
            dataA4 = np.append(dataA4, A4)

        dataX = self._waves * len(self._angles)

        wrk_reports = 5
        if self._use_can:
            wrk_reports = 8
        workflow_prog = Progress(self, start=0.85, end=1.0, nreports=wrk_reports)
        # Create the output workspaces
        ass_ws = self._output_ws_name + "_ass"
        workflow_prog.report("Creating Workspace")
        CreateWorkspace(
            OutputWorkspace=ass_ws,
            DataX=dataX,
            DataY=dataA1,
            NSpec=len(self._angles),
            UnitX="Wavelength",
            ParentWorkspace=self._sample_ws_name,
            EnableLogging=False,
        )
        workspaces = [ass_ws]

        if self._use_can:
            workflow_prog.report("Creating assc Workspace")
            assc_ws = self._output_ws_name + "_assc"
            workspaces.append(assc_ws)
            CreateWorkspace(
                OutputWorkspace=assc_ws,
                DataX=dataX,
                DataY=dataA2,
                NSpec=len(self._angles),
                UnitX="Wavelength",
                ParentWorkspace=self._sample_ws_name,
                EnableLogging=False,
            )

            workflow_prog.report("Creating acsc Workspace")
            acsc_ws = self._output_ws_name + "_acsc"
            workspaces.append(acsc_ws)
            CreateWorkspace(
                OutputWorkspace=acsc_ws,
                DataX=dataX,
                DataY=dataA3,
                NSpec=len(self._angles),
                UnitX="Wavelength",
                ParentWorkspace=self._sample_ws_name,
                EnableLogging=False,
            )

            workflow_prog.report("Creating acc Workspace")
            acc_ws = self._output_ws_name + "_acc"
            workspaces.append(acc_ws)
            CreateWorkspace(
                OutputWorkspace=acc_ws,
                DataX=dataX,
                DataY=dataA4,
                NSpec=len(self._angles),
                UnitX="Wavelength",
                ParentWorkspace=self._sample_ws_name,
                EnableLogging=False,
            )

        if self._interpolate:
            self._interpolate_corrections(workspaces)

        workflow_prog.report("Constructing Sample Logs")
        sample_log_workspaces = workspaces
        sample_logs = [
            ("sample_shape", "cylinder"),
            ("sample_filename", self._sample_ws_name),
            ("sample_inner", self._sample_inner_radius),
            ("sample_outer", self._sample_outer_radius),
            ("emode", self._emode),
            ("efixed", self._efixed),
        ]

        if self._use_can:
            sample_logs.append(("can_filename", self._can_ws_name))
            sample_logs.append(("can_outer", self._can_outer_radius))

        log_names = [item[0] for item in sample_logs]
        log_values = [item[1] for item in sample_logs]

        for ws_name in sample_log_workspaces:
            workflow_prog.report("Adding sample logs to %s" % ws_name)
            AddSampleLogMultiple(Workspace=ws_name, LogNames=log_names, LogValues=log_values, EnableLogging=False)

        workflow_prog.report("Create GroupWorkspace Output")
        GroupWorkspaces(InputWorkspaces=",".join(workspaces), OutputWorkspace=self._output_ws_name, EnableLogging=False)
        self.setPropertyValue("OutputWorkspace", self._output_ws_name)
        workflow_prog.report("Algorithm complete")

    # ------------------------------------------------------------------------------

    def _setup(self):
        setup_prog = Progress(self, start=0.00, end=0.01, nreports=2)
        setup_prog.report("Obtaining input properties")
        self._sample_ws_name = self.getPropertyValue("SampleWorkspace")
        self._sample_density_type = self.getPropertyValue("SampleDensityType")
        self._sample_number_density_unit = self.getPropertyValue("SampleNumberDensityUnit")
        self._sample_density = self.getProperty("SampleDensity").value
        self._sample_chemical_formula = self.getPropertyValue("SampleChemicalFormula")
        self._sample_coherent_cross_section = self.getPropertyValue("SampleCoherentXSection")
        self._sample_incoherent_cross_section = self.getPropertyValue("SampleIncoherentXSection")
        self._sample_attenuation_cross_section = self.getPropertyValue("SampleAttenuationXSection")
        self._sample_inner_radius = self.getProperty("SampleInnerRadius").value
        self._sample_outer_radius = self.getProperty("SampleOuterRadius").value
        self._number_can = 1

        self._can_ws_name = self.getPropertyValue("CanWorkspace")
        self._use_can = self._can_ws_name != ""
        self._can_density_type = self.getPropertyValue("CanDensityType")
        self._can_number_density_unit = self.getPropertyValue("CanNumberDensityUnit")
        self._can_density = self.getProperty("CanDensity").value
        self._can_chemical_formula = self.getPropertyValue("CanChemicalFormula")
        self._can_coherent_cross_section = self.getPropertyValue("CanCoherentXSection")
        self._can_incoherent_cross_section = self.getPropertyValue("CanIncoherentXSection")
        self._can_attenuation_cross_section = self.getPropertyValue("CanAttenuationXSection")
        self._can_outer_radius = self.getProperty("CanOuterRadius").value
        if self._use_can:
            self._number_can = 2

        self._step_size = self.getProperty("StepSize").value
        self._radii = np.zeros(self._number_can + 1)
        self._radii[0] = self._sample_inner_radius
        self._radii[1] = self._sample_outer_radius
        if self._use_can:
            self._radii[2] = self._can_outer_radius

        setup_prog.report("Obtaining beam values")
        beam_width = self.getProperty("BeamWidth").value
        beam_height = self.getProperty("BeamHeight").value
        self._beam = [
            beam_height,
            0.5 * beam_width,
            -0.5 * beam_width,
            (beam_width / 2),
            -(beam_width / 2),
            0.0,
            beam_height,
            0.0,
            beam_height,
        ]

        self._interpolate = self.getProperty("Interpolate").value
        self._number_wavelengths = self.getProperty("NumberWavelengths").value

        self._emode = self.getPropertyValue("Emode")
        self._efixed = self.getProperty("Efixed").value

        if self._emode == "Efixed":
            logger.information("No interpolation is possible in Efixed mode.")
            self._interpolate = False

        if self._efixed == 0.0 and self._emode != "Elastic":
            # In all the modes other than elastic, efixed is needed.
            # So try to get from instrument if the input is not set.
            try:
                self._efixed = self._getEfixed()
                logger.information("Found Efixed = {0}".format(self._efixed))
            except ValueError:
                raise RuntimeError("Could not find the Efixed parameter in the instrument. " "Please specify manually.")

        self._set_sample_method = "Chemical Formula" if self._sample_chemical_formula != "" else "Cross Sections"
        self._set_can_method = "Chemical Formula" if self._can_chemical_formula != "" else "Cross Sections"

        # purge the lists
        self._angles = list()
        self._waves = list()

        self._output_ws_name = self.getPropertyValue("OutputWorkspace")

    # ------------------------------------------------------------------------------

    def _sample(self):
        sample_prog = Progress(self, start=0.01, end=0.03, nreports=2)
        sample_prog.report("Setting Sample Material for Sample")

        sample_ws, self._sample_density = self._set_material(
            self._sample_ws_name,
            self._set_sample_method,
            self._sample_chemical_formula,
            self._sample_coherent_cross_section,
            self._sample_incoherent_cross_section,
            self._sample_attenuation_cross_section,
            self._sample_density_type,
            self._sample_density,
            self._sample_number_density_unit,
        )

        sample_material = sample_ws.sample().getMaterial()
        # total scattering x-section
        self._sig_s = np.zeros(self._number_can)
        self._sig_s[0] = sample_material.totalScatterXSection()
        # absorption x-section
        self._sig_a = np.zeros(self._number_can)
        self._sig_a[0] = sample_material.absorbXSection()
        # density
        self._density = np.zeros(self._number_can)
        self._density[0] = self._sample_density

        if self._use_can:
            sample_prog.report("Setting Sample Material for Container")

            can_ws, self._can_density = self._set_material(
                self._can_ws_name,
                self._set_can_method,
                self._can_chemical_formula,
                self._can_coherent_cross_section,
                self._can_incoherent_cross_section,
                self._can_attenuation_cross_section,
                self._can_density_type,
                self._can_density,
                self._can_number_density_unit,
            )

            can_material = can_ws.sample().getMaterial()
            self._sig_s[1] = can_material.totalScatterXSection()
            self._sig_a[1] = can_material.absorbXSection()
            self._density[1] = self._can_density

    def _set_material(
        self,
        ws_name,
        method,
        chemical_formula,
        coherent_x_section,
        incoherent_x_section,
        attenuation_x_section,
        density_type,
        density,
        number_density_unit,
    ):
        """
        Sets the sample material for a given workspace
        @param ws_name              :: name of the workspace to set sample material for
        @param method               :: the method used to set the sample material
        @param chemical_formula     :: Chemical formula of sample
        @param coherent_x_section   :: the coherent cross section
        @param incoherent_x_section :: the incoherent cross section
        @param attenuation_x_section:: the absorption cross section
        @param density_type         :: 'Mass Density' or 'Number Density'
        @param density              :: Density of sample
        @param number_density_unit  :: the unit to use ('Atoms' or 'Formula Units') if the density type is Number density
        @return pointer to the workspace with sample material set
                AND
                number density of the sample material
        """
        set_material_alg = self.createChildAlgorithm("SetSampleMaterial")
        set_material_alg.setProperty("InputWorkspace", ws_name)
        set_material_alg = set_material_density(set_material_alg, density_type, density, number_density_unit)

        if method == "Chemical Formula":
            set_material_alg.setProperty("ChemicalFormula", chemical_formula)
        else:
            set_material_alg.setProperty("CoherentXSection", coherent_x_section)
            set_material_alg.setProperty("IncoherentXSection", incoherent_x_section)
            set_material_alg.setProperty("AttenuationXSection", attenuation_x_section)
            set_material_alg.setProperty("ScatteringXSection", float(coherent_x_section) + float(incoherent_x_section))

        set_material_alg.execute()
        ws = set_material_alg.getProperty("InputWorkspace").value
        number_density = ws.sample().getMaterial().numberDensity
        return ws, number_density

    # ------------------------------------------------------------------------------

    def _get_angles(self):
        num_hist = mtd[self._sample_ws_name].getNumberHistograms()
        angle_prog = Progress(self, start=0.03, end=0.07, nreports=num_hist)
        source_pos = mtd[self._sample_ws_name].getInstrument().getSource().getPos()
        sample_pos = mtd[self._sample_ws_name].getInstrument().getSample().getPos()
        beam_pos = sample_pos - source_pos
        self._angles = list()
        for index in range(0, num_hist):
            angle_prog.report("Obtaining data for detector angle %i" % index)
            detector = mtd[self._sample_ws_name].getDetector(index)
            two_theta = detector.getTwoTheta(sample_pos, beam_pos) * 180.0 / math.pi
            self._angles.append(two_theta)
        logger.information("Detector angles : %i from %f to %f " % (len(self._angles), self._angles[0], self._angles[-1]))

    # ------------------------------------------------------------------------------

    def _wave_range(self):
        if self._emode != "Elastic":
            self._fixed = math.sqrt(81.787 / self._efixed)

        if self._emode == "Efixed":
            self._waves.append(self._fixed)
            logger.information("Efixed mode, setting lambda_fixed to {0}".format(self._fixed))
        else:
            wave_range = "__wave_range"
            ExtractSingleSpectrum(InputWorkspace=self._sample_ws_name, OutputWorkspace=wave_range, WorkspaceIndex=0)

            Xin = mtd[wave_range].readX(0)
            wave_min = mtd[wave_range].readX(0)[0]
            wave_max = mtd[wave_range].readX(0)[len(Xin) - 1]
            number_waves = self._number_wavelengths
            wave_bin = (wave_max - wave_min) / (number_waves - 1)

            self._waves = list()
            wave_prog = Progress(self, start=0.07, end=0.10, nreports=number_waves)
            for idx in range(0, number_waves):
                wave_prog.report("Appending wave data: %i" % idx)
                self._waves.append(wave_min + idx * wave_bin)
            DeleteWorkspace(wave_range, EnableLogging=False)

            if self._emode == "Elastic":
                self._elastic = self._waves[int(len(self._waves) / 2)]
                logger.information("Elastic lambda : %f" % self._elastic)

            logger.information("Lambda : %i values from %f to %f" % (len(self._waves), self._waves[0], self._waves[-1]))

    # ------------------------------------------------------------------------------

    def _getEfixed(self):
        inst = mtd[self._sample_ws_name].getInstrument()

        if inst.hasParameter("Efixed"):
            return inst.getNumberParameter("EFixed")[0]

        if inst.hasParameter("analyser"):
            analyser_name = inst.getStringParameter("analyser")[0]
            analyser_comp = inst.getComponentByName(analyser_name)

            if analyser_comp is not None and analyser_comp.hasParameter("Efixed"):
                return analyser_comp.getNumberParameter("EFixed")[0]

        raise ValueError("No Efixed parameter found")

    # ------------------------------------------------------------------------------

    def _transmission(self):
        distance = self._radii[1] - self._radii[0]
        trans = math.exp(-distance * self._density[0] * (self._sig_s[0] + self._sig_a[0]))
        logger.information("Sample transmission : %f" % trans)
        if self._use_can:
            distance = self._radii[2] - self._radii[1]
            trans = math.exp(-distance * self._density[1] * (self._sig_s[1] + self._sig_a[1]))
            logger.information("Can transmission : %f" % trans)

    # ------------------------------------------------------------------------------

    def _interpolate_corrections(self, workspaces):
        """
        Performs interpolation on the correction workspaces such that the number of bins
        matches that of the input sample workspace.

        @param workspaces List of correction workspaces to interpolate
        """

        for ws in workspaces:
            SplineInterpolation(
                WorkspaceToMatch=self._sample_ws_name, WorkspaceToInterpolate=ws, OutputWorkspace=ws, OutputWorkspaceDeriv=""
            )

    # ------------------------------------------------------------------------------

    def _cyl_abs(self, angle):
        #  Parameters :
        #  self._step_size - step size
        #  self._beam - beam parameters
        #  nan - number of annuli
        #  radii - list of radii (for each annulus)
        #  density - list of densities (for each annulus)
        #  sigs - list of scattering cross-sections (for each annulus)
        #  siga - list of absorption cross-sections (for each annulus)
        #  angle - list of angles
        #  wavelas - elastic wavelength
        #  waves - list of wavelengths
        #  Output parameters :  A1 - Ass ; A2 - Assc ; A3 - Acsc ; A4 - Acc

        amu_scat = np.zeros(self._number_can)
        amu_scat = self._density * self._sig_s
        sig_abs = np.zeros(self._number_can)
        sig_abs = self._density * self._sig_a
        amu_tot_i = np.zeros(self._number_can)
        amu_tot_s = np.zeros(self._number_can)

        theta = angle * math.pi / 180.0
        A1 = []
        A2 = []
        A3 = []
        A4 = []
        # loop over wavelengths
        for wave in self._waves:
            # loop over annuli
            if self._emode == "Elastic":
                amu_tot_i = amu_scat + sig_abs * self._elastic / 1.7979
                amu_tot_s = amu_scat + sig_abs * self._elastic / 1.7979
            elif self._emode == "Direct":
                amu_tot_i = amu_scat + sig_abs * self._fixed / 1.7979
                amu_tot_s = amu_scat + sig_abs * wave / 1.7979
            elif self._emode == "Indirect":
                amu_tot_i = amu_scat + sig_abs * wave / 1.7979
                amu_tot_s = amu_scat + sig_abs * self._fixed / 1.7979
            elif self._emode == "Efixed":
                amu_tot_i = amu_scat + sig_abs * self._fixed / 1.7979
                amu_tot_s = amu_scat + sig_abs * self._fixed / 1.7979
            (Ass, Assc, Acsc, Acc) = self._acyl(theta, amu_scat, amu_tot_i, amu_tot_s)
            A1.append(Ass)
            A2.append(Assc)
            A3.append(Acsc)
            A4.append(Acc)
        return A1, A2, A3, A4

    # ------------------------------------------------------------------------------

    def _acyl(self, theta, amu_scat, amu_tot_i, amu_tot_s):
        A = self._beam[1]
        Area_s = 0.0
        Ass = 0.0
        Acc = 0.0
        Acsc = 0.0
        Assc = 0.0
        nan = self._number_can
        if self._number_can < 2:
            #
            #  No. STEPS ARE CHOSEN SO THAT STEP WIDTH IS THE SAME FOR ALL ANNULI
            #
            AAAA, BBBA, Area_A = self._sum_rom(0, 0, A, self._radii[0], self._radii[1], self._ms, theta, amu_scat, amu_tot_i, amu_tot_s)
            AAAB, BBBB, Area_B = self._sum_rom(0, 0, -A, self._radii[0], self._radii[1], self._ms, theta, amu_scat, amu_tot_i, amu_tot_s)
            Area_s += Area_A + Area_B
            Ass += AAAA + AAAB
            Ass /= Area_s
        else:
            for i in range(0, self._number_can - 1):
                radius_1 = self._radii[i]
                radius_2 = self._radii[i + 1]
                #
                #  No. STEPS ARE CHOSEN SO THAT STEP WIDTH IS THE SAME FOR ALL ANNULI
                #
                ms = int(self._ms * (radius_2 - radius_1) / (self._radii[1] - self._radii[0]))
                if ms < 1:
                    ms = 1
                AAAA, BBBA, Area_A = self._sum_rom(i, 0, A, radius_1, radius_2, ms, theta, amu_scat, amu_tot_i, amu_tot_s)
                AAAB, BBBB, Area_B = self._sum_rom(i, 0, -A, radius_1, radius_2, ms, theta, amu_scat, amu_tot_i, amu_tot_s)
                Area_s += Area_A + Area_B
                Ass += AAAA + AAAB
                Assc += BBBA + BBBB
            Ass = Ass / Area_s
            Assc = Assc / Area_s
            radius_1 = self._radii[nan - 1]
            radius_2 = self._radii[nan]
            ms = int(self._ms * (radius_2 - radius_1) / (self._radii[1] - self._radii[0]))
            if ms < 1:
                ms = 1
            AAAA, BBBA, Area_A = self._sum_rom(nan - 1, 1, A, radius_1, radius_2, ms, theta, amu_scat, amu_tot_i, amu_tot_s)
            AAAB, BBBB, Area_B = self._sum_rom(nan - 1, 1, -A, radius_1, radius_2, ms, theta, amu_scat, amu_tot_i, amu_tot_s)
            Area_C = Area_A + Area_B
            Acsc = (AAAA + AAAB) / Area_C
            Acc = (BBBA + BBBB) / Area_C
        return Ass, Assc, Acsc, Acc

    # ------------------------------------------------------------------------------

    def _sum_rom(self, n_scat, n_abs, a, r1, r2, ms, theta, amu_scat, amu_tot_i, amu_tot_s):
        # n_scat is region for scattering
        # n_abs is region for absorption
        nan = self._number_can
        omega_add = 0.0
        if a < 0.0:
            omega_add = math.pi
        AAA = 0.0
        BBB = 0.0
        Area = 0.0
        theta_deg = math.pi - theta
        num = ms
        r_step = (r2 - r1) / ms
        r_add = -0.5 * r_step + r1

        # start loop over M
        for M in range(1, num + 1):
            r = M * r_step + r_add
            number_omega = int(math.pi * r / r_step)
            omega_ster = math.pi / number_omega
            omega_deg = -0.5 * omega_ster + omega_add
            Area_y = r * r_step * omega_ster * amu_scat[n_scat]
            sum_1 = 0.0
            sum_2 = 0.0
            i = 1
            Area_sum = 0.0
            for _ in range(1, number_omega + 1):
                omega = i * omega_ster + omega_deg
                distance = r * math.sin(omega)

                skip = True
                if abs(distance) <= a:
                    #
                    # CALCULATE DISTANCE INCIDENT NEUTRON PASSES THROUGH EACH ANNULUS
                    LIS = []
                    for j in range(0, nan):
                        LIST = self._distance(r, self._radii[j], omega)
                        LISN = self._distance(r, self._radii[j + 1], omega)
                        LIS.append(LISN - LIST)
                    #
                    # CALCULATE DISTANCE SCATTERED NEUTRON PASSES THROUGH EACH ANNULUS
                    omega_scattered = omega + theta_deg
                    LSS = []
                    for j in range(0, nan):
                        LSST = self._distance(r, self._radii[j], omega_scattered)
                        LSSN = self._distance(r, self._radii[j + 1], omega_scattered)
                        LSS.append(LSSN - LSST)
                    #
                    # CALCULATE ABSORPTION FOR PATH THROUGH ALL ANNULI,AND THROUGH INNER ANNULI
                    path = np.zeros(3)
                    # 	split into input (I) and scattered (S) paths
                    path[0] += amu_tot_i[0] * LIS[0] + amu_tot_s[0] * LSS[0]
                    if nan == 2:
                        path[2] += amu_tot_i[1] * LIS[1] + amu_tot_s[1] * LSS[1]
                        path[1] = path[0] + path[2]
                    sum_1 += math.exp(-path[n_abs])
                    sum_2 += math.exp(-path[n_abs + 1])
                    Area_sum += 1.0
                    skip = False

                if skip:
                    i = number_omega - i + 2
                else:
                    i += 1
        AAA += sum_1 * Area_y
        BBB += sum_2 * Area_y
        Area += Area_sum * Area_y
        return AAA, BBB, Area

    # ------------------------------------------------------------------------------

    def _distance(self, r1, radius, omega):
        r = r1
        distance = 0.0
        b = r * math.sin(omega)
        if abs(b) < radius:
            t = r * math.cos(omega)
            c = radius * radius - b * b
            d = math.sqrt(c)
            if r <= radius:
                distance = t + d
            else:
                distance = d * (1.0 + math.copysign(1.0, t))
        return distance


# ------------------------------------------------------------------------------


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CylinderPaalmanPingsCorrection)
