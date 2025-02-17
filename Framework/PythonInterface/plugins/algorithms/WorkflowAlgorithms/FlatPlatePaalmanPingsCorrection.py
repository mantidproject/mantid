# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-instance-attributes

import math

import numpy as np
from mantid.simpleapi import AddSampleLog, CreateWorkspace, DeleteWorkspace, ExtractSingleSpectrum, GroupWorkspaces, SplineInterpolation
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


class FlatPlatePaalmanPingsCorrection(PythonAlgorithm):
    # Useful constants
    PICONV = math.pi / 180.0
    TABULATED_WAVELENGTH = 1.798
    TABULATED_ENERGY = 25.305

    # Sample variables
    _sample_ws_name = None
    _sample_chemical_formula = None
    _sample_density_type = None
    _sample_density = None
    _sample_thickness = None
    _sample_angle = 0.0

    # Container Variables
    _use_can = False
    _can_ws_name = None
    _can_chemical_formula = None
    _can_density_type = None
    _can_density = None
    _can_front_thickness = None
    _can_back_thickness = None
    _has_sample_in = False
    _has_can_front_in = False
    _has_can_back_in = False

    _number_wavelengths = 10
    _emode = None
    _efixed = 0.0
    _output_ws_name = None
    _angles = list()
    _wavelengths = list()
    _interpolate = None

    # ------------------------------------------------------------------------------

    def category(self):
        return "Workflow\\MIDAS;CorrectionFunctions\\AbsorptionCorrections"

    def summary(self):
        return "Calculates absorption corrections for a flat plate sample using Paalman & Pings format."

    # ------------------------------------------------------------------------------

    def PyInit(self):
        ws_validator = InstrumentValidator()

        self.declareProperty(
            MatrixWorkspaceProperty("SampleWorkspace", "", direction=Direction.Input, validator=ws_validator),
            doc="Name for the input sample workspace",
        )

        self.declareProperty(name="SampleChemicalFormula", defaultValue="", doc="Sample chemical formula")

        self.declareProperty(
            name="SampleCoherentXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The coherent cross-section for the sample material in barns. To be used instead of Chemical Formula.",
        )

        self.declareProperty(
            name="SampleIncoherentXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The incoherent cross-section for the sample material in barns. To be used instead of Chemical Formula.",
        )

        self.declareProperty(
            name="SampleAttenuationXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The absorption cross-section for the sample material in barns. To be used instead of Chemical Formula.",
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

        self.declareProperty(name="SampleThickness", defaultValue=0.0, validator=FloatBoundedValidator(0.0), doc="Sample thickness in cm")

        self.declareProperty(name="SampleAngle", defaultValue=0.0, doc="Angle between incident beam and normal to flat plate surface")

        self.declareProperty(
            MatrixWorkspaceProperty("CanWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional, validator=ws_validator),
            doc="Name for the input container workspace",
        )

        self.declareProperty(name="CanChemicalFormula", defaultValue="", doc="Container chemical formula")

        self.declareProperty(
            name="CanCoherentXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The coherent cross-section for the can material in barns. To be used instead of Chemical Formula.",
        )

        self.declareProperty(
            name="CanIncoherentXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The incoherent cross-section for the can material in barns. To be used instead of Chemical Formula.",
        )

        self.declareProperty(
            name="CanAttenuationXSection",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="The absorption cross-section for the can material in barns. To be used instead of Chemical Formula.",
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

        self.declareProperty(
            name="CanFrontThickness", defaultValue=0.0, validator=FloatBoundedValidator(0.0), doc="Container front thickness in cm"
        )

        self.declareProperty(
            name="CanBackThickness", defaultValue=0.0, validator=FloatBoundedValidator(0.0), doc="Container back thickness in cm"
        )

        self.declareProperty(
            name="NumberWavelengths", defaultValue=10, validator=IntBoundedValidator(1), doc="Number of wavelengths for calculation"
        )

        self.declareProperty(
            name="Interpolate", defaultValue=True, doc="Interpolate the correction workspaces to match the sample workspace"
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
            "Specify manually to override. This is used only in Efixed energy transfer mode.",
        )

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output), doc="The output corrections workspace group"
        )

    # ------------------------------------------------------------------------------

    def validateInputs(self):
        issues = dict()

        sample_ws = self.getProperty("SampleWorkspace").value
        can_ws_name = self.getPropertyValue("CanWorkspace")
        use_can = can_ws_name != ""

        # Ensure that a can chemical formula is given when using a can workspace
        if use_can:
            can_chemical_formula = self.getPropertyValue("CanChemicalFormula")
            can_coherent_cross_section = self.getProperty("CanCoherentXSection").value
            can_incoherent_cross_section = self.getProperty("CanIncoherentXSection").value
            can_attenuation_cross_section = self.getProperty("CanAttenuationXSection").value
            if (
                can_chemical_formula == ""
                and can_coherent_cross_section == 0.0
                and can_incoherent_cross_section == 0.0
                and can_attenuation_cross_section == 0.0
            ):
                issues["CanChemicalFormula"] = "Must provide a chemical formula or cross sections when providing a can workspace."

        self._emode = self.getPropertyValue("Emode")
        self._efixed = self.getProperty("Efixed").value

        if self._emode != "Efixed":
            if not isinstance(sample_ws, MatrixWorkspace):
                issues["SampleWorkspace"] = "The SampleWorkspace must be a MatrixWorkspace."
            elif sample_ws.getAxis(0).getUnit().unitID() != "Wavelength":
                # require both sample and can ws have wavelenght as x-axis
                issues["SampleWorkspace"] = "The SampleWorkspace must have units of wavelength."

            if use_can:
                if not isinstance(mtd[can_ws_name], MatrixWorkspace):
                    issues["CanWorkspace"] = "The CanWorkspace must be a MatrixWorkspace."
                elif mtd[can_ws_name].getAxis(0).getUnit().unitID() != "Wavelength":
                    issues["CanWorkspace"] = "The CanWorkspace must have units of wavelength."

        return issues

    # ------------------------------------------------------------------------------

    def PyExec(self):
        self._setup()
        self._wave_range()

        setup_prog = Progress(self, start=0.0, end=0.2, nreports=2)
        # Set sample material form chemical formula
        setup_prog.report("Set sample material")
        self._sample_density = self._set_material(
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

        # If using a can, set sample material using chemical formula
        if self._use_can:
            setup_prog.report("Set container sample material")
            self._can_density = self._set_material(
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

        # Holders for the corrected data
        data_ass = []
        data_assc = []
        data_acsc = []
        data_acc = []

        self._get_angles()
        num_angles = len(self._angles)
        workflow_prog = Progress(self, start=0.2, end=0.8, nreports=num_angles * 2)

        # Check sample input
        sam_material = mtd[self._sample_ws_name].sample().getMaterial()
        self._has_sample_in = bool(
            self._sample_density and self._sample_thickness and (sam_material.totalScatterXSection() + sam_material.absorbXSection())
        )
        if not self._has_sample_in:
            logger.warning(
                "The sample has not been given, or the information is incomplete. Continuing but no absorption for sample will be computed."
            )

        # Check can input
        if self._use_can:
            can_material = mtd[self._can_ws_name].sample().getMaterial()
            if self._can_density and (can_material.totalScatterXSection() + can_material.absorbXSection()):
                self._has_can_front_in = bool(self._can_front_thickness)
                self._has_can_back_in = bool(self._can_back_thickness)
            else:
                logger.warning(
                    "A can workspace was given but the can information is incomplete. Continuing but no absorption for the can will "
                    "be computed."
                )

            if not self._has_can_front_in:
                logger.warning(
                    "A can workspace was given but the can front thickness was not given. Continuing but no absorption for can front"
                    " will be computed."
                )
            if not self._has_can_back_in:
                logger.warning(
                    "A can workspace was given but the can back thickness was not given. Continuing but no absorption for can back"
                    " will be computed."
                )

        for angle_idx in range(num_angles):
            workflow_prog.report("Running flat correction for angle %s" % angle_idx)
            angle = self._angles[angle_idx]
            (ass, assc, acsc, acc) = self._flat_abs(angle)

            logger.information("Angle %d: %f successful" % (angle_idx + 1, self._angles[angle_idx]))
            workflow_prog.report("Appending data for angle %s" % angle_idx)
            data_ass = np.append(data_ass, ass)
            data_assc = np.append(data_assc, assc)
            data_acsc = np.append(data_acsc, acsc)
            data_acc = np.append(data_acc, acc)

        log_prog = Progress(self, start=0.8, end=1.0, nreports=8)

        sample_logs = {
            "sample_shape": "flatplate",
            "sample_filename": self._sample_ws_name,
            "sample_thickness": self._sample_thickness,
            "sample_angle": self._sample_angle,
            "emode": self._emode,
            "efixed": self._efixed,
        }
        dataX = self._wavelengths * num_angles

        # Create the output workspaces
        ass_ws = self._output_ws_name + "_ass"
        log_prog.report("Creating ass output Workspace")
        CreateWorkspace(
            OutputWorkspace=ass_ws,
            DataX=dataX,
            DataY=data_ass,
            NSpec=num_angles,
            UnitX="Wavelength",
            VerticalAxisUnit="SpectraNumber",
            ParentWorkspace=self._sample_ws_name,
            EnableLogging=False,
        )
        log_prog.report("Adding sample logs")
        self._add_sample_logs(ass_ws, sample_logs)

        workspaces = [ass_ws]

        if self._use_can:
            log_prog.report("Adding can sample logs")
            AddSampleLog(Workspace=ass_ws, LogName="can_filename", LogType="String", LogText=str(self._can_ws_name), EnableLogging=False)

            assc_ws = self._output_ws_name + "_assc"
            workspaces.append(assc_ws)
            log_prog.report("Creating assc output workspace")
            CreateWorkspace(
                OutputWorkspace=assc_ws,
                DataX=dataX,
                DataY=data_assc,
                NSpec=num_angles,
                UnitX="Wavelength",
                VerticalAxisUnit="SpectraNumber",
                ParentWorkspace=self._sample_ws_name,
                EnableLogging=False,
            )
            log_prog.report("Adding assc sample logs")
            self._add_sample_logs(assc_ws, sample_logs)
            AddSampleLog(Workspace=assc_ws, LogName="can_filename", LogType="String", LogText=str(self._can_ws_name), EnableLogging=False)

            acsc_ws = self._output_ws_name + "_acsc"
            workspaces.append(acsc_ws)
            log_prog.report("Creating acsc outputworkspace")
            CreateWorkspace(
                OutputWorkspace=acsc_ws,
                DataX=dataX,
                DataY=data_acsc,
                NSpec=num_angles,
                UnitX="Wavelength",
                VerticalAxisUnit="SpectraNumber",
                ParentWorkspace=self._sample_ws_name,
                EnableLogging=False,
            )
            log_prog.report("Adding acsc sample logs")
            self._add_sample_logs(acsc_ws, sample_logs)
            AddSampleLog(Workspace=acsc_ws, LogName="can_filename", LogType="String", LogText=str(self._can_ws_name), EnableLogging=False)

            acc_ws = self._output_ws_name + "_acc"
            workspaces.append(acc_ws)
            log_prog.report("Creating acc workspace")
            CreateWorkspace(
                OutputWorkspace=acc_ws,
                DataX=dataX,
                DataY=data_acc,
                NSpec=num_angles,
                UnitX="Wavelength",
                VerticalAxisUnit="SpectraNumber",
                ParentWorkspace=self._sample_ws_name,
                EnableLogging=False,
            )
            log_prog.report("Adding acc sample logs")
            self._add_sample_logs(acc_ws, sample_logs)
            AddSampleLog(Workspace=acc_ws, LogName="can_filename", LogType="String", LogText=str(self._can_ws_name), EnableLogging=False)

        if self._interpolate:
            self._interpolate_corrections(workspaces)
        log_prog.report("Grouping Output Workspaces")
        GroupWorkspaces(InputWorkspaces=",".join(workspaces), OutputWorkspace=self._output_ws_name, EnableLogging=False)
        self.setPropertyValue("OutputWorkspace", self._output_ws_name)

    # ------------------------------------------------------------------------------

    def _setup(self):
        self._sample_ws_name = self.getPropertyValue("SampleWorkspace")
        self._sample_chemical_formula = self.getPropertyValue("SampleChemicalFormula")
        self._sample_coherent_cross_section = self.getPropertyValue("SampleCoherentXSection")
        self._sample_incoherent_cross_section = self.getPropertyValue("SampleIncoherentXSection")
        self._sample_attenuation_cross_section = self.getPropertyValue("SampleAttenuationXSection")
        self._sample_density_type = self.getPropertyValue("SampleDensityType")
        self._sample_number_density_unit = self.getPropertyValue("SampleNumberDensityUnit")
        self._sample_density = self.getProperty("SampleDensity").value
        self._sample_thickness = self.getProperty("SampleThickness").value
        self._sample_angle = self.getProperty("SampleAngle").value

        self._can_ws_name = self.getPropertyValue("CanWorkspace")
        self._use_can = self._can_ws_name != ""

        self._can_chemical_formula = self.getPropertyValue("CanChemicalFormula")
        self._can_coherent_cross_section = self.getPropertyValue("CanCoherentXSection")
        self._can_incoherent_cross_section = self.getPropertyValue("CanIncoherentXSection")
        self._can_attenuation_cross_section = self.getPropertyValue("CanAttenuationXSection")
        self._can_density_type = self.getPropertyValue("CanDensityType")
        self._can_number_density_unit = self.getPropertyValue("CanNumberDensityUnit")
        self._can_density = self.getProperty("CanDensity").value
        self._can_front_thickness = self.getProperty("CanFrontThickness").value
        self._can_back_thickness = self.getProperty("CanBackThickness").value

        self._number_wavelengths = self.getProperty("NumberWavelengths").value
        self._interpolate = self.getProperty("Interpolate").value

        self._emode = self.getPropertyValue("Emode")
        self._efixed = self.getProperty("Efixed").value

        if (self._emode == "Efixed" or self._emode == "Direct" or self._emode == "Indirect") and self._efixed == 0.0:
            # Efixed mode requested with default efixed, try to read from Instrument Parameters
            try:
                self._efixed = self._getEfixed()
                logger.information("Found Efixed = {0}".format(self._efixed))
            except ValueError:
                raise RuntimeError(
                    "Efixed, Direct or Indirect mode requested with the default value,"
                    "but could not find the Efixed parameter in the instrument."
                )

        if self._emode == "Efixed":
            logger.information("No interpolation is possible in Efixed mode.")
            self._interpolate = False

        self._set_sample_method = "Chemical Formula" if self._sample_chemical_formula != "" else "Cross Sections"
        self._set_can_method = "Chemical Formula" if self._can_chemical_formula != "" else "Cross Sections"

        self._output_ws_name = self.getPropertyValue("OutputWorkspace")

        # purge the lists
        self._angles = list()
        self._wavelengths = list()

    # ------------------------------------------------------------------------------

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
        return ws.sample().getMaterial().numberDensity

    # ------------------------------------------------------------------------------

    def _get_angles(self):
        num_hist = mtd[self._sample_ws_name].getNumberHistograms()
        source_pos = mtd[self._sample_ws_name].getInstrument().getSource().getPos()
        sample_pos = mtd[self._sample_ws_name].getInstrument().getSample().getPos()
        beam_pos = sample_pos - source_pos
        self._angles = list()
        for index in range(0, num_hist):
            detector = mtd[self._sample_ws_name].getDetector(index)
            two_theta = detector.getTwoTheta(sample_pos, beam_pos) / self.PICONV  # calc angle
            self._angles.append(two_theta)

    # ------------------------------------------------------------------------------

    def _wave_range(self):
        if self._emode == "Efixed":
            lambda_fixed = math.sqrt(81.787 / self._efixed)
            self._wavelengths.append(lambda_fixed)
            logger.information("Efixed mode, setting lambda_fixed to {0}".format(lambda_fixed))
        else:
            wave_range = "__WaveRange"
            ExtractSingleSpectrum(InputWorkspace=self._sample_ws_name, OutputWorkspace=wave_range, WorkspaceIndex=0)
            Xin = mtd[wave_range].readX(0)
            wave_min = mtd[wave_range].readX(0)[0]
            wave_max = mtd[wave_range].readX(0)[len(Xin) - 1]
            number_waves = self._number_wavelengths
            wave_bin = (wave_max - wave_min) / (number_waves - 1)

            self._wavelengths = list()
            for idx in range(0, number_waves):
                self._wavelengths.append(wave_min + idx * wave_bin)

            DeleteWorkspace(wave_range, EnableLogging=False)

    # ------------------------------------------------------------------------------

    def _getEfixed(self):
        return_eFixed = 0.0
        inst = mtd[self._sample_ws_name].getInstrument()

        if inst.hasParameter("Efixed"):
            return_eFixed = inst.getNumberParameter("EFixed")[0]
        elif inst.hasParameter("analyser"):
            analyser_name = inst.getStringParameter("analyser")[0]
            analyser_comp = inst.getComponentByName(analyser_name)

            if analyser_comp is not None and analyser_comp.hasParameter("Efixed"):
                return_eFixed = analyser_comp.getNumberParameter("EFixed")[0]

        if return_eFixed > 0:
            return return_eFixed
        else:
            raise ValueError("No non-zero Efixed parameter found")

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

    def _add_sample_logs(self, ws, sample_logs):
        """
        Add a dictionary of logs to a workspace.

        The type of the log is inferred by the type of the value passed to the log.

        @param ws Workspace to add logs too.
        @param sample_logs Dictionary of logs to append to the workspace.
        """

        for key, value in sample_logs.items():
            if isinstance(value, bool):
                log_type = "String"
            elif isinstance(value, (int, float)):
                log_type = "Number"
            else:
                log_type = "String"

            AddSampleLog(Workspace=ws, LogName=key, LogType=log_type, LogText=str(value), EnableLogging=False)

    # ------------------------------------------------------------------------------

    def _flat_abs(self, angle):
        """
        FlatAbs - calculate flat plate absorption factors

        For more information See:
          - MODES User Guide: http://www.isis.stfc.ac.uk/instruments/iris/data-analysis/modes-v3-user-guide-6962.pdf
          - C J Carlile, Rutherford Laboratory report, RL-74-103 (1974)

        The current implementation is based on:
          - J. Wuttke: 'Absorption-Correction Factors for Scattering from Flat or Tubular Samples:
            Open-Source Implementation libabsco, and Why it Should be Used with Caution',
            http://apps.jcns.fz-juelich.de/doku/sc/_media/abs00.pdf

        @return: A tuple containing the attenuations;
            1) scattering and absorption in sample,
            2) scattering in sample and absorption in sample and container
            3) scattering in container and absorption in sample and container,
            4) scattering and absorption in container.
        """

        # self._sample_angle is the normal to the sample surface, i.e.
        # self._sample_angle = 0 means that the sample is perpendicular
        # to the incident beam
        alpha = (90.0 + self._sample_angle) * self.PICONV
        theta = angle * self.PICONV
        salpha = np.sin(alpha)
        if theta > (alpha + np.pi):
            stha = np.sin(abs(theta - alpha - np.pi))
        else:
            stha = np.sin(abs(theta - alpha))

        nlam = len(self._wavelengths)

        ass = np.ones(nlam)
        assc = np.ones(nlam)
        acsc = np.ones(nlam)
        acc = np.ones(nlam)

        # Scattering in direction of slab --> calculation is not reliable
        # Default to 1 for everything
        # Tolerance is 0.001 rad ~ 0.06 deg
        if abs(theta - alpha) < 0.001:
            return ass, assc, acsc, acc

        sample = mtd[self._sample_ws_name].sample()
        sam_material = sample.getMaterial()

        # List of wavelengths
        waveslengths = np.array(self._wavelengths)

        sst = np.vectorize(self._self_shielding_transmission)
        ssr = np.vectorize(self._self_shielding_reflection)

        ki_s, kf_s = 0, 0
        if self._has_sample_in:
            ki_s, kf_s, ass = self._sample_cross_section_calc(sam_material, waveslengths, theta, alpha, stha, salpha, sst, ssr)

        # Container --> Acc, Assc, Acsc
        if self._use_can:
            ass, assc, acsc, acc = self._can_cross_section_calc(waveslengths, theta, alpha, stha, salpha, ki_s, kf_s, ass, acc, sst, ssr)

        return ass, assc, acsc, acc

    # ------------------------------------------------------------------------------

    def _sample_cross_section_calc(self, sam_material, waves, theta, alpha, stha, salpha, sst, ssr):
        # Sample cross section (value for each of the wavelengths and for E = Efixed)
        sample_x_section = (
            sam_material.totalScatterXSection() + sam_material.absorbXSection() * waves / self.TABULATED_WAVELENGTH
        ) * self._sample_density

        if self._efixed > 0:
            sample_x_section_efixed = (
                sam_material.totalScatterXSection() + sam_material.absorbXSection() * np.sqrt(self.TABULATED_ENERGY / self._efixed)
            ) * self._sample_density
        elif self._emode == "Elastic":
            sample_x_section_efixed = 0

        # Sample --> Ass
        if self._emode == "Efixed":
            ki_s = sample_x_section_efixed * self._sample_thickness / salpha
            kf_s = sample_x_section_efixed * self._sample_thickness / stha
        else:
            ki_s, kf_s = self._calc_ki_kf(waves, self._sample_thickness, salpha, stha, sample_x_section, sample_x_section_efixed)

        if theta < alpha or theta > (alpha + np.pi):
            # transmission case
            ass = sst(ki_s, kf_s)
        else:
            # reflection case
            ass = ssr(ki_s, kf_s)

        return ki_s, kf_s, ass

    # ------------------------------------------------------------------------------

    def _can_cross_section_calc(self, wavelengths, theta, alpha, stha, salpha, ki_s, kf_s, ass, acc, sst, ssr):
        can_sample = mtd[self._can_ws_name].sample()
        can_material = can_sample.getMaterial()

        if self._has_can_front_in or self._has_can_back_in:
            # Calculate can cross section (value for each of the wavelengths and for E = Efixed)
            can_x_section = (
                can_material.totalScatterXSection() + can_material.absorbXSection() * wavelengths / self.TABULATED_WAVELENGTH
            ) * self._can_density

            if self._efixed > 0:
                can_x_section_efixed = (
                    can_material.totalScatterXSection() + can_material.absorbXSection() * np.sqrt(self.TABULATED_ENERGY / self._efixed)
                ) * self._can_density
            elif self._emode == "Elastic":
                can_x_section_efixed = 0

        ki_c1, kf_c1, ki_c2, kf_c2 = 0, 0, 0, 0
        acc1, acc2 = np.ones(len(self._wavelengths)), np.ones(len(self._wavelengths))
        if self._has_can_front_in:
            # Front container --> Acc1
            ki_c1, kf_c1, acc1 = self._can_thickness_calc(
                can_x_section, can_x_section_efixed, self._can_front_thickness, wavelengths, theta, alpha, stha, salpha, ssr, sst
            )
        if self._has_can_back_in:
            # Back container --> Acc2
            ki_c2, kf_c2, acc2 = self._can_thickness_calc(
                can_x_section, can_x_section_efixed, self._can_back_thickness, wavelengths, theta, alpha, stha, salpha, ssr, sst
            )

        # Attenuation due to passage by other layers (sample or container)
        if theta < alpha or theta > (alpha + np.pi):  # transmission case
            assc, acsc, acc = self._container_transmission_calc(acc, acc1, acc2, ki_s, kf_s, ki_c1, kf_c2, ass)
        else:  # reflection case
            assc, acsc, acc = self._container_reflection_calc(acc, acc1, acc2, ki_s, kf_s, ki_c1, kf_c1, ass)

        return ass, assc, acsc, acc

    # ------------------------------------------------------------------------------

    def _can_thickness_calc(self, can_x_section, can_x_section_efixed, can_thickness, wavelengths, theta, alpha, stha, salpha, ssr, sst):
        if self._emode == "Efixed":
            ki = can_x_section_efixed * can_thickness / salpha
            kf = can_x_section_efixed * can_thickness / stha
        else:
            ki, kf = self._calc_ki_kf(wavelengths, can_thickness, salpha, stha, can_x_section, can_x_section_efixed)

        if theta < alpha or theta > (alpha + np.pi):
            # transmission case
            acc = sst(ki, kf)
        else:
            # reflection case
            acc = ssr(ki, kf)

        return ki, kf, acc

    # ------------------------------------------------------------------------------

    def _container_transmission_calc(self, acc, acc1, acc2, ki_s, kf_s, ki_c1, kf_c2, ass):
        if self._has_can_front_in and self._has_can_back_in:
            acc = (self._can_front_thickness * acc1 * np.exp(-kf_c2) + self._can_back_thickness * acc2 * np.exp(-ki_c1)) / (
                self._can_front_thickness + self._can_back_thickness
            )
            if self._has_sample_in:
                acsc = (
                    self._can_front_thickness * acc1 * np.exp(-kf_s - kf_c2) + self._can_back_thickness * acc2 * np.exp(-ki_c1 - ki_s)
                ) / (self._can_front_thickness + self._can_back_thickness)
            else:
                acsc = acc
            assc = ass * np.exp(-ki_c1 - kf_c2)
        elif self._has_can_front_in:
            acc = acc1
            if self._has_sample_in:
                acsc = acc1 * np.exp(-kf_s)
            else:
                acsc = acc
            assc = ass * np.exp(-ki_c1)
        elif self._has_can_back_in:
            acc = acc2
            if self._has_sample_in:
                acsc = acc2 * np.exp(-ki_s)
            else:
                acsc = acc
            assc = ass * np.exp(-kf_c2)
        else:
            if self._has_sample_in:
                acsc = 0.5 * np.exp(-kf_s) + 0.5 * np.exp(-ki_s)
            else:
                acsc = acc
            assc = ass

        return assc, acsc, acc

    # ------------------------------------------------------------------------------

    def _container_reflection_calc(self, acc, acc1, acc2, ki_s, kf_s, ki_c1, kf_c1, ass):
        if self._has_can_front_in and self._has_can_back_in:
            acc = (self._can_front_thickness * acc1 + self._can_back_thickness * acc2 * np.exp(-ki_c1 - kf_c1)) / (
                self._can_front_thickness + self._can_back_thickness
            )
            if self._has_sample_in:
                acsc = (self._can_front_thickness * acc1 + self._can_back_thickness * acc2 * np.exp(-ki_c1 - ki_s - kf_s - kf_c1)) / (
                    self._can_front_thickness + self._can_back_thickness
                )
            else:
                acsc = acc
            assc = ass * np.exp(-ki_c1 - kf_c1)
        elif self._has_can_front_in:
            acc = acc1
            if self._has_sample_in:
                acsc = acc1
            else:
                acsc = acc
            assc = ass * np.exp(-ki_c1 - kf_c1)
        elif self._has_can_back_in:
            acc = acc2
            if self._has_sample_in:
                acsc = acc2 * np.exp(-ki_s - kf_s)
            else:
                acsc = acc
            assc = ass * np.exp(-ki_c1 - kf_c1)
        else:
            if self._has_sample_in:
                acsc = 0.5 + 0.5 * np.exp(-ki_s - kf_s)
            else:
                acsc = acc
            assc = ass

        return assc, acsc, acc

    # ------------------------------------------------------------------------------

    def _self_shielding_transmission(self, ki, kf):
        if abs(ki - kf) < 1.0e-3:
            return np.exp(-ki) * (1.0 - 0.5 * (kf - ki) + (kf - ki) ** 2 / 12.0)
        else:
            return (np.exp(-kf) - np.exp(-ki)) / (ki - kf)

    # ------------------------------------------------------------------------------

    def _self_shielding_reflection(self, ki, kf):
        return (1.0 - np.exp(-ki - kf)) / (ki + kf)

    # ------------------------------------------------------------------------------

    def _calc_ki_kf(self, waves, thickness, sinangle1, sinangle2, x_section, x_section_efixed=0):
        ki = np.ones(waves.size)
        kf = np.ones(waves.size)
        if self._emode == "Elastic":
            ki = np.copy(x_section)
            kf = np.copy(x_section)
        elif self._emode == "Direct":
            ki *= x_section_efixed
            kf = np.copy(x_section)
        elif self._emode == "Indirect":
            ki = np.copy(x_section)
            kf *= x_section_efixed
        ki *= thickness / sinangle1
        kf *= thickness / sinangle2
        return ki, kf

    # ------------------------------------------------------------------------------


# Register algorithm with Mantid
AlgorithmFactory.subscribe(FlatPlatePaalmanPingsCorrection)
