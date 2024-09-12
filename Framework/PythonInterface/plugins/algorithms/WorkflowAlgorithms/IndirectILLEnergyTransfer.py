# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import math
import numpy as np
from mantid import config, mtd, logger
from mantid.dataobjects import TableWorkspace
from mantid.kernel import (
    StringListValidator,
    Direction,
    FloatBoundedValidator,
    FloatArrayProperty,
    FloatArrayLengthValidator,
    IntBoundedValidator,
)
from mantid.api import (
    PythonAlgorithm,
    MultipleFileProperty,
    FileProperty,
    WorkspaceGroupProperty,
    FileAction,
    Progress,
    WorkspaceProperty,
    PropertyMode,
)
from mantid.simpleapi import *

N_TUBES = 16
N_PIXELS_PER_TUBE = 128
N_MONITOR = 1


def _ws_or_none(s):
    return mtd[s] if s != "" else None


def _make_name(name, suffix):
    return "__" + name + "_" + suffix


def _extract_workspace(ws, ws_out, x_start, x_end):
    """
    Extracts a part of the workspace and
    shifts the x-axis to start from 0
    @param  ws      :: input workspace name
    @param  ws_out  :: output workspace name
    @param  x_start :: start bin of workspace to be extracted
    @param  x_end   :: end bin of workspace to be extracted
    """
    CropWorkspace(InputWorkspace=ws, OutputWorkspace=ws_out, XMin=x_start, XMax=x_end)
    ScaleX(InputWorkspace=ws_out, OutputWorkspace=ws_out, Factor=-x_start, Operation="Add")


