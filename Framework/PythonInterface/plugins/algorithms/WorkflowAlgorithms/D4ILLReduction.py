# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import (
    AlgorithmFactory,
    FileAction,
    FileProperty,
    MultipleFileProperty,
    Progress,
    PythonAlgorithm,
    WorkspaceGroup,
    WorkspaceGroupProperty,
)
from mantid.kernel import DeltaEModeType, Direction, FloatBoundedValidator, StringListValidator, UnitConversion
from mantid.simpleapi import (
    ConvertAxisByFormula,
    CloneWorkspace,
    CreateSingleValuedWorkspace,
    CreateWorkspace,
    DeadTimeCorrection,
    DeleteWorkspace,
    DeleteWorkspaces,
    Divide,
    ExtractMonitors,
    GroupWorkspaces,
    LoadAndMerge,
    MaskDetectorsIf,
    MoveInstrumentComponent,
    mtd,
    Multiply,
    RenameWorkspace,
    SaveAscii,
    SumOverlappingTubes,
)
import numpy as np


class D4ILLReduction(PythonAlgorithm):
    def category(self):
        return "ILL\\Diffraction"

    def summary(self):
        return "Performs diffraction data reduction for the D4 instrument at the ILL."

    def seeAlso(self):
        return ["LoadILLDiffraction"]

    def name(self):
        return "D4ILLReduction"

    def validateInputs(self):
        issues = dict()

        return issues

    def PyInit(self):
        self.declareProperty(MultipleFileProperty("Run", extensions=["nxs"]), doc="File path of run(s).")

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="The output workspace based on the value of ProcessAs.",
        )

        self.declareProperty(
            FileProperty("BankPositionOffsetsFile", "", action=FileAction.OptionalLoad),
            doc="The path to the file with bank position offsets.",
        )

        self.declareProperty(name="ZeroPositionAngle", defaultValue=0.0, doc="The angular position correction common to all banks.")

        self.declareProperty(
            FileProperty("EfficiencyCalibrationFile", "", action=FileAction.OptionalLoad),
            doc="The path to the file with relative detector efficiencies.",
        )

        self.declareProperty(name="DeadTimeTauDetector", defaultValue=7e-6, doc="The count rate coefficient for detectors.")

        self.declareProperty(name="DeadTimeTauMonitor", defaultValue=2.4e-6, doc="The count rate coefficient for the monitor.")

        self.declareProperty(
            name="Wavelength",
            defaultValue=0.5,
            doc="The measurement wavelength, in Angstrom. Will be used if not default or if Ei in metadata is 0.",
        )

        self.declareProperty(
            name="NormaliseBy",
            defaultValue="Monitor",
            validator=StringListValidator(["Monitor", "Time", "None"]),
            direction=Direction.Input,
            doc="What normalisation approach to use on data.",
        )

        self.declareProperty(
            name="NormalisationStandard",
            defaultValue=1.0e6,
            doc="Standard value against which the normalisation which be performed. The default is for normalisation to monitor.",
        )

        self.declareProperty(
            name="ScatteringAngleBinSize",
            defaultValue=0.5,
            validator=FloatBoundedValidator(lower=0),
            direction=Direction.Input,
            doc="Scattering angle bin size in degrees used for expressing scan data on a single TwoTheta axis.",
        )

        self.declareProperty("ExportASCII", True, doc="Whether or not to export the output workspaces as ASCII files.")

        self.declareProperty("ClearCache", True, doc="Whether or not to clear the cache of intermediate workspaces.")

        self.declareProperty(
            "DebugMode", defaultValue=False, doc="Whether to create and show all intermediate workspaces at each correction step."
        )

    def _correct_bank_positions(self, ws):
        """
        Corrects bank positions by rotating them around the sample by the defined zero angle and values available from
        an ASCII calibration file.

        Args:
        ws: (str) name of the workspace to be corrected
        return: (str) corrected workspace name
        """
        zero_angle_corr = self.getProperty("ZeroPositionAngle").value
        calibration_file = self.getPropertyValue("BankPositionOffsetsFile")
        n_banks = (
            mtd[ws][0].getInstrument().getIntParameter("number_banks")[0]
            if isinstance(mtd[ws], WorkspaceGroup)
            else mtd[ws].getInstrument().getIntParameter("number_banks")[0]
        )
        bank_shifts = self._get_shifts(calibration_file, zero_angle_corr, n_banks)
        if self.getProperty("DebugMode").value:
            ws_old = ws
            ws = "{}_bank_pos_corrected".format(ws_old)
            CloneWorkspace(InputWorkspace=ws_old, OutputWorkspace=ws)
        if isinstance(mtd[ws], WorkspaceGroup):
            for entry in mtd[ws]:
                self._rotate_banks(entry.name(), bank_shifts)
        else:
            self._rotate_banks(ws, bank_shifts)
        return ws

    def _create_dead_time_correction(self, ws, tau):
        """
        Creates a workspace containing multiplicative dead-time correction.

        Args:
        ws: (str) workspace to be corrected
        tau: (float) count rate coefficient
        return: (str) corrected workspace name
        """
        time_ws = "time_ws"
        time_val = mtd[ws].run().getLogData("duration").value
        CreateSingleValuedWorkspace(DataValue=time_val, OutputWorkspace=time_ws)
        tmp_ws = "tmp"
        Divide(LHSWorkspace=ws, RHSWorkspace=time_ws, OutputWorkspace=tmp_ws)
        corr_ws = "correction_ws"
        # DeadTimeCorrection algorithm corrects workspace containing rates, while D4 data contains counts and we want
        # to keep it that way, therefore we create a temporary workspace with rates and extract the correction for later
        # use
        DeadTimeCorrection(InputWorkspace=tmp_ws, Tau=tau, OutputWorkspace=corr_ws)
        # output of the final divide contains the multiplicative dead-time correction
        Divide(LHSWorkspace=corr_ws, RHSWorkspace=tmp_ws, OutputWorkspace=corr_ws)
        DeleteWorkspaces(WorkspaceList=[tmp_ws, time_ws])
        return corr_ws

    def _correct_dead_time(self, ws, tau):
        """
        Corrects for the dead-time of detectors or a monitor.

        Args:
        ws: (str) workspace (or group) name to be corrected
        tau: (float) count rate coefficient
        return: (str) output workspace name
        """
        if isinstance(mtd[ws], WorkspaceGroup):
            corrected_list = []
            for entry in mtd[ws]:
                corrected_ws = self._correct_dead_time(entry.name(), tau)
                corrected_list.append(corrected_ws)
            corrected_ws = "{}_dt_corrected".format(ws)
            GroupWorkspaces(InputWorkspaces=corrected_list, OutputWorkspace=corrected_ws)
            if self.getProperty("ClearCache").value:
                DeleteWorkspace(Workspace=ws)
            return corrected_ws
        correction_ws = self._create_dead_time_correction(ws, tau)
        corrected_ws = "{}_dt_corrected".format(ws)
        Multiply(LHSWorkspace=ws, RHSWorkspace=correction_ws, OutputWorkspace=corrected_ws)
        if self.getProperty("ClearCache").value:
            DeleteWorkspace(Workspace=correction_ws)
        return corrected_ws

    def _create_efficiency_correction(self, efficiency_file_name):
        """
        Creates a workspace with efficiency factors for each detector.

        Args:
        efficiency_file_name: (str) path to the file containing efficiency correction factors for each detector
        return: (str) name of the workspace containing efficiency correction
        """
        y_values = []
        det_no = 0
        try:
            with open(efficiency_file_name, "r") as efficiency:
                for line in efficiency:
                    if "#" in line:
                        continue
                    line = line.strip(" ")
                    columns = line.split()
                    if columns:
                        corr_val = float(columns[2])
                        y_values.append(corr_val if corr_val > 0 else np.nan)
                        det_no += 1
        except (RuntimeError, IndexError, FileNotFoundError) as e:
            self.log().warning(str(e))
            self.log().warning("The efficiency correction file does not exist or is faulty.")
            return
        eff_corr_ws = "eff_corr_ws"
        x_values = np.array(0)
        CreateWorkspace(DataX=x_values, DataY=np.array(y_values), NSpec=len(y_values), OutputWorkspace=eff_corr_ws)
        return eff_corr_ws

    def _correct_relative_efficiency(self, ws):
        """
        Corrects the relative efficiency of each detector using values provided in an external ASCII file.

        Args:
        ws: (str) name of the workspace to be corrected
        return: (str) name of corrected workspace
        """
        if self.getProperty("EfficiencyCalibrationFile").isDefault:
            return ws
        calibration_file = self.getPropertyValue("EfficiencyCalibrationFile")
        eff_corr_ws = self._create_efficiency_correction(calibration_file)
        if eff_corr_ws is None:  # provided efficiency correction file was faulty
            return ws

        output_ws = "{}_efficiency_corrected".format(ws)
        Divide(LHSWorkspace=ws, RHSWorkspace=eff_corr_ws, OutputWorkspace=output_ws)
        MaskDetectorsIf(InputWorkspace=output_ws, OutputWorkspace=output_ws, Operator="NotFinite")
        if self.getProperty("ClearCache").value:
            DeleteWorkspaces(WorkspaceList=[eff_corr_ws, ws])
        return output_ws

    def _convert_to_q(self, ws, output_ws):
        """
        Converts the bin axis values of the  provided workspace from scattering angle to momentum exchange.

        Args:
        ws: (str) name of the workspace with x-axis in 2theta units
        return: (str) name of the workspace converted to q
        """
        Ei = mtd[ws].getRun().getLogData("Ei").value
        if self.getProperty("Wavelength").isDefault and Ei != 0:
            wavelength = UnitConversion.run("Energy", "Wavelength", Ei, 0, 0, 0, DeltaEModeType.Elastic, 0.0)
        else:
            wavelength = self.getProperty("Wavelength").value
        ConvertAxisByFormula(
            InputWorkspace=ws,
            OutputWorkspace=output_ws,
            Formula="4*{0}*sin(x*{0}/360.0) / {1}".format(np.pi, wavelength),
            Axis="X",
            AxisUnits="MomentumTransfer",
        )
        return output_ws

    def _create_diffractograms(self, ws):
        """
        Creates diffractograms of reduced data as a function of scattering angle and momentum exchange.

        Args:
        ws: (str) input workspace name to be put on common 2theta and q axis
        return: (str) workspace group name containing diffractograms in 2theta and q
        """
        diff_2theta_ws = "{}_diffractogram_2theta".format(ws)
        SumOverlappingTubes(
            InputWorkspaces=ws,
            OutputWorkspace=diff_2theta_ws,
            OutputType="1D",
            ScatteringAngleBinning=self.getProperty("ScatteringAngleBinSize").value,
            HeightAxis="-10,10",  # in D4C case, this is arbitrarily large to contain all detectors
        )
        diff_q_ws = "{}_diffractogram_q".format(ws)
        self._convert_to_q(diff_2theta_ws, output_ws=diff_q_ws)
        output_name = self.getPropertyValue("OutputWorkspace")
        GroupWorkspaces(InputWorkspaces=[diff_2theta_ws, diff_q_ws], OutputWorkspace=output_name)
        if self.getProperty("ClearCache").value:
            DeleteWorkspace(Workspace=ws)
        return output_name

    def _separate_detectors_monitors(self, ws):
        """
        Separates the input workspace into groups containing monitor workspaces and detector workspaces.

        Args:
        ws: (str) name of the input workspace
        return: (tuple) two names of workspace groups containing detector, and monitor workspaces
        """
        mon = "{}_mon".format(ws)
        mon_list = []
        det_list = []
        if isinstance(mtd[ws], WorkspaceGroup):
            for entry in mtd[ws]:
                det_ws = entry.name()
                det_list.append(det_ws)
                mon_ws = "{}_mon".format(det_ws)
                mon_list.append(mon_ws)
                ExtractMonitors(InputWorkspace=entry, DetectorWorkspace=det_ws, MonitorWorkspace=mon_ws)
        else:
            det_ws = "{}_1".format(ws)
            det_list.append(det_ws)
            mon_ws = "{}_mon".format(det_ws)
            mon_list.append(mon_ws)
            ExtractMonitors(InputWorkspace=ws, DetectorWorkspace=det_ws, MonitorWorkspace=mon_ws)
        GroupWorkspaces(InputWorkspaces=mon_list, OutputWorkspace=mon)
        GroupWorkspaces(InputWorkspaces=det_list, OutputWorkspace=ws)
        return ws, mon

    def _finalize(self, ws):
        """Finalizes the reduction step by setting unique names to the output workspaces."""
        output_ws_name = self.getPropertyValue("OutputWorkspace")
        for entry in mtd[ws]:
            # the output is going to contain the output workspace name as prefix, 'diffractogram' string as the middle,
            # and the suffix will indicate the X-axis unit of the workspace: either 2theta or q
            output_name = "{}_diffractogram_{}".format(output_ws_name, (entry.name())[(entry.name()).rfind("_") + 1 :])
            RenameWorkspace(InputWorkspace=entry, OutputWorkspace=output_name)
            if self.getProperty("ExportASCII").value:
                SaveAscii(InputWorkspace=output_name, Filename="{}.dat".format(output_name))
        self.setProperty("OutputWorkspace", mtd[ws])

    def _get_shifts(self, calibration_file, zero_angle_corr, n_banks):
        """
        Loads the angular shifts for all banks from a provided ASCII file.

        Args:
        calibration_file: (str) path to bank shifts calibration file
        zero_angle_corr: (float) angular correction (in deg) to all banks
        n_banks: (int) number of banks of the detector
        return: (list) a list with angles for banks to be rotated around the sample (in deg)
        """
        bank_shifts = []
        try:
            with open(calibration_file, "r") as calibration:
                for line in calibration:
                    if "#" in line:  # comments are ignored
                        continue
                    line = line.strip(" ")
                    columns = line.split()
                    if columns:
                        angular_shift = float(columns[1]) + zero_angle_corr
                        bank_shifts.append(angular_shift)
                if len(bank_shifts) != n_banks:
                    raise RuntimeError("Bank shifts calibration file does not have entries for all banks.")
        except FileNotFoundError:
            self.log().warning("Bank calibration file not found or not provided.")
            bank_shifts = [zero_angle_corr] * n_banks
        except (RuntimeError, IndexError) as e:
            self.log().warning(str(e))
            self.log().warning("Padding the shifts list with zero angle correction.")
            bank_shifts.extend([zero_angle_corr] * (np.zeros(n_banks - len(bank_shifts))))
        return bank_shifts

    def _load_data(self, progress):
        """Loads the data scan, with each scan step in individual workspace, and splits detectors from monitors.

        Args:
        progress: (obj) object of Progress class allowing to follow execution of the loading
        """
        ws = "__{}".format(self.getPropertyValue("OutputWorkspace"))
        progress.report(0, "Loading data")
        LoadAndMerge(
            Filename=self.getPropertyValue("Run"), LoaderName="LoadILLDiffraction", OutputWorkspace=ws, startProgress=0.0, endProgress=0.3
        )
        det, mon = self._separate_detectors_monitors(ws)
        return ws, mon, progress

    def _get_normalisation_factor(self, det, mon, method, norm_standard):
        """
        Creates a workspace used as denominator in normalisation.

        Args:
        det: (str) name of the detector workspace
        mon: (str) name of the monitor workspace
        method: (str) normalisation method, one out of Monitor, Time, or None
        norm_standard: (float) value of the standard to which normalise
        return: name of normalisation workspace
        """
        if method == "Time":
            value = mtd[det].getRun().getLogData("duration").value
            error = 0.01  # s
        elif method == "Monitor":
            value = mtd[mon].readY(0)[0]
            error = mtd[mon].readE(0)[0]
        else:  # method == None
            value = 1.0
            error = 0.0
        norm_ws = "norm_factor"
        CreateSingleValuedWorkspace(OutputWorkspace=norm_ws, DataValue=value / norm_standard, ErrorValue=error / norm_standard)
        return norm_ws

    def _normalise(self, ws, mon, method, norm_standard):
        """
        Normalises the provided workspace using the chosen method and to a designated standard.

        Args:
        ws: (str) name of workspace to normalise
        mon: (str) monitor workspace name, used when method is Monitor
        method: (str) normalisation method, one out of Monitor, Time, or None
        norm_standard: (float) value of the standard to which normalise
        return: (str) name of normalised workspace
        """
        if isinstance(mtd[ws], WorkspaceGroup):
            normalised_list = []
            for entry_no, entry in enumerate(mtd[ws]):
                normalised_ws = self._normalise(entry.name(), mtd[mon][entry_no].name(), method, norm_standard)
                normalised_list.append(normalised_ws)
            normalised_group_ws = "{}_normalised".format(ws)
            GroupWorkspaces(InputWorkspaces=normalised_list, OutputWorkspace=normalised_group_ws)
            if self.getProperty("ClearCache").value:
                DeleteWorkspaces(WorkspaceList=[ws, mon])
            return normalised_group_ws
        normalisation_ws = self._get_normalisation_factor(ws, mon, method, norm_standard)
        normalised_ws = "{}_normalised".format(ws)
        Divide(LHSWorkspace=ws, RHSWorkspace=normalisation_ws, OutputWorkspace=normalised_ws)
        if self.getProperty("ClearCache").value:
            DeleteWorkspaces(WorkspaceList=[normalisation_ws])
        return normalised_ws

    def _rotate_banks(self, ws, shift):
        """
        Rotates all banks in a workspace by a value provided in the shift list.

        Args:
        ws: (str) workspace to be corrected
        shift: (list(float)) list containing angular rotations around the sample
        """
        bank_no = 0
        deg2rad = np.pi / 180.0
        compInfo = mtd[ws].componentInfo()
        for comp in compInfo:
            if "panel" in comp.name:
                if shift[bank_no] == 0:
                    bank_no += 1
                    continue
                pos = comp.position
                x, y, z = pos.getX(), pos.getY(), pos.getZ()
                # calculate rotation around Y by angle theta (in degrees)
                r = np.sqrt(x**2 + z**2)
                starting_angle = np.arctan2(x, z)
                newY = y
                newX = r * np.sin(starting_angle + shift[bank_no] * deg2rad)
                newZ = r * np.cos(starting_angle + shift[bank_no] * deg2rad)
                MoveInstrumentComponent(Workspace=ws, ComponentName=comp.name, X=newX, Y=newY, Z=newZ, RelativePosition=False)
                bank_no += 1

    def PyExec(self):
        nReports = 7
        progress = Progress(self, start=0.0, end=1.0, nreports=nReports)
        self._debug = self.getProperty("DebugMode").value

        ws, mon, progress = self._load_data(progress)
        progress.report("Correcting for dead-time")
        ws = self._correct_dead_time(ws, self.getProperty("DeadTimeTauDetector").value)
        mon = self._correct_dead_time(mon, self.getProperty("DeadTimeTauMonitor").value)
        progress.report("Correcting bank positions")
        ws = self._correct_bank_positions(ws)
        progress.report("Correcting relative efficiency")
        ws = self._correct_relative_efficiency(ws)
        progress.report("Normalising to monitor/time")
        ws = self._normalise(
            ws, mon, method=self.getPropertyValue("NormaliseBy"), norm_standard=self.getProperty("NormalisationStandard").value
        )
        progress.report("Creating diffractograms")
        ws = self._create_diffractograms(ws)
        progress.report("Finalizing")
        self._finalize(ws)


AlgorithmFactory.subscribe(D4ILLReduction)
