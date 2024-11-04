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
    PropertyMode,
    Progress,
    WorkspaceGroupProperty,
    mtd,
)
from mantid.kernel import StringMandatoryValidator, Direction, logger, IntBoundedValidator, FloatBoundedValidator, StringListValidator


class IndirectAnnulusAbsorption(DataProcessorAlgorithm):
    # Sample variables
    _sample_ws_name = ""
    _sample_chemical_formula = ""
    _sample_density_type = None
    _sample_density = 0.0
    _sample_inner_radius = 0.0
    _sample_outer_radius = 0.0

    # Container variables
    _can_ws_name = ""
    _can_chemical_formula = ""
    _can_density_type = None
    _can_density = 0.0
    _can_inner_radius = 0.0
    _can_outer_radius = 0.0
    _use_can_corrections = False
    _can_scale = 0.0

    _output_ws = None
    _ass_ws = None
    _abs_ws = None
    _acc_ws = None
    _events = 0

    def category(self):
        return "Workflow\\Inelastic;CorrectionFunctions\\AbsorptionCorrections;Workflow\\MIDAS"

    def summary(self):
        return "Calculates indirect absorption corrections for an annulus sample shape."

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
            doc="Use of Mass density or Number density",
        )
        self.declareProperty(name="SampleDensity", defaultValue=0.1, doc="Mass density (g/cm^3) or Number density (atoms/Angstrom^3)")
        self.declareProperty(name="SampleInnerRadius", defaultValue=0.23, validator=FloatBoundedValidator(0.0), doc="Sample radius")
        self.declareProperty(name="SampleOuterRadius", defaultValue=0.27, validator=FloatBoundedValidator(0.0), doc="Sample radius")
        self.declareProperty(name="SampleHeight", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Sample height")

        # Container options
        self.declareProperty(
            MatrixWorkspaceProperty("CanWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="Container workspace.",
        )
        self.declareProperty(name="UseCanCorrections", defaultValue=False, doc="Use Container corrections in subtraction")
        self.declareProperty(name="CanChemicalFormula", defaultValue="", doc="Chemical formula for the Container")
        self.declareProperty(
            name="CanDensityType",
            defaultValue="Mass Density",
            validator=StringListValidator(["Mass Density", "Number Density"]),
            doc="Container density type.",
        )
        self.declareProperty(name="CanDensity", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Container number density")
        self.declareProperty(name="CanInnerRadius", defaultValue=0.19, validator=FloatBoundedValidator(0.0), doc="Container inner radius")
        self.declareProperty(name="CanOuterRadius", defaultValue=0.35, validator=FloatBoundedValidator(0.0), doc="Container outer radius")
        self.declareProperty(
            name="CanScaleFactor", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Scale factor to multiply Container data"
        )

        # Beam size
        self.declareProperty(name="BeamHeight", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Height of the beam (cm)")
        self.declareProperty(name="BeamWidth", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Width of the beam (cm)")

        # General options
        self.declareProperty(
            name="NumberWavelengths", defaultValue=10, validator=IntBoundedValidator(1), doc="Number of wavelengths for calculation"
        )
        self.declareProperty(name="Events", defaultValue=1000, validator=IntBoundedValidator(0), doc="Number of neutron events")

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

        sample_thickness = self._sample_outer_radius - self._sample_inner_radius
        logger.information("Sample thickness: " + str(sample_thickness))

        prog.report("Calculating sample corrections")
        SetBeam(sample_wave_ws, Geometry={"Shape": "Slit", "Width": self._beam_width, "Height": self._beam_height})

        if self._sample_density_type == "Mass Density":
            sample_mat_list = {"ChemicalFormula": self._sample_chemical_formula, "SampleMassDensity": self._sample_density}
        if self._sample_density_type == "Number Density":
            sample_mat_list = {"ChemicalFormula": self._sample_chemical_formula, "SampleNumberDensity": self._sample_density}
        SetSample(
            sample_wave_ws,
            Geometry={
                "Shape": "HollowCylinder",
                "Height": self._sample_height,
                "InnerRadius": self._sample_inner_radius,
                "OuterRadius": self._sample_outer_radius,
                "Center": [0.0, 0.0, 0.0],
                "Axis": 1,
            },
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
        multiply_alg = self.createChildAlgorithm("Multiply", enableLogging=False)

        if self._can_ws_name is not None:
            can1_wave_ws = "__can1_wave"
            can2_wave_ws = "__can2_wave"
            convert_unit_alg.setProperty("InputWorkspace", self._can_ws_name)
            convert_unit_alg.setProperty("OutputWorkspace", can1_wave_ws)
            convert_unit_alg.setProperty("Target", "Wavelength")
            convert_unit_alg.setProperty("EMode", self._emode)
            convert_unit_alg.setProperty("EFixed", self._efixed)
            convert_unit_alg.execute()
            mtd.addOrReplace(can1_wave_ws, convert_unit_alg.getProperty("OutputWorkspace").value)

            if self._can_scale != 1.0:
                logger.information("Scaling container by: %s" % self._can_scale)
                scale_alg = self.createChildAlgorithm("Scale", enableLogging=False)
                scale_alg.setProperty("InputWorkspace", can1_wave_ws)
                scale_alg.setProperty("OutputWorkspace", can1_wave_ws)
                scale_alg.setProperty("Factor", self._can_scale)
                scale_alg.setProperty("Operation", "Multiply")
                scale_alg.execute()
            clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
            clone_alg.setProperty("InputWorkspace", can1_wave_ws)
            clone_alg.setProperty("OutputWorkspace", can2_wave_ws)
            clone_alg.execute()
            mtd.addOrReplace(can2_wave_ws, clone_alg.getProperty("OutputWorkspace").value)

            can_thickness_1 = self._sample_inner_radius - self._can_inner_radius
            can_thickness_2 = self._can_outer_radius - self._sample_outer_radius
            logger.information("Container thickness: %f & %f" % (can_thickness_1, can_thickness_2))

            if self._use_can_corrections:
                prog.report("Calculating container corrections")
                divide_alg.setProperty("LHSWorkspace", sample_wave_ws)
                divide_alg.setProperty("RHSWorkspace", self._ass_ws)
                divide_alg.setProperty("OutputWorkspace", sample_wave_ws)
                divide_alg.execute()

                if self._sample_density_type == "Mass Density":
                    container_mat_list = {"ChemicalFormula": self._can_chemical_formula, "SampleMassDensity": self._can_density}
                if self._sample_density_type == "Number Density":
                    container_mat_list = {"ChemicalFormula": self._can_chemical_formula, "SampleNumberDensity": self._can_density}

                SetBeam(can1_wave_ws, Geometry={"Shape": "Slit", "Width": self._beam_width, "Height": self._beam_height})

                SetSample(
                    can1_wave_ws,
                    Geometry={
                        "Shape": "HollowCylinder",
                        "Height": float(self._sample_height),
                        "InnerRadius": float(self._can_inner_radius),
                        "OuterRadius": float(self._sample_inner_radius),
                        "Center": [0.0, 0.0, 0.0],
                        "Axis": 1,
                    },
                    Material=container_mat_list,
                )

                MonteCarloAbsorption(
                    InputWorkspace=can1_wave_ws,
                    OutputWorkspace="__Acc1",
                    EventsPerPoint=self._events,
                    NumberOfWavelengthPoints=self._number_wavelengths,
                    Interpolation="CSpline",
                )

                SetBeam(can2_wave_ws, Geometry={"Shape": "Slit", "Width": self._beam_width, "Height": self._beam_height})

                SetSample(
                    can2_wave_ws,
                    Geometry={
                        "Shape": "HollowCylinder",
                        "Height": float(self._sample_height),
                        "InnerRadius": float(self._sample_outer_radius),
                        "OuterRadius": float(self._can_outer_radius),
                        "Center": [0.0, 0.0, 0.0],
                        "Axis": 1,
                    },
                    Material=container_mat_list,
                )

                MonteCarloAbsorption(
                    InputWorkspace=can2_wave_ws,
                    OutputWorkspace="__Acc2",
                    EventsPerPoint=self._events,
                    NumberOfWavelengthPoints=self._number_wavelengths,
                    Interpolation="CSpline",
                )

                multiply_alg.setProperty("LHSWorkspace", "__Acc1")
                multiply_alg.setProperty("RHSWorkspace", "__Acc2")
                multiply_alg.setProperty("OutputWorkspace", self._acc_ws)
                multiply_alg.execute()
                mtd.addOrReplace(self._acc_ws, multiply_alg.getProperty("OutputWorkspace").value)
                delete_alg.setProperty("Workspace", "__Acc1")
                delete_alg.execute()
                delete_alg.setProperty("Workspace", "__Acc2")
                delete_alg.execute()

                divide_alg.setProperty("LHSWorkspace", can1_wave_ws)
                divide_alg.setProperty("RHSWorkspace", self._acc_ws)
                divide_alg.setProperty("OutputWorkspace", can1_wave_ws)
                divide_alg.execute()
                minus_alg.setProperty("LHSWorkspace", sample_wave_ws)
                minus_alg.setProperty("RHSWorkspace", can1_wave_ws)
                minus_alg.setProperty("OutputWorkspace", sample_wave_ws)
                minus_alg.execute()
                group += "," + self._acc_ws

            else:
                prog.report("Calculating can scaling")
                minus_alg.setProperty("LHSWorkspace", sample_wave_ws)
                minus_alg.setProperty("RHSWorkspace", can1_wave_ws)
                minus_alg.setProperty("OutputWorkspace", sample_wave_ws)
                minus_alg.execute()
                divide_alg.setProperty("LHSWorkspace", sample_wave_ws)
                divide_alg.setProperty("RHSWorkspace", self._ass_ws)
                divide_alg.setProperty("OutputWorkspace", sample_wave_ws)
                divide_alg.execute()

            delete_alg.setProperty("Workspace", can1_wave_ws)
            delete_alg.execute()
            delete_alg.setProperty("Workspace", can2_wave_ws)
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

        prog.report("Recording sample logs")
        sample_log_workspaces = [self._output_ws, self._ass_ws]
        sample_logs = [
            ("sample_shape", "annulus"),
            ("sample_filename", self._sample_ws_name),
            ("sample_inner", self._sample_inner_radius),
            ("sample_outer", self._sample_outer_radius),
            ("can_inner", self._can_inner_radius),
            ("can_outer", self._can_outer_radius),
        ]

        if self._can_ws_name is not None:
            sample_logs.append(("container_filename", self._can_ws_name))
            sample_logs.append(("container_scale", self._can_scale))
            if self._use_can_corrections:
                sample_log_workspaces.append(self._acc_ws)
                sample_logs.append(("container_thickness_1", can_thickness_1))
                sample_logs.append(("container_thickness_2", can_thickness_2))

        log_names = [item[0] for item in sample_logs]
        log_values = [item[1] for item in sample_logs]

        add_sample_log_alg = self.createChildAlgorithm("AddSampleLogMultiple", enableLogging=False)
        for ws_name in sample_log_workspaces:
            add_sample_log_alg.setProperty("Workspace", ws_name)
            add_sample_log_alg.setProperty("LogNames", log_names)
            add_sample_log_alg.setProperty("LogValues", log_values)
            add_sample_log_alg.execute()

        self.setProperty("OutputWorkspace", self._output_ws)

        # Output the Ass workspace if it is wanted, delete if not
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
        self._sample_inner_radius = self.getProperty("SampleInnerRadius").value
        self._sample_outer_radius = self.getProperty("SampleOuterRadius").value
        self._sample_height = self.getProperty("SampleHeight").value

        self._can_ws_name = self.getPropertyValue("CanWorkspace")
        if self._can_ws_name == "":
            self._can_ws_name = None
        self._use_can_corrections = self.getProperty("UseCanCorrections").value
        self._can_chemical_formula = self.getPropertyValue("CanChemicalFormula")
        self._can_density_type = self.getPropertyValue("CanDensityType")
        self._can_density = self.getProperty("CanDensity").value
        self._can_inner_radius = self.getProperty("CanInnerRadius").value
        self._can_outer_radius = self.getProperty("CanOuterRadius").value
        self._can_scale = self.getProperty("CanScaleFactor").value

        self._beam_height = float(self.getProperty("BeamHeight").value)
        self._beam_width = float(self.getProperty("BeamWidth").value)

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

        if self._use_can_corrections and self._can_chemical_formula == "":
            issues["CanChemicalFormula"] = "Must be set to use can corrections"

        if self._use_can_corrections and self._can_ws_name is None:
            issues["UseCanCorrections"] = "Must specify a can workspace to use can corrections"

        # Geometry validation: can inner < sample inner < sample outer < can outer
        if self._sample_outer_radius <= self._sample_inner_radius:
            issues["SampleOuterRadius"] = "Must be greater than SampleInnerRadius"

        if self._can_ws_name is not None:
            if self._sample_inner_radius <= self._can_inner_radius:
                issues["SampleInnerRadius"] = "Must be greater than CanInnerRadius"

            if self._can_outer_radius <= self._sample_outer_radius:
                issues["CanOuterRadius"] = "Must be greater than SampleOuterRadius"

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
AlgorithmFactory.subscribe(IndirectAnnulusAbsorption)
