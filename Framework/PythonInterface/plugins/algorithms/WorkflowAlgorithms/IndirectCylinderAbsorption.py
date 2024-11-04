# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init, too-many-instance-attributes
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, WorkspaceGroupProperty, PropertyMode, Progress
from mantid.kernel import (
    StringMandatoryValidator,
    Direction,
    logger,
    FloatBoundedValidator,
    IntBoundedValidator,
    MaterialBuilder,
    StringListValidator,
)
from mantid.simpleapi import (
    AddSampleLogMultiple,
    AnnularRingAbsorption,
    ConvertUnits,
    CylinderAbsorption,
    DeleteWorkspace,
    Divide,
    GroupWorkspaces,
    Minus,
    Scale,
    SetSampleMaterial,
)

from IndirectCommon import get_efixed


class IndirectCylinderAbsorption(DataProcessorAlgorithm):
    # Sample variables
    _sample_ws_name = None
    _sample_chemical_formula = None
    _sample_density_type = None
    _sample_density = None
    _sample_radius = None

    # Container variables
    _can_ws_name = None
    _use_can_corrections = None
    _can_chemical_formula = None
    _can_density_type = None
    _can_density = None
    _can_radius = None
    _can_scale = None

    _events = None
    _output_ws = None
    _abs_ws = None
    _ass_ws = None
    _acc_ws = None

    def category(self):
        return "Workflow\\Inelastic;CorrectionFunctions\\AbsorptionCorrections;Workflow\\MIDAS"

    def summary(self):
        return "Calculates indirect absorption corrections for a cylinder sample shape."

    def PyInit(self):
        # Sample options
        self.declareProperty(MatrixWorkspaceProperty("SampleWorkspace", "", direction=Direction.Input), doc="Sample workspace.")
        self.declareProperty(
            name="SampleChemicalFormula", defaultValue="", validator=StringMandatoryValidator(), doc="Sample chemical formula"
        )
        self.declareProperty(
            name="SampleDensityType",
            defaultValue="Mass Density",
            validator=StringListValidator(["Mass Density", "Number Density"]),
            doc="Use of Mass density or Number density",
        )
        self.declareProperty(name="SampleDensity", defaultValue=0.1, doc="Mass density (g/cm^3) or Number density (atoms/Angstrom^3)")
        self.declareProperty(name="SampleRadius", defaultValue=0.1, validator=FloatBoundedValidator(0.0), doc="Sample radius")

        # Container options
        self.declareProperty(
            MatrixWorkspaceProperty("CanWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="Container workspace.",
        )
        self.declareProperty(name="UseCanCorrections", defaultValue=False, doc="Use can corrections in subtraction")
        self.declareProperty(name="CanChemicalFormula", defaultValue="", doc="Can chemical formula")
        self.declareProperty(
            name="CanDensityType",
            defaultValue="Mass Density",
            validator=StringListValidator(["Mass Density", "Number Density"]),
            doc="Use of Mass density or Number density",
        )
        self.declareProperty(name="CanDensity", defaultValue=0.1, doc="Mass density (g/cm^3) or Number density (atoms/Angstrom^3)")
        self.declareProperty(name="CanRadius", defaultValue=0.2, validator=FloatBoundedValidator(0.0), doc="Can radius")
        self.declareProperty(
            name="CanScaleFactor", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Scale factor to multiply can data"
        )

        # General options
        self.declareProperty(name="Events", defaultValue=5000, validator=IntBoundedValidator(0), doc="Number of neutron events")

        # Output options
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="The output corrected workspace."
        )

        self.declareProperty(
            WorkspaceGroupProperty("CorrectionsWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The corrections workspace for scattering and absorptions in sample.",
        )

    # pylint: disable=too-many-branches
    def PyExec(self):
        self._setup()

        # Set up progress reporting
        n_prog_reports = 2
        if self._can_ws_name is not None:
            n_prog_reports += 1
        prog = Progress(self, 0.0, 1.0, n_prog_reports)

        efixed = get_efixed(self._sample_ws_name)

        sample_wave_ws = "__sam_wave"
        ConvertUnits(
            InputWorkspace=self._sample_ws_name,
            OutputWorkspace=sample_wave_ws,
            Target="Wavelength",
            EMode="Indirect",
            EFixed=efixed,
            EnableLogging=False,
        )

        prog.report("Calculating sample corrections")
        if self._sample_density_type == "Mass Density":
            builder = MaterialBuilder()
            mat = builder.setFormula(self._sample_chemical_formula).setMassDensity(self._sample_density).build()
            self._sample_density = mat.numberDensity
        SetSampleMaterial(sample_wave_ws, ChemicalFormula=self._sample_chemical_formula, SampleNumberDensity=self._sample_density)
        prog.report("Calculating sample corrections")
        CylinderAbsorption(
            InputWorkspace=sample_wave_ws,
            OutputWorkspace=self._ass_ws,
            SampleNumberDensity=self._sample_density,
            NumberOfWavelengthPoints=10,
            CylinderSampleHeight=3.0,
            CylinderSampleRadius=self._sample_radius,
            NumberOfSlices=1,
            NumberOfAnnuli=10,
        )

        group = self._ass_ws

        if self._can_ws_name is not None:
            can_wave_ws = "__can_wave"
            ConvertUnits(
                InputWorkspace=self._can_ws_name,
                OutputWorkspace=can_wave_ws,
                Target="Wavelength",
                EMode="Indirect",
                EFixed=efixed,
                EnableLogging=False,
            )
            if self._can_scale != 1.0:
                logger.information("Scaling can by: " + str(self._can_scale))
                Scale(InputWorkspace=can_wave_ws, OutputWorkspace=can_wave_ws, Factor=self._can_scale, Operation="Multiply")

            can_thickness = self._can_radius - self._sample_radius
            logger.information("Container thickness: " + str(can_thickness))

            if self._use_can_corrections:
                # Doing can corrections
                prog.report("Calculating container corrections")
                Divide(LHSWorkspace=sample_wave_ws, RHSWorkspace=self._ass_ws, OutputWorkspace=sample_wave_ws)

                if self._sample_density_type == "Mass Density":
                    builder = MaterialBuilder()
                    mat = builder.setFormula(self._can_chemical_formula).setMassDensity(self._can_density).build()
                    self._can_density = mat.numberDensity
                SetSampleMaterial(can_wave_ws, ChemicalFormula=self._can_chemical_formula, SampleNumberDensity=self._can_density)

                AnnularRingAbsorption(
                    InputWorkspace=can_wave_ws,
                    OutputWorkspace=self._acc_ws,
                    SampleHeight=3.0,
                    SampleThickness=can_thickness,
                    CanInnerRadius=0.9 * self._sample_radius,
                    CanOuterRadius=1.1 * self._can_radius,
                    SampleChemicalFormula=self._can_chemical_formula,
                    SampleNumberDensity=self._can_density,
                    NumberOfWavelengthPoints=10,
                    EventsPerPoint=self._events,
                )

                Divide(LHSWorkspace=can_wave_ws, RHSWorkspace=self._acc_ws, OutputWorkspace=can_wave_ws)
                Minus(LHSWorkspace=sample_wave_ws, RHSWorkspace=can_wave_ws, OutputWorkspace=sample_wave_ws)
                group += "," + self._acc_ws

            else:
                # Doing simple can subtraction
                prog.report("Calculating container scaling")
                Minus(LHSWorkspace=sample_wave_ws, RHSWorkspace=can_wave_ws, OutputWorkspace=sample_wave_ws)
                Divide(LHSWorkspace=sample_wave_ws, RHSWorkspace=self._ass_ws, OutputWorkspace=sample_wave_ws)

            DeleteWorkspace(can_wave_ws, EnableLogging=False)

        else:
            Divide(LHSWorkspace=sample_wave_ws, RHSWorkspace=self._ass_ws, OutputWorkspace=sample_wave_ws)

        ConvertUnits(
            InputWorkspace=sample_wave_ws,
            OutputWorkspace=self._output_ws,
            Target="DeltaE",
            EMode="Indirect",
            EFixed=efixed,
            EnableLogging=False,
        )
        DeleteWorkspace(sample_wave_ws, EnableLogging=False)

        # Record sample logs
        prog.report("Recording sample logs")
        sample_log_workspaces = [self._output_ws, self._ass_ws]
        sample_logs = [("sample_shape", "cylinder"), ("sample_filename", self._sample_ws_name), ("sample_radius", self._sample_radius)]

        if self._can_ws_name is not None:
            sample_logs.append(("container_filename", self._can_ws_name))
            sample_logs.append(("container_scale", self._can_scale))
            if self._use_can_corrections:
                sample_log_workspaces.append(self._acc_ws)
                sample_logs.append(("container_thickness", can_thickness))

        log_names = [item[0] for item in sample_logs]
        log_values = [item[1] for item in sample_logs]

        for ws_name in sample_log_workspaces:
            AddSampleLogMultiple(Workspace=ws_name, LogNames=log_names, LogValues=log_values, EnableLogging=False)

        self.setProperty("OutputWorkspace", self._output_ws)

        # Output the Abs group workspace if it is wanted, delete if not
        if self._abs_ws == "":
            DeleteWorkspace(self._ass_ws, EnableLogging=False)
            if self._can_ws_name is not None and self._use_can_corrections:
                DeleteWorkspace(self._acc_ws, EnableLogging=False)

        else:
            GroupWorkspaces(InputWorkspaces=group, OutputWorkspace=self._abs_ws, EnableLogging=False)
            self.setProperty("CorrectionsWorkspace", self._abs_ws)

    def _setup(self):
        """
        Get algorithm properties.
        """

        self._sample_ws_name = self.getPropertyValue("SampleWorkspace")
        self._sample_chemical_formula = self.getPropertyValue("SampleChemicalFormula")
        self._sample_density_type = self.getPropertyValue("SampleDensityType")
        self._sample_density = self.getProperty("SampleDensity").value
        self._sample_radius = self.getProperty("SampleRadius").value

        self._can_ws_name = self.getPropertyValue("CanWorkspace")
        if self._can_ws_name == "":
            self._can_ws_name = None

        self._use_can_corrections = self.getProperty("UseCanCorrections").value
        self._can_chemical_formula = self.getPropertyValue("CanChemicalFormula")
        self._can_density_type = self.getPropertyValue("CanDensityType")
        self._can_density = self.getProperty("CanDensity").value
        self._can_radius = self.getProperty("CanRadius").value
        self._can_scale = self.getProperty("CanScaleFactor").value

        self._events = self.getPropertyValue("Events")

        self._output_ws = self.getPropertyValue("OutputWorkspace")

        self._abs_ws = self.getPropertyValue("CorrectionsWorkspace")
        if self._abs_ws == "":
            self._ass_ws = "__ass"
            self._acc_ws = "__acc"
        else:
            self._ass_ws = self._abs_ws + "_ass"
            self._acc_ws = self._abs_ws + "_acc"

    def validateInputs(self):
        """
        Validate algorithm options.
        """

        self._setup()
        issues = dict()

        if self._sample_radius > self._can_radius:
            issues["CanRadius"] = "Must be greater than SampleRadius"

        if self._use_can_corrections and self._can_chemical_formula == "":
            issues["CanChemicalFormula"] = "Must be set to use can corrections"

        if self._use_can_corrections and self._can_ws_name is None:
            issues["UseCanCorrections"] = "Must specify a can workspace to use can corrections"

        return issues


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectCylinderAbsorption)
