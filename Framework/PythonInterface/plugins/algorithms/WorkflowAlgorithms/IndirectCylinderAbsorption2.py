# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import SetBeam, SetSample, MonteCarloAbsorption, GroupWorkspaces
from mantid.api import (
    DataProcessorAlgorithm,
    AlgorithmFactory,
    MatrixWorkspaceProperty,
    WorkspaceGroupProperty,
    PropertyMode,
    Progress,
    mtd,
)
from mantid.kernel import StringMandatoryValidator, Direction, logger, FloatBoundedValidator, IntBoundedValidator, StringListValidator


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

    def version(self):
        return 2

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
            doc="Use of Mass Density or Number density",
        )
        self.declareProperty(name="SampleDensity", defaultValue=0.1, doc="Mass Density (g/cm^3) or Number density (atoms/Angstrom^3)")
        self.declareProperty(name="SampleRadius", defaultValue=0.1, validator=FloatBoundedValidator(0.0), doc="Sample radius")
        self.declareProperty(name="SampleHeight", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Sample height")

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
            doc="Use of Mass Density or Number density",
        )
        self.declareProperty(name="CanDensity", defaultValue=0.1, doc="Mass Density (g/cm^3) or Number density (atoms/Angstrom^3)")
        self.declareProperty(name="CanRadius", defaultValue=0.2, validator=FloatBoundedValidator(0.0), doc="Can radius")
        self.declareProperty(
            name="CanScaleFactor", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Scale factor to multiply can data"
        )

        # Beam size
        self.declareProperty(name="BeamHeight", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Height of the beam (cm)")
        self.declareProperty(name="BeamWidth", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Width of the beam (cm)")

        # General options
        self.declareProperty(
            name="NumberWavelengths", defaultValue=10, validator=IntBoundedValidator(1), doc="Number of wavelengths for calculation"
        )

        self.declareProperty(name="Events", defaultValue=5000, validator=IntBoundedValidator(0), doc="Number of neutron events")

        # Output options
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="The output corrected workspace."
        )

        self.declareProperty(
            WorkspaceGroupProperty("CorrectionsWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The corrections workspace for scattering and absorptions in sample.",
        )

    def PyExec(self):
        # Set up progress reporting
        n_prog_reports = 2
        if self._can_ws_name is not None:
            n_prog_reports += 1
        prog = Progress(self, 0.0, 1.0, n_prog_reports)

        sample_wave_ws = "__sam_wave"
        convert_unit_alg = self.createChildAlgorithm("ConvertUnits", enableLogging=False)
        convert_unit_alg.setProperty("InputWorkspace", self._sample_ws_name)
        convert_unit_alg.setProperty("OutputWorkspace", sample_wave_ws)
        convert_unit_alg.setProperty("Target", "Wavelength")
        convert_unit_alg.setProperty("EMode", self._emode)
        convert_unit_alg.setProperty("EFixed", self._efixed)
        convert_unit_alg.execute()
        mtd.addOrReplace(sample_wave_ws, convert_unit_alg.getProperty("OutputWorkspace").value)

        prog.report("Calculating sample corrections")
        SetBeam(sample_wave_ws, Geometry={"Shape": "Slit", "Width": self._beam_width, "Height": self._beam_height})

        if self._sample_density_type == "Mass Density":
            sample_mat_list = {"ChemicalFormula": self._sample_chemical_formula, "SampleMassDensity": self._sample_density}
        if self._sample_density_type == "Number Density":
            sample_mat_list = {"ChemicalFormula": self._sample_chemical_formula, "SampleNumberDensity": self._sample_density}

        SetSample(
            sample_wave_ws,
            Geometry={"Shape": "Cylinder", "Height": self._sample_height, "Radius": self._sample_radius, "Center": [0.0, 0.0, 0.0]},
            Material=sample_mat_list,
        )

        prog.report("Calculating sample corrections")
        MonteCarloAbsorption(
            InputWorkspace=sample_wave_ws,
            OutputWorkspace=self._ass_ws,
            EventsPerPoint=self._events,
            NumberOfWavelengthPoints=self._number_wavelengths,
            Interpolation="CSpline",
        )

        group = self._ass_ws

        delete_alg = self.createChildAlgorithm("DeleteWorkspace", enableLogging=False)
        divide_alg = self.createChildAlgorithm("Divide", enableLogging=False)
        minus_alg = self.createChildAlgorithm("Minus", enableLogging=False)

        if self._can_ws_name is not None:
            can_wave_ws = "__can_wave"
            convert_unit_alg.setProperty("InputWorkspace", self._can_ws_name)
            convert_unit_alg.setProperty("OutputWorkspace", can_wave_ws)
            convert_unit_alg.setProperty("Target", "Wavelength")
            convert_unit_alg.setProperty("EMode", self._emode)
            convert_unit_alg.setProperty("EFixed", self._efixed)
            convert_unit_alg.execute()
            mtd.addOrReplace(can_wave_ws, convert_unit_alg.getProperty("OutputWorkspace").value)

            if self._can_scale != 1.0:
                logger.information("Scaling can by: %s" % self._can_scale)
                scale_alg = self.createChildAlgorithm("Scale", enableLogging=False)
                scale_alg.setProperty("InputWorkspace", can_wave_ws)
                scale_alg.setProperty("OutputWorkspace", can_wave_ws)
                scale_alg.setProperty("Factor", self._can_scale)
                scale_alg.setProperty("Operation", "Multiply")
                scale_alg.execute()

            can_thickness = self._can_radius - self._sample_radius
            logger.information("Container thickness: " + str(can_thickness))

            if self._use_can_corrections:
                # Doing can corrections
                prog.report("Calculating container corrections")
                divide_alg.setProperty("LHSWorkspace", sample_wave_ws)
                divide_alg.setProperty("RHSWorkspace", self._ass_ws)
                divide_alg.setProperty("OutputWorkspace", sample_wave_ws)
                divide_alg.execute()

                if self._sample_density_type == "Mass Density":
                    container_mat_list = {"ChemicalFormula": self._can_chemical_formula, "SampleMassDensity": self._can_density}
                if self._sample_density_type == "Number Density":
                    container_mat_list = {"ChemicalFormula": self._can_chemical_formula, "SampleNumberDensity": self._can_density}

                SetSample(
                    can_wave_ws,
                    Geometry={
                        "Shape": "HollowCylinder",
                        "Height": self._sample_height,
                        "InnerRadius": self._sample_radius,
                        "OuterRadius": self._can_radius,
                        "Center": [0.0, 0.0, 0.0],
                    },
                    Material=container_mat_list,
                )

                MonteCarloAbsorption(
                    InputWorkspace=can_wave_ws,
                    OutputWorkspace=self._acc_ws,
                    EventsPerPoint=self._events,
                    NumberOfWavelengthPoints=self._number_wavelengths,
                    Interpolation="CSpline",
                )

                divide_alg.setProperty("LHSWorkspace", can_wave_ws)
                divide_alg.setProperty("RHSWorkspace", self._acc_ws)
                divide_alg.setProperty("OutputWorkspace", can_wave_ws)
                divide_alg.execute()
                minus_alg.setProperty("LHSWorkspace", sample_wave_ws)
                minus_alg.setProperty("RHSWorkspace", can_wave_ws)
                minus_alg.setProperty("OutputWorkspace", sample_wave_ws)
                minus_alg.execute()
                group += "," + self._acc_ws

            else:
                # Doing simple can subtraction
                prog.report("Calculating container scaling")
                minus_alg.setProperty("LHSWorkspace", sample_wave_ws)
                minus_alg.setProperty("RHSWorkspace", can_wave_ws)
                minus_alg.setProperty("OutputWorkspace", sample_wave_ws)
                minus_alg.execute()
                divide_alg.setProperty("LHSWorkspace", sample_wave_ws)
                divide_alg.setProperty("RHSWorkspace", self._ass_ws)
                divide_alg.setProperty("OutputWorkspace", sample_wave_ws)
                divide_alg.execute()

            delete_alg.setProperty("Workspace", can_wave_ws)
            delete_alg.execute()

        else:
            divide_alg.setProperty("LHSWorkspace", sample_wave_ws)
            divide_alg.setProperty("RHSWorkspace", self._ass_ws)
            divide_alg.setProperty("OutputWorkspace", sample_wave_ws)
            divide_alg.execute()

        convert_unit_alg.setProperty("InputWorkspace", sample_wave_ws)
        convert_unit_alg.setProperty("OutputWorkspace", self._output_ws)
        convert_unit_alg.setProperty("Target", "DeltaE")
        convert_unit_alg.setProperty("EMode", self._emode)
        convert_unit_alg.setProperty("EFixed", self._efixed)
        convert_unit_alg.execute()
        mtd.addOrReplace(self._output_ws, convert_unit_alg.getProperty("OutputWorkspace").value)
        delete_alg.setProperty("Workspace", sample_wave_ws)
        delete_alg.execute()

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

        add_sample_log_alg = self.createChildAlgorithm("AddSampleLogMultiple", enableLogging=False)
        for ws_name in sample_log_workspaces:
            add_sample_log_alg.setProperty("Workspace", ws_name)
            add_sample_log_alg.setProperty("LogNames", log_names)
            add_sample_log_alg.setProperty("LogValues", log_values)
            add_sample_log_alg.execute()

        self.setProperty("OutputWorkspace", self._output_ws)

        # Output the Abs group workspace if it is wanted, delete if not
        if self._abs_ws == "":
            delete_alg.setProperty("Workspace", self._ass_ws)
            delete_alg.execute()
            if self._can_ws_name is not None and self._use_can_corrections:
                delete_alg.setProperty("Workspace", self._acc_ws)
                delete_alg.execute()

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
        self._sample_height = self.getProperty("SampleHeight").value

        self._can_ws_name = self.getPropertyValue("CanWorkspace")
        if self._can_ws_name == "":
            self._can_ws_name = None

        self._use_can_corrections = self.getProperty("UseCanCorrections").value
        self._can_chemical_formula = self.getPropertyValue("CanChemicalFormula")
        self._can_density_type = self.getPropertyValue("CanDensityType")
        self._can_density = self.getProperty("CanDensity").value
        self._can_radius = self.getProperty("CanRadius").value
        self._can_scale = self.getProperty("CanScaleFactor").value

        self._beam_height = self.getProperty("BeamHeight").value
        self._beam_width = self.getProperty("BeamWidth").value

        self._emode = "Indirect"
        self._efixed = self._getEfixed()
        self._number_wavelengths = self.getProperty("NumberWavelengths").value
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

        if self._can_ws_name is not None:
            if self._sample_radius >= self._can_radius:
                issues["CanRadius"] = "Must be greater than SampleRadius"

        if self._use_can_corrections and self._can_chemical_formula == "":
            issues["CanChemicalFormula"] = "Must be set to use can corrections"

        if self._use_can_corrections and self._can_ws_name is None:
            issues["UseCanCorrections"] = "Must specify a can workspace to use can corrections"

        return issues

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


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectCylinderAbsorption)