class IndirectILLEnergyTransfer(PythonAlgorithm):
    _run_file = None
    _map_file = None
    _parameter_file = None
    _reduction_type = None
    _mirror_sense = None
    _doppler_energy = None
    _doppler_speed = None
    _velocity_profile = None
    _instrument_name = None
    _instrument = None
    _analyser = None
    _reflection = None
    _dead_channels = None
    _ws = None
    _red_ws = None
    _psd_int_range = None
    _use_map_file = None
    _spectrum_axis = None
    _efixed = None
    _normalise_to = None
    _monitor_cutoff = None
    _sample_coords = None
    _fit_option = None
    _group_by = None
    _pulse_chopper = None
    _group_detectors = None
    _progress = None

    def category(self):
        return "Workflow\\MIDAS;Workflow\\Inelastic;Inelastic\\Indirect;Inelastic\\Reduction;ILL\\Indirect"

    def summary(self):
        return "Performs initial energy transfer reduction for ILL indirect geometry data, instrument IN16B."

    def seeAlso(self):
        return ["IndirectILLReductionQENS", "IndirectILLReductionFWS"]

    def name(self):
        return "IndirectILLEnergyTransfer"

    def PyInit(self):
        self.declareProperty(MultipleFileProperty("Run", extensions=["nxs"]), doc="File path of run (s).")

        self.declareProperty(
            FileProperty("MapFile", "", action=FileAction.OptionalLoad, extensions=["map", "xml"]),
            doc="Filename of the detector grouping map file to use. \n"
            "By default all the pixels will be summed per each tube. \n"
            "Use .map or .xml file (see GroupDetectors documentation) "
            "only if different range is needed for each tube.",
        )

        self.declareProperty(
            name="ManualPSDIntegrationRange",
            defaultValue=[1, 128],
            doc="Integration range of vertical pixels in each PSD tube. \n"
            "By default all the pixels will be summed per each tube. \n"
            "Use this option if the same range (other than default) "
            "is needed for all the tubes.",
        )

        self.declareProperty(name="Analyser", defaultValue="silicon", validator=StringListValidator(["silicon"]), doc="Analyser crystal.")

        self.declareProperty(
            name="Reflection", defaultValue="111", validator=StringListValidator(["111", "311"]), doc="Analyser reflection."
        )

        self.declareProperty(
            name="CropDeadMonitorChannels",
            defaultValue=False,
            doc="Whether or not to exclude the first and last few channels " "with 0 monitor count in the energy transfer formula.",
        )

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output), doc="Group name for the reduced workspace(s)."
        )

        self.declareProperty(
            name="SpectrumAxis",
            defaultValue="SpectrumNumber",
            validator=StringListValidator(["SpectrumNumber", "2Theta", "Q", "Q2"]),
            doc="The spectrum axis conversion target.",
        )

        self.declareProperty(
            name="NormaliseTo",
            defaultValue="Monitor",
            validator=StringListValidator(["Monitor", "None"]),
            doc="Choose to normalise to monitor.",
        )

        self.declareProperty(
            name="MonitorCutoff",
            defaultValue=0.5,
            validator=FloatBoundedValidator(lower=0.0, upper=1.0),
            doc="Choose the cutoff fraction wrt the maximum of the monitor counts.",
        )

        self.declareProperty(
            WorkspaceProperty("InputElasticChannelWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The name of the input elastic channel workspace.",
        )

        self.declareProperty(
            WorkspaceProperty("OutputElasticChannelWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The name of the output elastic channel workspace.",
        )

        self.declareProperty(
            name="ElasticPeakFitting",
            defaultValue="FitAllPixelGroups",
            validator=StringListValidator(["FitAllPixelGroups", "FitEquatorialOnly"]),
            doc="Choose the method for calibrating TOF axes.",
        )

        self.declareProperty(
            name="GroupPixelsBy",
            defaultValue=4,
            validator=IntBoundedValidator(lower=1, upper=128),
            doc="Choose how to group the pixels for elastic peak fitting; must be a power of 2.",
        )

        self.declareProperty(
            FloatArrayProperty("SampleCoordinates", [0.0, 0.0, 0.0], FloatArrayLengthValidator(3), direction=Direction.Input),
            doc="The sample coordinates X, Y, Z.",
        )

        self.declareProperty(
            name="PulseChopper", defaultValue="Auto", validator=StringListValidator(["Auto", "12", "34"]), doc="Define the pulse chopper."
        )

        bats_options = "BATS only options"
        self.setPropertyGroup("MonitorCutoff", bats_options)
        self.setPropertyGroup("InputElasticChannelWorkspace", bats_options)
        self.setPropertyGroup("OutputElasticChannelWorkspace", bats_options)
        self.setPropertyGroup("ElasticPeakFitting", bats_options)
        self.setPropertyGroup("GroupPixelsBy", bats_options)
        self.setPropertyGroup("SampleCoordinates", bats_options)
        self.setPropertyGroup("PulseChopper", bats_options)

        self.declareProperty(
            name="GroupDetectors",
            defaultValue=True,
            doc="Group the pixels using the range, tube-by-tube (default) or in a custom way; \n"
            "it is not recommended to group the detectors at this stage, \n"
            "in order to get absorption corrections right, \n"
            "however the default value is True for backwards compatibility.",
        )

        self.declareProperty(name="DiscardSingleDetectors", defaultValue=False, doc="Whether to discard the spectra of single detectors.")

        self.declareProperty(name="DeleteMonitorWorkspace", defaultValue=True, doc="Whether to delete the monitor spectrum.")
        self.setPropertyGroup("DeleteMonitorWorkspace", bats_options)

    def validateInputs(self):
        issues = dict()

        self._psd_int_range = self.getProperty("ManualPSDIntegrationRange").value

        if not self.getPropertyValue("MapFile"):
            if len(self._psd_int_range) != 2:
                issues["ManualPSDIntegrationRange"] = "Specify comma separated pixel range, e.g. 1,128"
            elif (
                self._psd_int_range[0] < 1 or self._psd_int_range[1] > N_PIXELS_PER_TUBE or self._psd_int_range[0] >= self._psd_int_range[1]
            ):
                issues["ManualPSDIntegrationRange"] = "Start or end pixel number is out of range [1-128], or has wrong order"

        group_by = self.getProperty("GroupPixelsBy").value
        if group_by <= 0 or (group_by & (group_by - 1)) != 0:  # quick check if the number is a power of 2
            issues["GroupPixelsBy"] = "Group by must be a power of 2, e.g. 2, 4, 8, 16 ... 128"

        epp_ws = self.getProperty("InputElasticChannelWorkspace").value
        if epp_ws and not isinstance(epp_ws, TableWorkspace):
            issues["InputElasticChannelWorkspace"] = "Input EPP workspace must be a TableWorkspace."

        return issues

    def setUp(self):
        """
        Sets up the input properties to configure the reduction flow
        """
        self._run_file = self.getPropertyValue("Run").replace(",", "+")  # automatic summing
        self._analyser = self.getPropertyValue("Analyser")
        self._map_file = self.getPropertyValue("MapFile")
        self._reflection = self.getPropertyValue("Reflection")
        self._dead_channels = self.getProperty("CropDeadMonitorChannels").value
        self._red_ws = self.getPropertyValue("OutputWorkspace")
        self._spectrum_axis = self.getPropertyValue("SpectrumAxis")
        self._normalise_to = self.getPropertyValue("NormaliseTo")
        self._monitor_cutoff = self.getProperty("MonitorCutoff").value
        self._sample_coords = self.getProperty("SampleCoordinates").value
        self._fit_option = self.getPropertyValue("ElasticPeakFitting")
        self._group_by = self.getProperty("GroupPixelsBy").value
        self._pulse_chopper = self.getPropertyValue("PulseChopper")
        self._group_detectors = self.getProperty("GroupDetectors").value

        self._use_map_file = self._map_file != ""

    def _load_map_file(self):
        """
        Loads the detector grouping map file
        @throws RuntimeError :: if neither the user defined nor the default file is found
        """

        self._instrument_name = self._instrument.getName()
        self._analyser = self.getPropertyValue("Analyser")
        self._reflection = self.getPropertyValue("Reflection")
        idf_directory = config["instrumentDefinition.directory"]
        ipf_name = self._instrument_name + "_" + self._analyser + "_" + self._reflection + "_Parameters.xml"
        self._parameter_file = os.path.join(idf_directory, ipf_name)
        self.log().information("Set parameter file : {0}".format(self._parameter_file))

    def _mask(self, ws, xstart, xend):
        """
        Masks the first and last bins
        @param   ws           :: input workspace name
        @param   xstart       :: MaskBins between x[0] and x[xstart]
        @param   xend         :: MaskBins between x[xend] and x[-1]
        """
        x_values = mtd[ws].readX(0)

        if xstart > 0:
            logger.debug("Mask bins smaller than {0}".format(xstart))
            MaskBins(InputWorkspace=ws, OutputWorkspace=ws, XMin=x_values[0], XMax=x_values[xstart])

        if xend < len(x_values) - 1:
            logger.debug("Mask bins larger than {0}".format(xend))
            MaskBins(InputWorkspace=ws, OutputWorkspace=ws, XMin=x_values[xend + 1], XMax=x_values[-1])

    def _convert_to_energy(self, ws):
        """
        Converts the x-axis from raw channel number to energy transfer for Doppler mode
        @param ws :: input workspace name
        """
        from scipy.constants import physical_constants

        c = physical_constants["speed of light in vacuum"][0]
        nm = physical_constants["neutron mass energy equivalent in MeV"][0]

        bsize = mtd[ws].blocksize()
        if self._doppler_energy != 0:
            # the doppler channels are linear in velocity (not time, neither deltaE)
            # so we perform 2-step conversion, first linear to v, then quadratic to deltaE
            efixed = mtd[ws].getInstrument().getNumberParameter("Efixed")[0]
            vfixed = math.sqrt(2 * efixed * c**2 / (nm * 1e9))
            vformula = "-2/({0}-1)*{1}*(x-{0}/2)+{2}".format(bsize, self._doppler_speed, vfixed)
            ConvertAxisByFormula(InputWorkspace=ws, OutputWorkspace=ws, Axis="X", Formula=vformula)
            nmass = nm * 1e9 / c**2  # mev / (m/s)**2
            eformula = "{0}*x*x/2 - {1}".format(nmass, efixed)
            ConvertAxisByFormula(InputWorkspace=ws, OutputWorkspace=ws, Axis="X", Formula=eformula, AxisUnits="DeltaE")
        else:
            # Center the data for elastic fixed window scan, for integration over the elastic peak
            # Here all that matters is that the center is at 0 deltaE, but there is no real meaning of the axis extent
            formula = "-(x-{0})*{1}".format(bsize / 2 - 0.5, 1e3)
            self.log().debug("The only energy value is 0 meV. Ignore the x-axis.")
            ConvertAxisByFormula(InputWorkspace=ws, OutputWorkspace=ws, Axis="X", Formula=formula, AxisUnits="DeltaE")

    @staticmethod
    def _monitor_max_range(ws):
        """
        Gives the bin indices of the first and last peaks in the monitor
        @param ws :: input workspace name
        return    :: [xmin,xmax]
        """

        y = mtd[ws].readY(0)
        size = len(y)
        mid = int(size / 2)
        imin = np.nanargmax(y[0:mid])
        imax = np.nanargmax(y[mid:size]) + mid
        return imin, imax

    @staticmethod
    def _monitor_zero_range(ws):
        """
        Gives the bin indices of the first and last bins in monitor, having at least 1% of max monitor count
        @param ws :: input workspace name
        return    :: [start,end]
        """

        y = mtd[ws].readY(0)
        nonzero = np.argwhere(y > max(y) / 100)
        start = nonzero[0][0] if nonzero.any() else 0
        end = nonzero[-1][0] if nonzero.any() else len(y)
        return start, end

    def _setup_run_properties(self):
        """
        Sets up the doppler properties, and deduces the reduction type
        @throws RuntimeError :: If anyone of the 3 required entries is missing
        """

        run = mtd[self._ws].getRun()

        message = "is not defined. Check your data."

        if run.hasProperty("Doppler.mirror_sense"):
            self._mirror_sense = run.getLogData("Doppler.mirror_sense").value
        else:
            raise RuntimeError("Mirror sense " + message)

        if run.hasProperty("Doppler.maximum_delta_energy"):
            self._doppler_energy = run.getLogData("Doppler.maximum_delta_energy").value
        else:
            raise RuntimeError("Maximum delta energy " + message)

        if run.hasProperty("Doppler.doppler_speed"):
            self._doppler_speed = run.getLogData("Doppler.doppler_speed").value
        else:
            raise RuntimeError("Doppler speed " + message)

        if run.hasProperty("Doppler.velocity_profile"):
            self._velocity_profile = run.getLogData("Doppler.velocity_profile").value
        else:
            raise RuntimeError("Velocity profile " + message)

        if self._doppler_energy == 0.0:
            self._reduction_type = "EFWS"
        else:
            if self._velocity_profile == 0:
                self._reduction_type = "QENS"
            else:
                self._reduction_type = "IFWS"

        if run.hasProperty("acquisition_mode"):
            if run.getLogData("acquisition_mode").value == 1:
                self._reduction_type = "BATS"

    def PyExec(self):
        self.setUp()

        self._progress = Progress(self, start=0.0, end=1.0, nreports=self._run_file.count("+"))

        LoadAndMerge(Filename=self._run_file, OutputWorkspace=self._red_ws, LoaderName="LoadILLIndirect")

        self._instrument = mtd[self._red_ws].getInstrument()

        self._load_map_file()

        run = str(mtd[self._red_ws].getRun().getLogData("run_number").value)[:6]

        self._ws = self._red_ws + "_" + run

        if self._run_file.count("+") > 0:  # multiple summed files
            self._ws += "_multiple"

        RenameWorkspace(InputWorkspace=self._red_ws, OutputWorkspace=self._ws)

        LoadParameterFile(Workspace=self._ws, Filename=self._parameter_file)

        self._efixed = self._instrument.getNumberParameter("Efixed")[0]

        self._setup_run_properties()

        if self._reduction_type == "BATS":
            self._reduce_bats(self._ws)
        else:
            if self._mirror_sense == 14:  # two wings, extract left and right
                size = mtd[self._ws].blocksize()
                left = self._ws + "_left"
                right = self._ws + "_right"
                _extract_workspace(self._ws, left, 0, size // 2)
                _extract_workspace(self._ws, right, size // 2, size)
                DeleteWorkspace(self._ws)
                self._reduce_one_wing_doppler(left)
                self._reduce_one_wing_doppler(right)
                GroupWorkspaces(InputWorkspaces=[left, right], OutputWorkspace=self._red_ws)

            elif self._mirror_sense == 16:  # one wing
                self._reduce_one_wing_doppler(self._ws)
                GroupWorkspaces(InputWorkspaces=[self._ws], OutputWorkspace=self._red_ws)

        if self._normalise_to == "Monitor":
            for ws in mtd[self._red_ws]:
                AddSampleLog(Workspace=ws, LogName="NormalisedTo", LogType="String", LogText="Monitor", EnableLogging=False)

        self.setProperty("OutputWorkspace", self._red_ws)

    def _create_elastic_channel_ws(self, epp_ws, run, epp_equator_ws=None, single_detectors=0):
        """
        Creates the elastic channel table workspace.
        @param epp_ws: the workspace containing the EPP
        @param run: run object
        @param epp_equator_ws: EPP workspace of the equatorial line
        @param single_detectors: the number of single detectors
        """
        speed, phase, _ = self._get_pulse_chopper_info(run, self._pulse_chopper)
        delay = run.getLogData("PSD.time_of_flight_2").value
        delay_sd = run.getLogData("SingleD.time_of_flight_2").value
        epp_ws.removeColumn("WorkspaceIndex")
        epp_ws.removeColumn("PeakCentreError")
        epp_ws.removeColumn("Sigma")
        epp_ws.removeColumn("SigmaError")
        epp_ws.removeColumn("Height")
        epp_ws.removeColumn("HeightError")
        epp_ws.removeColumn("chiSq")
        if epp_equator_ws:
            equator_center = epp_equator_ws.cell("PeakCentre", 0)
            center_status = epp_equator_ws.cell("FitStatus", 0)
            equator_row = {"PeakCentre": equator_center, "FitStatus": center_status}
            epp_ws.addRow(equator_row)
        epp_ws.addColumn("double", "ChopperSpeed")
        epp_ws.addColumn("double", "ChopperPhase")
        epp_ws.addColumn("double", "TOF_Delay")
        equator_epp = 1 if epp_equator_ws else 0
        for row in range(epp_ws.rowCount()):
            epp_ws.setCell("ChopperSpeed", row, speed)
            epp_ws.setCell("ChopperPhase", row, phase)
            if epp_ws.rowCount() - single_detectors - equator_epp <= row < epp_ws.rowCount() - equator_epp:
                epp_ws.setCell("TOF_Delay", row, delay_sd)
            else:
                epp_ws.setCell("TOF_Delay", row, delay)

    @staticmethod
    def _t0_offset(center_chopper_speed, center_chopper_phase, shifted_chopper_phase, center_psd_delay, shifted_psd_delay):
        """
        Calculates the t0 offset between measurements with and without inelastic offset.
        """
        return -(shifted_chopper_phase - center_chopper_phase) / center_chopper_speed / 6 + (shifted_psd_delay - center_psd_delay) * 1e-6

    def _get_pulse_chopper_info(self, run, pulse):
        """
        Retrieves information on pulse chopper pair.
        In low repetition mode pulse chopper and trigger chopper are not the same.
        In high repetition mode pulse trigger is always 1.
        @param run : the run of workspace
        @param pulse : 'Auto', '12' or '34'
        @return : [pulse chopper speed, pulse chopper phase, source distance]
        """
        pulse_index = 1
        distance = 0.0
        if pulse == "Auto":
            if run.hasProperty("monitor.master_pickup"):
                trigger = run.getLogData("monitor.master_pickup").value
                if not 1 <= trigger <= 4:
                    self.log().information("Unexpected trigger chopper " + str(trigger))
                else:
                    if trigger == 1 or trigger == 2:  # and low repetition rate
                        pulse_index = 3
                        distance = 33.388
        elif pulse == "34":
            pulse_index = 3
            distance = 33.388

        chopper_speed_param = "CH{0}.rotation_speed".format(pulse_index)
        chopper_phase_param = "CH{0}.phase".format(pulse_index)

        if not run.hasProperty(chopper_speed_param) or not run.hasProperty(chopper_phase_param):
            raise RuntimeError("Unable to retrieve the pulse chopper speed and phase.")
        else:
            speed = run.getLogData(chopper_speed_param).value
            phase = run.getLogData(chopper_phase_param).value

        return [speed, phase, distance]

    def _convert_to_energy_bats(self, ws, epp_ws, t0_offset=0.0, sd_t0_offset=0.0):
        """
        Converts the workspace to TOF for Bats mode.
        @param ws: input workspace
        @param epp_ws: input EPP workspace
        @param t0_offset: TOF offset for the measurements with inelastic shift
        @param sd_t0_offset: TOF offset for single detectors
        """
        detector_info = ws.detectorInfo()
        l1 = detector_info.l1()
        middle = N_PIXELS_PER_TUBE // 2
        l2_equator = (detector_info.l2(middle) + detector_info.l2(middle + 1)) / 2.0
        v_fixed = self._instrument.getNumberParameter("Vfixed")[0]
        elastic_tof_equator = ((l1 + l2_equator) / v_fixed + t0_offset) * 1e6
        run = ws.getRun()
        channel_width = run.getLogData("PSD.time_of_flight_0").value
        input_epp = self.getProperty("InputElasticChannelWorkspace").value
        output_epp = self.getPropertyValue("OutputElasticChannelWorkspace")
        rows = epp_ws.rowCount()

        elastic_channel_equator = epp_ws.cell("PeakCentre", rows - 1)
        x = ws.extractX()

        elastic_tof = elastic_tof_equator
        elastic_channel = elastic_channel_equator

        x_new = elastic_tof + (x[0] - elastic_channel) * channel_width
        ws.setX(0, x_new)

        expected_row_count = (N_TUBES * N_PIXELS_PER_TUBE) // self._group_by + self._get_single_detectors_number(ws.name()) + 1

        if self._fit_option == "FitAllPixelGroups" and rows != expected_row_count:
            self._fit_option = "FitEquatorialOnly"
            self.log().notice(
                "Not enough rows in the elastic channel workspace to use all pixel groups fitting. "
                "Switching to using only the equatorial fitting."
            )

        for pixel in range(1, N_PIXELS_PER_TUBE * N_TUBES + N_MONITOR):
            group = (pixel - 1) // self._group_by
            if self._fit_option == "FitAllPixelGroups" and epp_ws.cell("FitStatus", group) == "success":
                l2 = detector_info.l2(pixel)
                elastic_tof = ((l1 + l2) / v_fixed + t0_offset) * 1e6
                elastic_channel = epp_ws.cell("PeakCentre", group)
            else:
                elastic_tof = elastic_tof_equator
                elastic_channel = elastic_channel_equator
            x_new = elastic_tof + (x[pixel] - elastic_channel) * channel_width
            ws.setX(pixel, x_new)

        for single_detector in range(N_PIXELS_PER_TUBE * N_TUBES + N_MONITOR, ws.getNumberHistograms()):
            group = rows - 1 - (ws.getNumberHistograms() - single_detector)
            if epp_ws.cell("FitStatus", group) in ["success", "narrowPeak"]:
                l2 = detector_info.l2(single_detector)
                elastic_tof = ((l1 + l2) / v_fixed + sd_t0_offset) * 1e6
                elastic_channel = epp_ws.cell("PeakCentre", group)
            else:
                elastic_tof = elastic_tof_equator
                elastic_channel = elastic_channel_equator
                self.log().warning(
                    "Single detector number {0}'s peak was not found. Using equatorial peak instead."
                    " The result is likely erroneous.".format(single_detector + 1)
                )

            x_new = elastic_tof + (x[single_detector] - elastic_channel) * channel_width
            ws.setX(single_detector, x_new)
        ws.getAxis(0).setUnit("TOF")

        if output_epp:
            RenameWorkspace(InputWorkspace=epp_ws, OutputWorkspace=output_epp)
            self.setProperty("OutputElasticChannelWorkspace", output_epp)
        elif not input_epp:
            DeleteWorkspace(epp_ws)

        self._rebin_to_monitor(ws.name(), v_fixed, elastic_tof_equator)

    def _rebin_to_monitor(self, ws, v_fixed, elastic_tof_equator):
        """
        Adjusts the TOF of the monitor, converts to energy and rebins data to the monitor binning.
        @param ws: input workspace name
        @param v_fixed: elastic velocity defined by the analyser
        @param elastic_tof_equator : elastic time-of-flight of the equatorial line
        """
        run = mtd[ws].getRun()
        mon_ws = _make_name(ws, "mon")
        ExtractMonitors(InputWorkspace=ws, DetectorWorkspace=ws, MonitorWorkspace=mon_ws)
        delay_offset = run.getLogData("monitor.time_of_flight_2").value - run.getLogData("PSD.time_of_flight_2").value
        ScaleX(InputWorkspace=mon_ws, OutputWorkspace=mon_ws, Factor=delay_offset, Operation="Add")
        frame_width = 1e6 * 2 * 4.0 / v_fixed
        mon_data = mtd[mon_ws].readY(0)
        mon_elastic = np.argmax(mon_data)
        mon_elastic_tof = mtd[mon_ws].readX(0)[mon_elastic]
        n_frames_diff = math.floor((mon_elastic_tof - elastic_tof_equator) / frame_width) + 1
        ScaleX(InputWorkspace=mon_ws, OutputWorkspace=mon_ws, Factor=-n_frames_diff * frame_width, Operation="Add")
        # Energy is elastic energy and currently doesn't take the energy transfer into account, so do this in lambda
        ConvertUnits(InputWorkspace=mon_ws, OutputWorkspace=mon_ws, Target="Wavelength", EMode="Elastic")
        ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target="Wavelength", EMode="Indirect")
        monitor = mtd[mon_ws].readY(0)
        x_axis = mtd[mon_ws].readX(0)
        cutoff = np.max(monitor) * self._monitor_cutoff
        mon_range = x_axis[:-1][monitor > cutoff]
        self.log().information("Cutoff from {0} to {1} in Energy [mev]".format(mon_range[0], mon_range[-1]))
        CropWorkspace(InputWorkspace=mon_ws, OutputWorkspace=mon_ws, XMin=mon_range[0], XMax=mon_range[-1])
        CropWorkspace(InputWorkspace=ws, OutputWorkspace=ws, XMin=mon_range[0], XMax=mon_range[-1])
        RebinToWorkspace(WorkspaceToRebin=ws, WorkspaceToMatch=mon_ws, OutputWorkspace=ws)
        if self._normalise_to == "Monitor":
            Divide(LHSWorkspace=ws, RHSWorkspace=mon_ws, OutputWorkspace=ws)
            ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws, NaNValue=0, NaNError=0, InfinityValue=0, InfinityError=0)
        if self.getProperty("DeleteMonitorWorkspace").value:
            DeleteWorkspace(mon_ws)
        else:
            # if kept, it is desired to have in DeltaE, which is not defined for monitor, so we need to do E, then scale X by Efixed
            ConvertUnits(InputWorkspace=mon_ws, OutputWorkspace=mon_ws, Target="Energy", EMode="Elastic")
            ScaleX(InputWorkspace=mon_ws, OutputWorkspace=mon_ws, Operation="Add", Factor=-self._efixed)
            mtd[mon_ws].getAxis(0).setUnit("DeltaE")
            RenameWorkspace(InputWorkspace=mon_ws, OutputWorkspace=mon_ws[2:])

    def _reduce_bats(self, ws):
        """
        Reduces inverted TOF mode (BATS)
        @param ws :: input workspace name
        """
        x, y, z = self._sample_coords
        if x**2 + y**2 + z**2 != 0.0:
            MoveInstrumentComponent(Workspace=ws, ComponentName="sample-position", X=x, Y=y, Z=z, RelativePosition=False)
        distance = self._get_pulse_chopper_info(mtd[ws].getRun(), self._pulse_chopper)[2]
        if distance != 0.0:
            MoveInstrumentComponent(Workspace=ws, ComponentName="chopper", Z=-distance, RelativePosition=False)

        input_epp = self.getProperty("InputElasticChannelWorkspace").value

        single_detectors = self._get_single_detectors_number(ws)
        offset = N_MONITOR if mtd[ws].detectorInfo().isMonitor(0) else 0

        if not input_epp:
            equator_epp_ws = _make_name(ws, "eq_epp")
            equator_ws = _make_name(ws, "eq")
            grouped_ws = _make_name(ws, "gr")
            epp_ws = _make_name(ws, "epp")
            equator_grouping_filename = self._instrument.getStringParameter("EquatorialGroupingFile")[0]
            grouping_file = os.path.join(config["groupingFiles.directory"], equator_grouping_filename)
            GroupDetectors(InputWorkspace=ws, OutputWorkspace=equator_ws, MapFile=grouping_file)
            to_crop = mtd[ws].blocksize() / 4
            CropWorkspace(InputWorkspace=equator_ws, OutputWorkspace=equator_ws, XMin=to_crop, XMax=3 * to_crop)
            FindEPP(InputWorkspace=equator_ws, OutputWorkspace=equator_epp_ws)
            DeleteWorkspace(equator_ws)

            if self._fit_option == "FitAllPixelGroups":
                GroupDetectors(
                    InputWorkspace=ws,
                    OutputWorkspace=grouped_ws,
                    GroupingPattern=self._group_pixels(self._group_by, single_detectors, offset),
                )
                CropWorkspace(InputWorkspace=grouped_ws, OutputWorkspace=grouped_ws, XMin=to_crop, XMax=3 * to_crop)
                FindEPP(InputWorkspace=grouped_ws, OutputWorkspace=epp_ws)
                self._create_elastic_channel_ws(mtd[epp_ws], mtd[ws].getRun(), mtd[equator_epp_ws], single_detectors=single_detectors)
                DeleteWorkspaces([equator_epp_ws, grouped_ws])
            else:
                single_det_ws = _make_name(ws, "sds")
                ExtractSpectra(
                    InputWorkspace=ws,
                    OutputWorkspace=single_det_ws,
                    StartWorkspaceIndex=N_TUBES * N_PIXELS_PER_TUBE + offset,
                    EndWorkspaceIndex=N_TUBES * N_PIXELS_PER_TUBE + single_detectors + offset - 1,
                )
                FindEPP(InputWorkspace=single_det_ws, OutputWorkspace=epp_ws)
                self._create_elastic_channel_ws(mtd[epp_ws], mtd[ws].getRun(), mtd[equator_epp_ws], single_detectors=single_detectors)
                DeleteWorkspaces([equator_epp_ws, single_det_ws])
            self._convert_to_energy_bats(mtd[ws], mtd[epp_ws])
        else:
            run = mtd[ws].getRun()

            center_chopper_speed = input_epp.cell("ChopperSpeed", 0)
            center_chopper_phase = input_epp.cell("ChopperPhase", 0)
            center_psd_delay = input_epp.cell("TOF_Delay", 0)

            shifted_chopper_phase = self._get_pulse_chopper_info(run, self._pulse_chopper)[1]
            shifted_psd_delay = run.getLogData("PSD.time_of_flight_2").value

            psd_t0_offset = self._t0_offset(
                center_chopper_speed, center_chopper_phase, shifted_chopper_phase, center_psd_delay, shifted_psd_delay
            )
            sd_t0_offset = 0
            if single_detectors:
                shifted_sd_delay = run.getLogData("SingleD.time_of_flight_2").value
                center_sd_delay = input_epp.cell("TOF_Delay", input_epp.rowCount() - 2)
                sd_t0_offset = self._t0_offset(
                    center_chopper_speed, center_chopper_phase, shifted_chopper_phase, center_sd_delay, shifted_sd_delay
                )
                self.log().information("SD T0 Offset is {0} [sec]".format(sd_t0_offset))

            self.log().information("PSD T0 Offset is {0} [sec]".format(psd_t0_offset))

            self._convert_to_energy_bats(mtd[ws], input_epp, psd_t0_offset, sd_t0_offset)

        rebin_ws = _make_name(ws, "rebin")
        ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target="DeltaE", EMode="Indirect")
        ExtractSingleSpectrum(InputWorkspace=ws, OutputWorkspace=rebin_ws, WorkspaceIndex=int(N_PIXELS_PER_TUBE / 2))
        RebinToWorkspace(WorkspaceToRebin=ws, WorkspaceToMatch=rebin_ws, OutputWorkspace=ws)
        if self._group_detectors:
            self._do_group_detectors(ws)
        if self._normalise_to == "Monitor":
            mtd[ws].setDistribution(True)
        GroupWorkspaces(InputWorkspaces=[ws], OutputWorkspace=self._red_ws)
        DeleteWorkspaces([rebin_ws])

    @staticmethod
    def _group_pixels(by=4, single_detectors=0, offset=1):
        """
        Groups pixels in the tubes by the factor, which should be a power of 2.
        @param by : the group pixels by
        """
        pattern = ""
        for i in range(N_TUBES):
            for j in range(N_PIXELS_PER_TUBE // by):
                start = i * N_PIXELS_PER_TUBE + j * by + offset
                end = start + by - 1
                pattern += str(start) + "-" + str(end) + ","

        for single_det in range(single_detectors):
            sd_index = N_TUBES * N_PIXELS_PER_TUBE + single_det + offset
            pattern += str(sd_index)
            pattern += ","

        return pattern[:-1]

    def _do_group_detectors(self, ws):
        """
        Groups the pixels either tube by tube (default), or by user given range or by user given grouping file
        @param ws : the workspace to group
        """
        if self._use_map_file:
            GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws, MapFile=self._map_file)
        else:
            self._group_detectors_with_range(ws)

    def _reduce_one_wing_doppler(self, ws):
        """
        Reduces given workspace in doppler mode assuming it is one wing already
        @param ws :: input workspace name
        """

        mon = "__mon_" + ws

        ExtractSingleSpectrum(InputWorkspace=ws, OutputWorkspace=mon, WorkspaceIndex=0)

        if self._group_detectors:
            self._do_group_detectors(ws)

        xmin, xmax = self._monitor_zero_range(mon)

        if self._normalise_to == "Monitor":
            self._normalise_to_monitor(ws, mon)

        if self._reduction_type == "QENS":
            if self._dead_channels:
                CropWorkspace(InputWorkspace=ws, OutputWorkspace=ws, XMin=float(xmin), XMax=float(xmax + 1.0))
                ScaleX(InputWorkspace=ws, OutputWorkspace=ws, Factor=-float(xmin), Operation="Add")
            else:
                self._mask(ws, xmin, xmax)

        DeleteWorkspace(mon)

        self._convert_to_energy(ws)

        target = None
        if self._spectrum_axis == "2Theta":
            target = "Theta"
        elif self._spectrum_axis == "Q":
            target = "ElasticQ"
        elif self._spectrum_axis == "Q2":
            target = "ElasticQSquared"

        if self._spectrum_axis != "SpectrumNumber":
            ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace=ws, EMode="Indirect", Target=target, EFixed=self._efixed)

    def _group_detectors_with_range(self, ws):
        """
        Groups (sums) the multi-detector's pixels according to given range
        @param ws :: input workspace name
        """
        pattern = ""

        # if the first spectrum does not correspond to a monitor, start from there
        offset = 0 if mtd[ws].getDetector(0).isMonitor() else -1

        for tube in range(1, N_TUBES + 1):
            pattern += str((tube - 1) * N_PIXELS_PER_TUBE + self._psd_int_range[0] + offset)
            pattern += "-"
            pattern += str((tube - 1) * N_PIXELS_PER_TUBE + self._psd_int_range[1] + offset)
            pattern += ","

        if not self.getProperty("DiscardSingleDetectors").value:
            num_single_det = self._get_single_detectors_number(ws)
            for single_det in range(num_single_det):
                sd_index = N_TUBES * N_PIXELS_PER_TUBE + single_det + offset + 1
                pattern += str(sd_index)
                pattern += ","

        pattern = pattern.rstrip(",")

        self.log().information("Grouping the detectors with pattern:\n {0}".format(pattern))

        GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws, GroupingPattern=pattern)

        # detector grouping using a pattern does not use the same convention for spectrum numbering than
        # with grouping files, so to keep consistency (and the tests happy), we set them manually
        for i in range(mtd[ws].getNumberHistograms()):
            mtd[ws].getSpectrum(i).setSpectrumNo(i)

    @staticmethod
    def _get_single_detectors_number(ws):
        """
        Get the total number of single detectors in the workspace.
        @param ws :: the workspace name, a string
        @return the total number of single detectors
        """
        monitor_count = N_MONITOR if mtd[ws].getDetector(0).isMonitor() else 0
        return mtd[ws].getNumberHistograms() - N_TUBES * N_PIXELS_PER_TUBE - monitor_count

    def _normalise_to_monitor(self, ws, mon):
        """
        Normalises the ws to the monitor dependent on the reduction type (doppler mode)
        @param ws :: input workspace name
        @param mon :: ws's monitor
        """
        x = mtd[ws].readX(0)

        if self._reduction_type == "QENS":
            # Normalise bin-to-bin, do not use NormaliseToMonitor, it uses scaling that we don't want
            Divide(LHSWorkspace=ws, OutputWorkspace=ws, RHSWorkspace=mon)

        elif self._reduction_type == "EFWS":
            # Integrate over the whole range

            int_ws = "__integral1_" + ws
            Integration(InputWorkspace=mon, OutputWorkspace=int_ws, RangeLower=x[0], RangeUpper=x[-1])

            if mtd[int_ws].readY(0)[0] != 0:  # this needs to be checked
                Scale(InputWorkspace=ws, OutputWorkspace=ws, Factor=1.0 / mtd[int_ws].readY(0)[0])

            # remember the integral of the monitor
            AddSampleLog(
                Workspace=ws, LogName="MonitorIntegral", LogType="Number", LogText=str(mtd[int_ws].readY(0)[0]), EnableLogging=False
            )

            DeleteWorkspace(int_ws)

        elif self._reduction_type == "IFWS":
            # Integrate over the two peaks at the beginning and at the end and sum
            size = mtd[ws].blocksize()
            x_start, x_end = self._monitor_max_range(mon)

            i1 = "__integral1_" + ws
            i2 = "__integral2_" + ws
            int_ws = "__integral_" + ws

            Integration(InputWorkspace=mon, OutputWorkspace=i1, RangeLower=x[0], RangeUpper=x[2 * x_start])

            Integration(InputWorkspace=mon, OutputWorkspace=i2, RangeLower=x[-2 * (size - x_end)], RangeUpper=x[-1])

            Plus(LHSWorkspace=i1, RHSWorkspace=i2, OutputWorkspace=int_ws)

            if mtd[int_ws].readY(0)[0] != 0:  # this needs to be checked
                Scale(InputWorkspace=ws, OutputWorkspace=ws, Factor=1.0 / mtd[int_ws].readY(0)[0])

            # remember the integral of the monitor
            AddSampleLog(
                Workspace=ws, LogName="MonitorIntegral", LogType="Number", LogText=str(mtd[int_ws].readY(0)[0]), EnableLogging=False
            )

            # store the x_start and x_end derived from monitor, needed later for integration
            AddSampleLogMultiple(Workspace=ws, LogNames=["MonitorLeftPeak", "MonitorRightPeak"], LogValues=[x_start, x_end])

            DeleteWorkspace(i1)
            DeleteWorkspace(i2)
            DeleteWorkspace(int_ws)

        ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws, NaNValue=0, NaNError=0, InfinityValue=0, InfinityError=0)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLEnergyTransfer)
