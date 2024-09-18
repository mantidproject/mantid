# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-lines, invalid-name, bare-except, too-many-instance-attributes
import math
import re

from mantid.simpleapi import *
from mantid.api import WorkspaceGroup, Workspace
from mantid.kernel import Logger
from mantid.kernel import V3D
import SANSUtility as su
from math import copysign

sanslog = Logger("SANS")


class BaseInstrument(object):
    def __init__(self, instr_filen=None):
        """
        Reads the instrument definition xml file
        @param instr_filen: the name of the instrument definition file to read
        @raise IndexError: if any parameters (e.g. 'default-incident-monitor-spectrum') aren't in the xml definition
        """
        if instr_filen is None:
            instr_filen = self._NAME + "_Definition.xml"

        config = ConfigService.Instance()
        self._definition_file = os.path.join(config["instrumentDefinition.directory"], instr_filen)

        inst_ws_name = self.load_empty()
        self.definition = AnalysisDataService.retrieve(inst_ws_name).getInstrument()

    def get_idf_file_path(self):
        return self._definition_file

    def get_default_beam_center(self):
        """
        Returns the default beam center position, or the pixel location
        of real-space coordinates (0,0).
        """
        return [0, 0]

    def name(self):
        """
        Return the name of the instrument
        """
        return self._NAME

    def versioned_name(self):
        """
        Hack-workaround so that we may temporarily display "SANS2DTUBES" as
        an option in the instrument dropdown menu in the interface.  To be removed
        as part of #9367.
        """
        if "SANS2D_Definition_Tubes" in self.idf_path:
            return "SANS2DTUBES"
        return self._NAME

    def load_empty(self, workspace_name=None):
        """
        Loads the instrument definition file into a workspace with the given name.
        If no name is given a hidden workspace is used
        @param workspace_name: the name of the workspace to create and/or display
        @return the name of the workspace that was created
        """
        if workspace_name is None:
            workspace_name = "__" + self._NAME + "_empty"

        LoadEmptyInstrument(Filename=self._definition_file, OutputWorkspace=workspace_name)

        return workspace_name


class DetectorBank(object):
    class _DectShape(object):
        """
        Stores the dimensions of the detector, normally this is a square
        which is easy, but it can have a hole in it which is harder!
        """

        def __init__(self, width, height, isRect=True, n_pixels=None):
            """
            Sets the dimensions of the detector
            @param width: the detector's width, spectra numbers along the width should increase in intervals of one
            @param height: the detector's height, spectra numbers along the down the height should increase in intervals of width
            @param isRect: true for rectangular or square detectors, i.e. number of pixels = width * height
            @param n_pixels: optional for rectangular shapes because if it is not given it is calculated from the
            height and width in that case
            """
            self._width = width
            self._height = height
            self._isRect = bool(isRect)
            self._n_pixels = n_pixels
            if n_pixels is None:
                if self._isRect:
                    self._n_pixels = self._width * self._height
                else:
                    raise AttributeError(
                        "Number of pixels in the detector unknown, you must state the number of pixels for non-rectangular detectors"
                    )

        def width(self):
            """
            read-only property getter, this object can't be altered
            """
            return self._width

        def height(self):
            return self._height

        def isRectangle(self):
            return self._isRect

        def n_pixels(self):
            return self._n_pixels

    class _MergeRange(object):
        """
        Stores property about the detector which is used to specify the merge ranges after the data has been reduced.
        """

        def __init__(self, q_min=None, q_max=None):
            """
            @param q_max: Default to None. Merge region maximum
            @param q_min: Default to 0.0. Merge region minimum
            """
            self.q_min = q_min
            self.q_max = q_max

            if self.q_min is None and self.q_max is None:
                self.q_merge_range = False
            else:
                self.q_merge_range = True

    class _RescaleAndShift(object):
        """
        Stores property about the detector which is used to rescale and shift
        data in the bank after data have been reduced. The scale attempts to
        take into account that the relative efficiency of different banks may not
        be the same. By default this scale is set to 1.0. The shift is strictly
        speaking more an effect of the geometry of the sample than the detector
        but is included here since both these are required to bring the scale+shift of reduced data
        collected on front and rear bank on the same 'level' before e.g. merging
        such data
        """

        def __init__(self, scale=1.0, shift=0.0, fitScale=False, fitShift=False, qMin=None, qMax=None):
            """
            @param scale: Default to 1.0. Value to multiply data with
            @param shift: Default to 0.0. Value to add to data
            @param fitScale: Default is False. Whether or not to try and fit this param
            @param fitShift: Default is False. Whether or not to try and fit this param
            @param qMin: When set to None (default) then for fitting use the overlapping q region of front and rear detectors
            @param qMax: When set to None (default) then for fitting use the overlapping q region of front and rear detectors
            """
            self.scale = scale
            self.shift = shift
            self.fitScale = bool(fitScale)
            self.fitShift = bool(fitShift)
            self.qMin = qMin
            self.qMax = qMax

            if self.qMin is None or self.qMax is None:
                self.qRangeUserSelected = False
            else:
                self.qRangeUserSelected = True

    _first_spec_num = None
    last_spec_num = None

    def __init__(self, instr, det_type):
        # detectors are known by many names, the 'uni' name is an instrument independent alias the 'long'
        # name is the instrument view name and 'short' name often used for convenience
        self._names = {
            "uni": det_type,
            "long": instr.getStringParameter(det_type + "-detector-name")[0],
            "short": instr.getStringParameter(det_type + "-detector-short-name")[0],
        }
        # the bank is often also referred to by its location, as seen by the sample
        if det_type.startswith("low"):
            position = "rear"
        else:
            position = "front"
        self._names["position"] = position

        cols_data = instr.getNumberParameter(det_type + "-detector-num-columns")
        if len(cols_data) > 0:
            rectanglar_shape = True
            width = int(cols_data[0])
        else:
            rectanglar_shape = False
            width = instr.getNumberParameter(det_type + "-detector-non-rectangle-width")[0]

        rows_data = instr.getNumberParameter(det_type + "-detector-num-rows")
        if len(rows_data) > 0:
            height = int(rows_data[0])
        else:
            rectanglar_shape = False
            height = instr.getNumberParameter(det_type + "-detector-non-rectangle-height")[0]

        n_pixels = None
        n_pixels_override = instr.getNumberParameter(det_type + "-detector-num-pixels")
        if len(n_pixels_override) > 0:
            n_pixels = int(n_pixels_override[0])
        # n_pixels is normally None and calculated by DectShape but LOQ (at least) has a detector with a hole
        self._shape = self._DectShape(width, height, rectanglar_shape, n_pixels)

        spec_entry = instr.getNumberParameter("first-low-angle-spec-number")
        if len(spec_entry) > 0:
            self.set_first_spec_num(int(spec_entry[0]))
        else:
            # 'first-low-angle-spec-number' is an optimal instrument parameter
            self.set_first_spec_num(0)

        # needed for compatibility with SANSReduction and SANSUtily, remove
        self.n_columns = width

        # this can be set to the name of a file with correction factor against wavelength
        self.correction_file = ""
        # this corrections are set by the mask file
        self.z_corr = 0.0
        self.x_corr = 0.0
        self._y_corr = 0.0
        self._rot_corr = 0.0
        # 23/3/12 RKH add 2 more variables
        self._radius_corr = 0.0
        self._side_corr = 0.0
        # 10/03/15 RKH add 2 more, valid for all detectors.  WHY do some of the above have an extra leading
        # underscore?? Seems they are the optional ones sorted below
        self.x_tilt = 0.0
        self.y_tilt = 0.0

        # hold rescale and shift object _RescaleAndShift
        self.rescaleAndShift = self._RescaleAndShift()
        self.mergeRange = self._MergeRange()

        # The orientation is set by default to Horizontal (Note this used to be HorizontalFlipped,
        # probably as part of some hack for specific run numbers of SANS2D)
        self._orientation = "Horizontal"

    def disable_y_and_rot_corrs(self):
        """
        Not all corrections are supported on all detectors
        """
        self._y_corr = None
        self._rot_corr = None
        # 23/3/12 RKH add 2 more variables
        self._radius_corr = None
        self._side_corr = None

    def get_y_corr(self):
        if self._y_corr is not None:
            return self._y_corr
        else:
            raise NotImplementedError("y correction is not used for this detector")

    def set_y_corr(self, value):
        """
        Only set the value if it isn't disabled
        @param value: set y_corr to this value, unless it's disabled
        """
        if self._y_corr is not None:
            self._y_corr = value

    def get_rot_corr(self):
        if self._rot_corr is not None:
            return self._rot_corr
        else:
            raise NotImplementedError("rot correction is not used for this detector")

    def set_rot_corr(self, value):
        """
        Only set the value if it isn't disabled
        @param value: set rot_corr to this value, unless it's disabled
        """
        if self._rot_corr is not None:
            self._rot_corr = value

    # 22/3/12 RKH added two new variables radius_corr, side_corr
    def get_radius_corr(self):
        if self._radius_corr is not None:
            return self._radius_corr
        else:
            raise NotImplementedError("radius correction is not used for this detector")

    def set_radius_corr(self, value):
        """
        Only set the value if it isn't disabled
        @param value: set radius_corr to this value, unless it's disabled
        """
        if self._rot_corr is not None:
            self._radius_corr = value

    def get_side_corr(self):
        if self._side_corr is not None:
            return self._side_corr
        else:
            raise NotImplementedError("side correction is not used for this detector")

    def set_side_corr(self, value):
        """
        Only set the value if it isn't disabled
        @param value: set side_corr to this value, unless it's disabled
        """
        if self._side_corr is not None:
            self._side_corr = value

    y_corr = property(get_y_corr, set_y_corr, None, None)
    rot_corr = property(get_rot_corr, set_rot_corr, None, None)
    # 22/3/12 RKH added 2 new variables
    radius_corr = property(get_radius_corr, set_radius_corr, None, None)
    side_corr = property(get_side_corr, set_side_corr, None, None)

    def get_first_spec_num(self):
        return self._first_spec_num

    def set_first_spec_num(self, value):
        self._first_spec_num = value
        self.last_spec_num = self._first_spec_num + self._shape.n_pixels() - 1

    def place_after(self, previousDet):
        self.set_first_spec_num(previousDet.last_spec_num + 1)

    def name(self, form="long"):
        if form.lower() == "inst_view":
            form = "long"
        if form not in self._names:
            form = "long"

        return self._names[form]

    def isAlias(self, guess):
        """
        Detectors are often referred to by more than one name, check
        if the supplied name is in the list
        @param guess: this name will be searched for in the list
        @return : True if the name was found, otherwise false
        """
        for name in list(self._names.values()):
            if guess.lower() == name.lower():
                return True
        return False

    def spectrum_block(self, ylow, xlow, ydim, xdim):
        """
        Compile a list of spectrum Numbers for rectangular block of size xdim by ydim
        """
        if ydim == "all":
            ydim = self._shape.height()
        if xdim == "all":
            xdim = self._shape.width()
        det_dimension = self._shape.width()
        base = self._first_spec_num

        if not self._shape.isRectangle():
            sanslog.warning("Attempting to block rows or columns in a non-rectangular detector, this is likely to give unexpected results!")

        output = ""
        if self._orientation == "Horizontal":
            start_spec = base + ylow * det_dimension + xlow
            for y in range(0, ydim):
                for x in range(0, xdim):
                    output += str(start_spec + x + (y * det_dimension)) + ","
        elif self._orientation == "Vertical":
            start_spec = base + xlow * det_dimension + ylow
            for x in range(det_dimension - 1, det_dimension - xdim - 1, -1):
                for y in range(0, ydim):
                    std_i = start_spec + y + ((det_dimension - x - 1) * det_dimension)
                    output += str(std_i) + ","
        elif self._orientation == "Rotated":
            # This is the horizontal one rotated so need to map the xlow and vlow to their rotated versions
            start_spec = base + ylow * det_dimension + xlow
            max_spec = det_dimension * det_dimension + base - 1
            for y in range(0, ydim):
                for x in range(0, xdim):
                    std_i = start_spec + x + (y * det_dimension)
                    output += str(max_spec - (std_i - base)) + ","

        return output.rstrip(",")

    # Used to constrain the possible values of the orientation of the detector bank against the direction that spectrum numbers increase in
    _ORIENTED = {"Horizontal": None, "Vertical": None, "Rotated": None}  # most runs have the detectors in this state

    def set_orien(self, orien):
        """
        Sets to relationship between the detectors and the spectra numbers. The relationship
        is given by an orientation string and this function throws if the string is not recognised
        @param orien: the orientation string must be a string contained in the dictionary _ORIENTED
        """
        self._ORIENTED[orien]
        self._orientation = orien

    def crop_to_detector(self, input_name, output_name=None):
        """
        Crops the workspace that is passed so that it only contains the spectra that correspond
        to this detector
        @param input_name: name of the workspace to crop
        @param output_name: name the workspace will take (default is the input name)
        """
        if not output_name:
            output_name = input_name

        try:
            wki = mtd[input_name]
            # Is it really necessary to crop?
            if wki.getNumberHistograms() != self.last_spec_num - self.get_first_spec_num() + 1:
                CropWorkspace(
                    InputWorkspace=input_name,
                    OutputWorkspace=output_name,
                    StartWorkspaceIndex=self.get_first_spec_num() - 1,
                    EndWorkspaceIndex=self.last_spec_num - 1,
                )
        except:
            raise ValueError(
                "Can not find spectra for %s in the workspace %s [%d,%d]\nException:"
                % (self.name(), input_name, self.get_first_spec_num(), self.last_spec_num)
                + str(sys.exc_info())
            )


class ISISInstrument(BaseInstrument):
    lowAngDetSet = None

    def __init__(self, filename=None, m4_instrument_component_name=None):
        """
        Reads the instrument definition xml file
        @param filename: the name of the instrument definition file to read
        @raise IndexError: if any parameters (e.g. 'default-incident-monitor-spectrum') aren't in the xml definition
        """
        super(ISISInstrument, self).__init__(instr_filen=filename)

        self.idf_path = self._definition_file

        # the spectrum with this number is used to normalize the workspace data
        self._incid_monitor = int(self.definition.getNumberParameter("default-incident-monitor-spectrum")[0])
        self.cen_find_step = float(self.definition.getNumberParameter("centre-finder-step-size")[0])
        # see if a second step size is defined. If not set the second value to the first for compatibility
        # logger.warning("Trying to find centre-finder-step-size2")
        try:
            self.cen_find_step2 = float(self.definition.getNumberParameter("centre-finder-step-size2")[0])
        except:
            # logger.warning("Failed to find centre-finder-step-size2")
            self.cen_find_step2 = self.cen_find_step

        try:
            self.beam_centre_scale_factor1 = float(self.definition.getNumberParameter("beam-centre-scale-factor1")[0])
        except:
            logger.information("Setting beam-centre-scale-factor1 to default (1000).")
            self.beam_centre_scale_factor1 = 1000.0

        try:
            self.beam_centre_scale_factor2 = float(self.definition.getNumberParameter("beam-centre-scale-factor2")[0])
        except:
            logger.information("Setting beam-centre-scale-factor1 to default (1000).")
            self.beam_centre_scale_factor2 = 1000.0

        firstDetect = DetectorBank(self.definition, "low-angle")
        # firstDetect.disable_y_and_rot_corrs()
        secondDetect = DetectorBank(self.definition, "high-angle")
        secondDetect.place_after(firstDetect)
        # add det_selection variable that will receive the DET/ REAR/FRONT/BOTH/MERGED
        self.det_selection = "REAR"
        self.DETECTORS = {"low-angle": firstDetect}
        self.DETECTORS["high-angle"] = secondDetect

        self.setDefaultDetector()
        # if this is set InterpolationRebin will be used on the monitor spectrum used to normalize the sample,
        #  useful because wavelength resolution in the monitor spectrum can be course in the range of interest
        self._use_interpol_norm = False
        # remove use_interpol_trans_calc once the beam centre finder has been converted
        self.use_interpol_trans_calc = False

        # the sample will be moved this distance a long the beam axis
        self.SAMPLE_Z_CORR = 0

        # Detector position information for SANS2D
        # why are these not defined in SANS2D
        self.FRONT_DET_RADIUS = 306.0
        self.FRONT_DET_DEFAULT_SD_M = 4.0
        self.FRONT_DET_DEFAULT_X_M = 1.1
        self.REAR_DET_DEFAULT_SD_M = 4.0

        # LOG files for SANS2D will have these encoder readings
        # why are these not defined in SANS2D
        self.FRONT_DET_X = 0.0
        self.FRONT_DET_Z = 0.0
        self.FRONT_DET_ROT = 0.0
        self.REAR_DET_Z = 0.0
        self.REAR_DET_X = 0

        # LOG files for Larmor will have these encoder readings
        # why are these not defined in Larmor
        self.BENCH_ROT = 0.0

        # spectrum number of the monitor used to as the incidient in the transmission calculations
        self.default_trans_spec = int(self.definition.getNumberParameter("default-transmission-monitor-spectrum")[0])
        self.incid_mon_4_trans_calc = self._incid_monitor

        isis = config.getFacility("ISIS")
        # Number of digits in standard file name
        self.run_number_width = isis.instrument(self._NAME).zeroPadding(0)

        # Set a flag if the instrument has an M4 monitor or not
        self.has_m4_monitor = self._has_m4_monitor_in_idf(m4_instrument_component_name)

        # this variable isn't used again and stops the instrument from being deep copied if this instance is deep copied
        self.definition = None

        # remove this function
        self._del_incidient_set = False

        # it is possible to set the TOF regions that is assumed to be background for each monitors
        self._back_ground = {}
        # the default start region, used for any monitors that a specific one wasn't set for
        self._back_start = None
        # default end region
        self._back_end = None
        # the background TOF region for ROI data. Note that either this or a transmission monitor is used.
        self._back_start_ROI = None
        self._back_end_ROI = None
        # if the user moves a monitor to this z coordinate (with MON/LENGTH ...) this will be recorded here.
        # These are overridden lines like TRANS/TRANSPEC=4/SHIFT=-100
        self.monitor_zs = {}
        # Used when new calibration required.
        self._newCalibrationWS = None
        # Centre of beam after a move has been applied,
        self.beam_centre_pos1_after_move = 0.0
        self.beam_centre_pos2_after_move = 0.0

    def get_incident_mon(self):
        """
        @return: the spectrum number of the incident scattering monitor
        """
        return self._incid_monitor

    def set_incident_mon(self, spectrum_number):
        """
        set the incident scattering monitor spectrum number regardless of
        lock
        @param spectrum_number: monitor's sectrum number
        """
        self._incid_monitor = int(spectrum_number)
        self._del_incidient_set = True

    def suggest_incident_mntr(self, spectrum_number):
        """
        remove this function and the data member it uses
        """
        if not self._del_incidient_set:
            self.set_incident_mon(spectrum_number)

    def set_sample_offset(self, value):
        """
        @param value: sample value offset
        """
        self.SAMPLE_Z_CORR = float(value) / 1000.0

    def is_interpolating_norm(self):
        return self._use_interpol_norm

    def set_interpolating_norm(self, on=True):
        """
        This method sets that the monitor spectrum should be interpolated before
        normalisation
        """
        self._use_interpol_norm = on

    def cur_detector(self):
        if self.lowAngDetSet:
            return self.DETECTORS["low-angle"]
        else:
            return self.DETECTORS["high-angle"]

    def get_low_angle_detector(self):
        """Provide a direct way to get the low bank detector.
        This method does not require to pass the name of the detector bank.
        """
        return self.DETECTORS["low-angle"]

    def get_high_angle_detector(self):
        """Provide a direct way to get the high bank detector
        This method does not require to pass the name of the detector bank.
        """
        return self.DETECTORS["high-angle"]

    def other_detector(self):
        if not self.lowAngDetSet:
            return self.DETECTORS["low-angle"]
        else:
            return self.DETECTORS["high-angle"]

    def getDetector(self, requested):
        for _n, detect in self.DETECTORS.items():
            if detect.isAlias(requested):
                return detect
        sanslog.notice("getDetector: Detector " + requested + "not found")

    def listDetectors(self):
        return self.cur_detector().name(), self.other_detector().name()

    def isHighAngleDetector(self, detName):
        if self.DETECTORS["high-angle"].isAlias(detName):
            return True

    def isDetectorName(self, detName):
        if self.other_detector().isAlias(detName):
            return True

        return self.cur_detector().isAlias(detName)

    def setDetector(self, detName):
        self.det_selection = detName
        if self.other_detector().isAlias(detName):
            self.lowAngDetSet = not self.lowAngDetSet
            return True
        elif self.cur_detector().isAlias(detName):
            return True

    def get_detector_selection(self):
        return self.det_selection

    def setDefaultDetector(self):
        self.lowAngDetSet = True

    def copy_correction_files(self):
        """
        Check if one of the efficiency files hasn't been set and assume the other is to be used
        """
        a = self.cur_detector()
        b = self.other_detector()
        if a.correction_file == "" and b.correction_file != "":
            a.correction_file = b.correction_file != ""
        if b.correction_file == "" and a.correction_file != "":
            b.correction_file = a.correction_file != ""

    def detector_file(self, det_name):
        det = self.getDetector(det_name)
        return det.correction_file

    def get_TOFs(self, monitor):
        """
        Gets the start and end time of flights for the region assumed to contain
        only background counts for this instrument
        @param monitor: spectrum number of the monitor's spectrum
        @return: the start time, the end time
        """
        monitor = int(monitor)
        if monitor in self._back_ground:
            return self._back_ground[int(monitor)]["start"], self._back_ground[int(monitor)]["end"]
        else:
            return self._back_start, self._back_end

    def set_TOFs(self, start, end, monitor=None):
        """
        Defines the start and end time of flights for the assumed background region
        for this instrument
        @param: start defines the start of the background region
        @param: end defines the end
        @param monitor: spectrum number of the monitor's spectrum, if none given affect the default
        """
        if start is not None:
            start = float(start)
        if end is not None:
            end = float(end)

        if monitor:
            self._back_ground[int(monitor)] = {"start": start, "end": end}
        else:
            self._back_start = start
            self._back_end = end

    def reset_TOFs(self, monitor=None):
        """
        Reset background region set by set_TOFs
        @param monitor: spectrum number of the monitor's spectrum, if none given affect the default
        """
        if monitor:
            monitor = int(monitor)
            if monitor in self._back_ground:
                del self._back_ground[int(monitor)]
        else:
            self._back_ground = {}
            self._back_start = None
            self._back_end = None
            self.reset_TOFs_for_ROI()

    def get_TOFs_for_ROI(self):
        """
        Gets the TOFs for the ROI which is required for the Transmission calculation. If it is
        not available then use the default setting
        @return: the start time, the end time
        """
        if self._back_start_ROI and self._back_end_ROI:
            return self._back_start_ROI, self._back_end_ROI
        else:
            return None, None

    def set_TOFs_for_ROI(self, start, end):
        """
        Sets the TOFs for the ROI which is required for the Transmission calculation.
        @param: start : defines the start of the background region for ROI
        @param: end : defines the end of the background region for ROI
        """
        if start is not None:
            start = float(start)
        if end is not None:
            end = float(end)
        self._back_start_ROI = start
        self._back_end_ROI = end

    def reset_TOFs_for_ROI(self):
        """
        Reset background region set by set_TOFs for ROI
        """
        self._back_start_ROI = None
        self._back_end_ROI = None

    def move_all_components(self, ws):
        """
        Move the sample object to the location set in the logs or user settings file
        @param ws: the workspace containing the sample to move
        """
        MoveInstrumentComponent(Workspace=ws, ComponentName="some-sample-holder", Z=self.SAMPLE_Z_CORR, RelativePosition=True)

        for i in list(self.monitor_zs.keys()):
            # get the current location
            component = self.monitor_names[i]
            ws = mtd[str(ws)]
            mon = ws.getInstrument().getComponentByName(component)
            z_loc = mon.getPos().getZ()
            # now the relative move
            offset = (self.monitor_zs[i] / 1000.0) - z_loc
            MoveInstrumentComponent(Workspace=ws, ComponentName=component, Z=offset, RelativePosition=True)

    def move_components(self, ws, beamX, beamY):
        """Define how to move the bank to position beamX and beamY must be implemented"""
        raise RuntimeError("Not Implemented")

    def elementary_displacement_of_single_component(
        self, workspace, component_name, coord1, coord2, coord1_scale_factor=1.0, coord2_scale_factor=1.0, relative_displacement=True
    ):
        """
        A simple elementary displacement of a single component.
        This provides the adequate displacement for finding the beam centre.
        @param workspace: the workspace which needs to have the move applied to it
        @param component_name: the name of the component which being displaced
        @param coord1: the first coordinate, which is x here
        @param coord2: the second coordinate, which is y here
        @param coord1_scale_factor: scale factor for the first coordinate
        @param coord2_scale_factor: scale factor for the second coordinate
        @param relative_displacement: If the displacement is to be relative (it normally should be)
        """
        raise RuntimeError("Not Implemented")

    def cur_detector_position(self, ws_name):
        """
        Return the position of the center of the detector bank
        @param ws_name: the input workspace name
        @raise RuntimeError: Not implemented
        """
        raise RuntimeError("Not Implemented")

    def on_load_sample(self, ws_name, beamcentre, isSample):
        """It will be called just after loading the workspace for sample and can

        It configures the instrument for the specific run of the workspace for handle historical changes in the instrument.

        It centralizes the detector bank to the beamcentre (tuple of two values)
        """
        ws_ref = mtd[str(ws_name)]
        try:
            run_num = LARMOR.get_run_number_from_workspace_reference(ws_ref)
        except:
            run_num = int(re.findall(r"\d+", str(ws_name))[0])

        if isSample:
            self.set_up_for_run(run_num)

        if self._newCalibrationWS:
            # We are about to transfer the Instrument Parameter File from the
            # calibration to the original workspace. We want to add new parameters
            # which the calibration file has not yet picked up.
            # IMPORTANT NOTE: This takes the parameter settings from the original workspace
            # if they are old too, then we don't pick up newly added parameters
            self._add_parmeters_absent_in_calibration(ws_name, self._newCalibrationWS)
            self.changeCalibration(ws_name)

        # centralize the bank to the centre
        dummy_centre, centre_shift = self.move_components(ws_name, beamcentre[0], beamcentre[1])
        return centre_shift

    def load_transmission_inst(self, ws_trans, ws_direct, beamcentre):
        """
        Called on loading of transmissions
        """
        pass

    def changeCalibration(self, ws_name):
        calib = mtd[self._newCalibrationWS]
        sanslog.notice("Applying new calibration for the detectors from " + str(calib.name()))
        CopyInstrumentParameters(calib, ws_name)

    def setCalibrationWorkspace(self, ws_reference):
        assert isinstance(ws_reference, Workspace)
        # we do deep copy of singleton - to be removed in 8470
        # this forces us to have 'copyable' objects.
        self._newCalibrationWS = str(ws_reference)

    def get_updated_beam_centre_after_move(self):
        """
        @returns the beam centre position after the instrument has moved
        """
        return self.beam_centre_pos1_after_move, self.beam_centre_pos2_after_move

    def _add_parmeters_absent_in_calibration(self, ws_name, calib_name):
        """
        We load the instrument specific Instrument Parameter File (IPF) and check if
        there are any settings which the calibration workspace does not have. The calibration workspace
        has its own parameter map stored in the nexus file. This means that if we add new
        entries to the IPF, then they are not being picked up. We do not want to change the existing
        values, just add new entries.
        @param ws_name: the name of the main workspace with the data
        @param calibration_workspace: the name of the calibration workspace
        """
        if calib_name is None or ws_name is None:
            return
        workspace = mtd[ws_name]
        calibration_workspace = mtd[calib_name]

        # 1.Iterate over all parameters in the original workspace
        # 2. Compare with the calibration workspace
        # 3. If it does not exist, then add it
        original_parmeters = workspace.getInstrument().getParameterNames()
        for param in original_parmeters:
            if not calibration_workspace.getInstrument().hasParameter(param):
                self._add_new_parameter_to_calibration(param, workspace, calibration_workspace)

    def _add_new_parameter_to_calibration(self, param_name, workspace, calibration_workspace):
        """
        Adds the missing value from the Instrument Parameter File (IPF) of the workspace to
        the IPF of the calibration workspace. We check for the
                                                             1. Type
                                                             2. Value
                                                             3. Name
                                                             4. ComponentName (which is the instrument)
        @param param_name: the name of the parameter to add
        @param workspace: the donor of the parameter
        @param calibration_workspace: the receiver of the parameter
        """
        ws_instrument = workspace.getInstrument()
        component_name = ws_instrument.getName()
        ipf_type = ws_instrument.getParameterType(param_name)
        # For now we only expect string, int and double
        type_ids = ["string", "int", "double"]
        value = None
        type_to_save = "Number"
        if ipf_type == type_ids[0]:
            value = ws_instrument.getStringParameter(param_name)
            type_to_save = "String"
        elif ipf_type == type_ids[1]:
            value = ws_instrument.getIntParameter(param_name)
        elif ipf_type == type_ids[2]:
            value = ws_instrument.getNumberParameter(param_name)
        else:
            raise RuntimeError(
                "ISISInstrument: An Instrument Parameter File value of unknown type" "is trying to be copied. Cannot handle this currently."
            )
        SetInstrumentParameter(
            Workspace=calibration_workspace,
            ComponentName=component_name,
            ParameterName=param_name,
            ParameterType=type_to_save,
            Value=str(value[0]),
        )

    def get_m4_monitor_det_ID(self):
        """
        Gets the detecor ID associated with Monitor 4
        @returns: the det ID of Monitor 4
        """
        raise RuntimeError("Monitor 4 does not seem to be implemented.")

    def _has_m4_monitor_in_idf(self, m4_name):
        """
        Checks if the instrument contains a component with the M4 name
        @param m4_name: the name of the M4 component
        @returns true if it has an M4 component, else false
        """
        return False if self.definition.getComponentByName(m4_name) is None else True


class LOQ(ISISInstrument):
    """
    Contains all the LOQ specific data and functions
    """

    _NAME = "LOQ"
    # minimum wavelength of neutrons assumed to be measurable by this instrument
    WAV_RANGE_MIN = 2.2
    # maximum wavelength of neutrons assumed to be measurable by this instrument
    WAV_RANGE_MAX = 10.0

    def __init__(self, idf_path="LOQ_Definition_20020226-.xml"):
        """
        Reads LOQ's instrument definition xml file
        @param idf_path: the idf file
        @raise IndexError: if any parameters (e.g. 'default-incident-monitor-spectrum') aren't in the xml definition
        """
        # The det id for the M4 monitor in LOQ
        self._m4_det_id = 17788
        self._m4_monitor_name = "monitor4"
        super(LOQ, self).__init__(idf_path, self._m4_monitor_name)
        # relates the numbers of the monitors to their names in the instrument definition file
        self.monitor_names = {1: "monitor1", 2: "monitor2"}

        if self.has_m4_monitor:
            self.monitor_names.update({self._m4_det_id: self._m4_monitor_name})
        elif self._m4_det_id in list(self.monitor_names.keys()):
            del self.monitor_names[self._m4_det_id]

    def on_load_sample(self, ws_name, beamcentre, isSample, other_centre=None):
        """It will be called just after loading the workspace for sample and can

        It configures the instrument for the specific run of the workspace for handle historical changes in the instrument.

        It centralizes the detector bank to the beamcentre (tuple of two values)
        """
        ws_ref = mtd[str(ws_name)]
        try:
            run_num = LARMOR.get_run_number_from_workspace_reference(ws_ref)
        except:
            run_num = int(re.findall(r"\d+", str(ws_name))[0])

        if isSample:
            self.set_up_for_run(run_num)

        if self._newCalibrationWS:
            # We are about to transfer the Instrument Parameter File from the
            # calibration to the original workspace. We want to add new parameters
            # which the calibration file has not yet picked up.
            # IMPORTANT NOTE: This takes the parameter settings from the original workspace
            # if they are old too, then we don't pick up newly added parameters
            self._add_parmeters_absent_in_calibration(ws_name, self._newCalibrationWS)
            self.changeCalibration(ws_name)

        # centralize the bank to the centre
        if other_centre:
            dummy_centre, centre_shift = self.move_components(
                ws_name, beamcentre[0], beamcentre[1], xbeam_other=other_centre[0], ybeam_other=other_centre[1]
            )
        else:
            dummy_centre, centre_shift = self.move_components(ws_name, beamcentre[0], beamcentre[1])
        return centre_shift

    def move_components(self, ws, xbeam, ybeam, xbeam_other=None, ybeam_other=None):
        """
        Move the locations of the sample and detector bank based on the passed beam center
        and information from the sample workspace logs
        @param ws: workspace containing the instrument information
        @param xbeam: x-position of the beam
        @param ybeam: y-position of the beam
        @return: the locations of (in the new coordinates) beam center, center of detector bank
        """
        self.move_all_components(ws)

        xshift = (317.5 / 1000.0) - xbeam
        yshift = (317.5 / 1000.0) - ybeam
        MoveInstrumentComponent(Workspace=ws, ComponentName=self.cur_detector().name(), X=xshift, Y=yshift, RelativePosition="1")
        if ybeam_other and xbeam_other:
            xshift_other = (317.5 / 1000.0) - xbeam_other
            yshift_other = (317.5 / 1000.0) - ybeam_other
            MoveInstrumentComponent(
                Workspace=ws, ComponentName=self.other_detector().name(), X=xshift_other, Y=yshift_other, RelativePosition="1"
            )
        # Have a separate move for x_corr, y_coor and z_coor just to make it more obvious in the
        # history, and to expert users what is going on
        det = self.cur_detector()
        det_other = self.other_detector()
        if det.x_corr != 0.0 or det.y_corr != 0.0 or det.z_corr != 0.0:
            MoveInstrumentComponent(
                Workspace=ws,
                ComponentName=det.name(),
                X=det.x_corr / 1000.0,
                Y=det.y_corr / 1000.0,
                Z=det.z_corr / 1000.0,
                RelativePosition="1",
            )
            if ybeam_other and xbeam_other:
                MoveInstrumentComponent(
                    Workspace=ws,
                    ComponentName=det_other.name(),
                    X=det_other.x_corr / 1000.0,
                    Y=det_other.y_corr / 1000.0,
                    Z=det_other.z_corr / 1000.0,
                    RelativePosition="1",
                )
            xshift = xshift + det.x_corr / 1000.0
            yshift = yshift + det.y_corr / 1000.0

        # Set the beam centre position afte the move, leave as they were
        self.beam_centre_pos1_after_move = xbeam
        self.beam_centre_pos2_after_move = ybeam

        return [xshift, yshift], [xshift, yshift]

    def elementary_displacement_of_single_component(
        self, workspace, component_name, coord1, coord2, coord1_scale_factor=1.0, coord2_scale_factor=1.0, relative_displacement=True
    ):
        """
        A simple elementary displacement of a single component.
        This provides the adequate displacement for finding the beam centre.
        @param workspace: the workspace which needs to have the move applied to it
        @param component_name: the name of the component which being displaced
        @param coord1: the first coordinate, which is x here
        @param coord2: the second coordinate, which is y here
        @param coord1_scale_factor: scale factor for the first coordinate
        @param coord2_scale_factor: scale factor for the second coordinate
        @param relative_displacement: If the displacement is to be relative (it normally should be)
        """
        MoveInstrumentComponent(
            Workspace=workspace, ComponentName=component_name, X=coord1, Y=coord2, RelativePosition=relative_displacement
        )

    def get_marked_dets(self):
        raise NotImplementedError("The marked detector list isn't stored for instrument " + self._NAME)

    def set_up_for_run(self, base_runno):
        """
        Needs to run whenever a sample is loaded
        """
        first = self.DETECTORS["low-angle"]
        second = self.DETECTORS["high-angle"]

        first.set_orien("Horizontal")
        # probably _first_spec_num was already set to this when the instrument parameter file was loaded
        first.set_first_spec_num(3)
        second.set_orien("Horizontal")
        second.place_after(first)

    def load_transmission_inst(self, ws_trans, ws_direct, beamcentre):
        """
        Loads information about the setup used for LOQ transmission runs
        """
        ws = mtd[ws_trans]
        instrument = ws.getInstrument()
        has_m4 = instrument.getComponentByName(self._m4_monitor_name)
        if has_m4 is None:
            trans_definition_file = os.path.join(config.getString("instrumentDefinition.directory"), self._NAME + "_trans_Definition.xml")
        else:
            trans_definition_file = os.path.join(
                config.getString("instrumentDefinition.directory"), self._NAME + "_trans_Definition_M4.xml"
            )
        LoadInstrument(Workspace=ws_trans, Filename=trans_definition_file, RewriteSpectraMap=False)
        LoadInstrument(Workspace=ws_direct, Filename=trans_definition_file, RewriteSpectraMap=False)

    def cur_detector_position(self, ws_name):
        """Return the position of the center of the detector bank"""
        ws = mtd[ws_name]
        pos = ws.getInstrument().getComponentByName(self.cur_detector().name()).getPos()
        cent_pos = 317.5 / 1000.0
        return [cent_pos - pos.getX(), cent_pos - pos.getY()]

    def get_m4_monitor_det_ID(self):
        return self._m4_det_id


class SANS2D(ISISInstrument):
    """
    The SANS2D instrument has movable detectors whose locations have to
    be read in from the workspace logs (Run object)
    """

    _NAME = "SANS2D"
    WAV_RANGE_MIN = 2.0
    WAV_RANGE_MAX = 14.0

    def __init__(self, idf_path=None):
        # The detector ID for the M4 monitor
        self._m4_det_id = 4
        self._m4_monitor_name = "monitor4"
        super(SANS2D, self).__init__(idf_path, self._m4_monitor_name)

        self._marked_dets = []
        # set to true once the detector positions have been moved to the locations given in the sample logs
        self.corrections_applied = False
        # a warning is issued if the can logs are not the same as the sample
        self._can_logs = {}
        # The user can set the distance between monitor 4 and the rear detector in millimetres, should be negative
        self.monitor_4_offset = None
        # relates the numbers of the monitors to their names in the instrument definition file
        self.monitor_names = {1: "monitor1", 2: "monitor2", 3: "monitor3", self._m4_det_id: self._m4_monitor_name}

    def set_up_for_run(self, base_runno):
        """
        Handles changes required when a sample is loaded, both generic
        and run specific
        """
        first = self.DETECTORS["low-angle"]
        second = self.DETECTORS["high-angle"]

        try:
            base_runno = int(base_runno)
            # first deal with some special cases
            if base_runno < 568:
                self.set_incident_mon(73730)
                first.set_first_spec_num(1)
                first.set_orien("Vertical")
                second.set_orien("Vertical")
            elif base_runno >= 568 and base_runno < 684:
                first.set_first_spec_num(9)
                first.set_orien("Rotated")
                second.set_orien("Rotated")
            else:
                # this is the default case
                first.set_first_spec_num(9)
                first.set_orien("Horizontal")
                second.set_orien("Horizontal")
        except ValueError:
            # this is the default case
            first.set_first_spec_num(9)
            first.set_orien("Horizontal")
            second.set_orien("Horizontal")

        # as spectrum numbers of the first detector have changed we'll move those in the second too
        second.place_after(first)

    def getDetValues(self, ws_name):
        """
        Retrieve the values of Front_Det_Z, Front_Det_X, Front_Det_Rot, Rear_Det_Z and Rear_Det_X from
        the workspace. If it does not find the value at the run info, it takes as default value the
        self.FRONT_DET_Z, self... which are extracted from the sample workspace at apply_detector_log.

        This is done to allow the function move_components to use the correct values and not to use
        all the values for TRANS ans SAMPLE the same, as sometimes, this assumption is not valid.

        The reason for this method is explained at the ticket http://trac.mantidproject.org/mantid/ticket/7314.
        """
        # set the default value for these variables
        values = [self.FRONT_DET_Z, self.FRONT_DET_X, self.FRONT_DET_ROT, self.REAR_DET_Z, self.REAR_DET_X]
        # get these variables from the workspace run
        run_info = mtd[str(ws_name)].run()
        ind = 0
        for name in ("Front_Det_Z", "Front_Det_X", "Front_Det_Rot", "Rear_Det_Z", "Rear_Det_X"):
            try:
                var = run_info.get(name).value
                if hasattr(var, "__iter__"):
                    var = var[-1]
                values[ind] = float(var)
            except:
                pass  # ignore, because we do have a default value
            ind += 1
        # return these variables
        return tuple(values)

    def move_components(self, ws, xbeam, ybeam):
        """
        Move the locations of the sample and detector bank based on the passed beam center
        and information from the sample workspace logs. If the location of the monitor was
        set with TRANS/TRANSPEC=4/SHIFT=... this function does the move instrument
        @param ws: workspace containing the instrument information
        @param xbeam: x-position of the beam in meters
        @param ybeam: y-position of the beam in meters
        @return: the locations of (in the new coordinates) beam center, center of detector bank
        """
        frontDet = self.getDetector("front")
        rearDet = self.getDetector("rear")

        FRONT_DET_Z, FRONT_DET_X, FRONT_DET_ROT, REAR_DET_Z, REAR_DET_X = self.getDetValues(ws)

        # Deal with front detector
        # 10/03/15 RKH need to add tilt of detector, in degrees, with respect to the horizontal or vertical of the detector plane
        # this time we can rotate about the detector's own axis so can use RotateInstrumentComponent,
        # ytilt rotates about x axis, xtilt rotates about z axis
        #
        if frontDet.y_tilt != 0.0:
            RotateInstrumentComponent(
                Workspace=ws, ComponentName=self.getDetector("front").name(), X="1.", Y="0.", Z="0.", Angle=frontDet.y_tilt
            )
        if frontDet.x_tilt != 0.0:
            RotateInstrumentComponent(
                Workspace=ws, ComponentName=self.getDetector("front").name(), X="0.", Y="0.", Z="1.", Angle=frontDet.x_tilt
            )
        #
        # 9/1/12  this all dates to Richard Heenan & Russell Taylor's original python development for SANS2d
        # the rotation axis on the SANS2d front detector is actually set front_det_radius = 306mm behind the detector.
        # Since RotateInstrumentComponent will only rotate about the centre of the detector, we have to to the rest here.
        # rotate front detector according to value in log file and correction value provided in user file
        rotateDet = -FRONT_DET_ROT - frontDet.rot_corr
        RotateInstrumentComponent(Workspace=ws, ComponentName=self.getDetector("front").name(), X="0.", Y="1.0", Z="0.", Angle=rotateDet)
        RotRadians = math.pi * (FRONT_DET_ROT + frontDet.rot_corr) / 180.0
        # The rear detector is translated to the beam position using the beam centre coordinates in the user file.
        # (Note that the X encoder values in NOT used for the rear detector.)
        # The front detector is translated using the difference in X encoder values, with a correction from the user file.
        # 21/3/12 RKH [In reality only the DIFFERENCE in X encoders is used, having separate X corrections for
        # both detectors is unnecessary,
        # but we will continue with this as it makes the mask file smore logical and avoids a retrospective change.]
        # 21/3/12 RKH add .side_corr    allows rotation axis of the front detector being offset from the detector X=0
        # this inserts *(1.0-math.cos(RotRadians)) into xshift, and
        # - frontDet.side_corr*math.sin(RotRadians) into zshift.
        # (Note we may yet need to introduce further corrections for parallax errors in the detectors, which may be wavelength dependent!)
        xshift = (
            (
                REAR_DET_X
                + rearDet.x_corr
                - frontDet.x_corr
                - FRONT_DET_X
                - frontDet.side_corr * (1 - math.cos(RotRadians))
                + (self.FRONT_DET_RADIUS + frontDet.radius_corr) * math.sin(RotRadians)
            )
            / 1000.0
            - self.FRONT_DET_DEFAULT_X_M
            - xbeam
        )
        yshift = frontDet.y_corr / 1000.0 - ybeam
        # Note don't understand the comment below (9/1/12 these are comments from the original python code,
        # you may remove them if you like!)
        # default in instrument description is 23.281m - 4.000m from sample at 19,281m !
        # need to add ~58mm to det1 to get to centre of detector, before it is rotated.
        # 21/3/12 RKH add .radius_corr
        zshift = (
            FRONT_DET_Z
            + frontDet.z_corr
            + (self.FRONT_DET_RADIUS + frontDet.radius_corr) * (1 - math.cos(RotRadians))
            - frontDet.side_corr * math.sin(RotRadians)
        ) / 1000.0
        zshift -= self.FRONT_DET_DEFAULT_SD_M
        MoveInstrumentComponent(
            Workspace=ws, ComponentName=self.getDetector("front").name(), X=xshift, Y=yshift, Z=zshift, RelativePosition="1"
        )

        # deal with rear detector

        # 10/03/15 RKH need to add tilt of detector, in degrees, with respect to the horizontal or vertical of the detector plane
        # Best to do the tilts first, while the detector is still centred on the z axis,
        # ytilt rotates about x axis, xtilt rotates about z axis
        # NOTE the beam centre coordinates may change
        if rearDet.y_tilt != 0.0:
            RotateInstrumentComponent(Workspace=ws, ComponentName=rearDet.name(), X="1.", Y="0.", Z="0.", Angle=rearDet.y_tilt)
        if rearDet.x_tilt != 0.0:
            RotateInstrumentComponent(Workspace=ws, ComponentName=rearDet.name(), X="0.", Y="0.", Z="1.", Angle=rearDet.x_tilt)

        xshift = -xbeam
        yshift = -ybeam
        zshift = (REAR_DET_Z + rearDet.z_corr) / 1000.0
        zshift -= self.REAR_DET_DEFAULT_SD_M
        sanslog.notice("Setup move " + str(xshift * 1000.0) + " " + str(yshift * 1000.0) + " " + str(zshift * 1000.0))
        MoveInstrumentComponent(Workspace=ws, ComponentName=rearDet.name(), X=xshift, Y=yshift, Z=zshift, RelativePosition="1")

        self.move_all_components(ws)
        # this implements the TRANS/TRANSPEC=4/SHIFT=... line, this overrides any other monitor move
        if self.monitor_4_offset:
            # get the current location of the monitor
            component = "monitor4"
            ws = mtd[str(ws)]
            mon = ws.getInstrument().getComponentByName(component)
            z_orig = mon.getPos().getZ()

            # the location is relative to the rear-detector, get its location
            det = ws.getInstrument().getComponentByName(self.cur_detector().name())
            det_z = det.getPos().getZ()

            monitor_4_offset = self.monitor_4_offset / 1000.0
            z_new = det_z + monitor_4_offset
            z_move = z_new - z_orig
            MoveInstrumentComponent(Workspace=ws, ComponentName=component, Z=z_move, RelativePosition=True)
            sanslog.notice("Monitor 4 is at z = " + str(z_new))

        # Are these returned values used anywhere?
        if self.cur_detector().name() == "front-detector":
            beam_cen = [0.0, 0.0]
            det_cen = [0.0, 0.0]
        else:
            beam_cen = [0.0, 0.0]
            det_cen = [-xbeam, -ybeam]

        # Set the beam centre position afte the move, leave as they were
        self.beam_centre_pos1_after_move = xbeam
        self.beam_centre_pos2_after_move = ybeam

        return beam_cen, det_cen

    def elementary_displacement_of_single_component(
        self, workspace, component_name, coord1, coord2, coord1_scale_factor=1.0, coord2_scale_factor=1.0, relative_displacement=True
    ):
        """
        A simple elementary displacement of a single component.
        This provides the adequate displacement for finding the beam centre.
        @param workspace: the workspace which needs to have the move applied to it
        @param component_name: the name of the component which being displaced
        @param coord1: the first coordinate, which is x here
        @param coord2: the second coordinate, which is y here
        @param coord1_scale_factor: scale factor for the first coordinate
        @param coord2_scale_factor: scale factor for the second coordinate
        @param relative_displacement: If the displacement is to be relative (it normally should be)
        """
        MoveInstrumentComponent(
            Workspace=workspace, ComponentName=component_name, X=coord1, Y=coord2, RelativePosition=relative_displacement
        )

    def get_detector_log(self, wksp):
        """
        Reads information about the state of the instrument on the information
        stored in the sample
        @param logs: a workspace pointer
        @return the values that were read as a dictionary
        """
        self._marked_dets = []
        wksp = su.getWorkspaceReference(wksp)
        # assume complete log information is stored in the first entry, it isn't stored in the group workspace itself
        if isinstance(wksp, WorkspaceGroup):
            wksp = wksp[0]

        samp = wksp.getRun()

        logvalues = {}
        logvalues["Front_Det_Z"] = self._get_const_num(samp, "Front_Det_Z")
        logvalues["Front_Det_X"] = self._get_const_num(samp, "Front_Det_X")
        logvalues["Front_Det_Rot"] = self._get_const_num(samp, "Front_Det_Rot")
        logvalues["Rear_Det_Z"] = self._get_const_num(samp, "Rear_Det_Z")
        logvalues["Rear_Det_X"] = self._get_const_num(samp, "Rear_Det_X")

        return logvalues

    def _get_const_num(self, log_data, log_name):
        """
        Get a the named entry from the log object. If the entry is a
        time series it's assumed to contain unchanging data and the first
        value is used. The answer must be convertible to float otherwise
        this throws.
        @param log_data: the sample object from a workspace
        @param log_name: a string with the name of the individual entry to load
        @return: the floating point number
        @raise TypeError: if that log entry can't be converted to a float
        """
        try:
            # return the log value if it stored as a single number
            return float(log_data.getLogData(log_name).value)
        except TypeError:

            def format_date(date_string, format, date_str_len):
                if len(date_string) > date_str_len:
                    date_string = date_string[:date_str_len]
                from datetime import datetime

                return datetime.strptime(date_string, format)

            # if the value was stored as a time series we have an array here
            property = log_data.getLogData(log_name)

            size = len(property.value)
            if size == 1:
                return float(log_data.getLogData(log_name).value[0])

            start = log_data.getLogData("run_start")
            dt_0 = format_date(start.value, "%Y-%m-%dT%H:%M:%S", 19)
            for i in range(0, size):
                dt = format_date(str(property.times[i]), "%Y-%m-%dT%H:%M:%S", 19)
                if dt > dt_0:
                    if i == 0:
                        return float(log_data.getLogData(log_name).value[0])
                    else:
                        return float(log_data.getLogData(log_name).value[i - 1])

            # this gets executed if all entries is before the start-time
            return float(log_data.getLogData(log_name).value[size - 1])

    def apply_detector_logs(self, logvalues):
        # apply the corrections that came from the logs
        self.FRONT_DET_Z = float(logvalues["Front_Det_Z"])
        self.FRONT_DET_X = float(logvalues["Front_Det_X"])
        self.FRONT_DET_ROT = float(logvalues["Front_Det_Rot"])
        self.REAR_DET_Z = float(logvalues["Rear_Det_Z"])
        self.REAR_DET_X = float(logvalues["Rear_Det_X"])
        self.corrections_applied = True
        if len(self._can_logs) > 0:
            self.check_can_logs(self._can_logs)

    def check_can_logs(self, new_logs):
        """
        Tests if applying the corrections from the passed logvalues
        would give the same result as the corrections that were
        already made
        @param new_logs: the new values to check are equivalent
        @return: True if the are the same False if not
        """
        if not self.corrections_applied:
            # the check needs to wait until there's something to compare against
            self._can_logs = new_logs

        if len(new_logs) == 0:
            return False

        existing_values = []
        existing_values.append(self.FRONT_DET_Z)
        existing_values.append(self.FRONT_DET_X)
        existing_values.append(self.FRONT_DET_ROT)
        existing_values.append(self.REAR_DET_Z)
        existing_values.append(self.REAR_DET_X)

        new_values = []
        new_values.append(float(new_logs["Front_Det_Z"]))
        new_values.append(float(new_logs["Front_Det_X"]))
        new_values.append(float(new_logs["Front_Det_Rot"]))
        new_values.append(float(new_logs["Rear_Det_Z"]))
        new_values.append(float(new_logs["Rear_Det_X"]))

        errors = 0
        corr_names = ["Front_Det_Z", "Front_Det_X", "Front_Det_Rot", "Rear_Det_Z", "Rear_Det_X"]
        for i in range(0, len(existing_values)):
            if math.fabs(existing_values[i] - new_values[i]) > 5e-04:
                sanslog.warning(
                    "values differ between sample and can runs: Sample "
                    + corr_names[i]
                    + " = "
                    + str(existing_values[i])
                    + ", can value is "
                    + str(new_values[i])
                )
                errors += 1

                self.append_marked(corr_names[i])

        # the check has been done clear up
        self._can_logs = {}

        return errors == 0

    def append_marked(self, detNames):
        self._marked_dets.append(detNames)

    def get_marked_dets(self):
        return self._marked_dets

    def load_transmission_inst(self, ws_trans, ws_direct, beamcentre):
        """
        SANS2D requires the centralize the detectors of the transmission
        as well as the sample and can.
        """
        self.move_components(ws_trans, beamcentre[0], beamcentre[1])
        if ws_trans != ws_direct:
            self.move_components(ws_direct, beamcentre[0], beamcentre[1])

    def cur_detector_position(self, ws_name):
        """Return the position of the center of the detector bank"""
        ws = mtd[ws_name]
        pos = ws.getInstrument().getComponentByName(self.cur_detector().name()).getPos()

        return [-pos.getX(), -pos.getY()]

    def on_load_sample(self, ws_name, beamcentre, isSample):
        """For SANS2D in addition of the operations defines in on_load_sample of ISISInstrument
        it has to deal with the log, which defines some offsets for the movement of the
        detector bank.
        """
        ws_ref = mtd[str(ws_name)]
        try:
            log = self.get_detector_log(ws_ref)
            if log == "":
                raise RuntimeError("Invalid log")
        except:
            if isSample:
                raise RuntimeError("Sample logs cannot be loaded, cannot continue")
            else:
                logger.warning("Can logs could not be loaded, using sample values.")

        if isSample:
            self.apply_detector_logs(log)
        else:
            self.check_can_logs(log)

        ISISInstrument.on_load_sample(self, ws_name, beamcentre, isSample)

    def get_m4_monitor_det_ID(self):
        return self._m4_det_id


class LARMOR(ISISInstrument):
    _NAME = "LARMOR"
    WAV_RANGE_MIN = 0.5
    WAV_RANGE_MAX = 13.5

    def __init__(self, idf_path=None):
        # The detector ID for the M4 monitor
        self._m4_det_id = 4
        self._m4_monitor_name = "monitor4"
        super(LARMOR, self).__init__(idf_path, self._m4_monitor_name)
        self._marked_dets = []
        # set to true once the detector positions have been moved to the locations given in the sample logs
        self.corrections_applied = False
        # a warning is issued if the can logs are not the same as the sample
        self._can_logs = {}

        self.monitor_names = dict()

        for i in range(1, 6):
            self.monitor_names[i] = "monitor" + str(i)

    def set_up_for_run(self, base_runno):
        """
        Needs to run whenever a sample is loaded
        """
        first = self.DETECTORS["low-angle"]
        second = self.DETECTORS["high-angle"]

        first.set_orien("Horizontal")
        first.set_first_spec_num(11)
        second.set_orien("Horizontal")
        second.place_after(first)

    def getDetValues(self, ws_name):
        """
        Retrieve the values of Bench_Rot from the workspace. If it does not find the value at the run info,
        it takes as default value the self.BENCH_ROT, which are extracted from the sample workspace
        at apply_detector_log.
        This is done to allow the function move_components to use the correct values and not to use
        all the values for TRANS ans SAMPLE the same, as sometimes, this assumption is not valid.
        The reason for this method is explained at the ticket http://trac.mantidproject.org/mantid/ticket/7314.
        """
        # set the default value for these variables
        values = [self.BENCH_ROT]
        # get these variables from the workspace run
        run_info = mtd[str(ws_name)].run()
        ind = 0
        name = "Bench_Rot"
        try:
            var = run_info.get(name).value
            if hasattr(var, "__iter__"):
                var = var[-1]
            values[ind] = float(var)
        except:
            pass  # ignore, because we do have a default value
        ind += 1
        # return these variables
        return tuple(values)

    def get_detector_log(self, wksp):
        """
        Reads information about the state of the instrument on the information
        stored in the sample
        @param logs: a workspace pointer
        @return the values that were read as a dictionary
        """
        # logger.warning("Entering get_detector_log")
        self._marked_dets = []
        wksp = su.getWorkspaceReference(wksp)
        # assume complete log information is stored in the first entry, it isn't stored in the group workspace itself
        if isinstance(wksp, WorkspaceGroup):
            wksp = wksp[0]

        samp = wksp.getRun()

        logvalues = {}
        logvalues["Bench_Rot"] = self._get_const_num(samp, "Bench_Rot")
        # logger.warning(str(logvalues))

        return logvalues

    def _get_const_num(self, log_data, log_name):
        """
        Get a the named entry from the log object. If the entry is a
        time series it's assumed to contain unchanging data and the first
        value is used. The answer must be convertible to float otherwise
        this throws.
        @param log_data: the sample object from a workspace
        @param log_name: a string with the name of the individual entry to load
        @return: the floating point number
        @raise TypeError: if that log entry can't be converted to a float
        """
        try:
            # return the log value if it stored as a single number
            return float(log_data.getLogData(log_name).value)
        except TypeError:

            def format_date(date_string, format, date_str_len):
                if len(date_string) > date_str_len:
                    date_string = date_string[:date_str_len]
                from datetime import datetime

                return datetime.strptime(date_string, format)

            # if the value was stored as a time series we have an array here
            property = log_data.getLogData(log_name)

            size = len(property.value)
            if size == 1:
                return float(log_data.getLogData(log_name).value[0])

            start = log_data.getLogData("run_start")
            dt_0 = format_date(start.value, "%Y-%m-%dT%H:%M:%S", 19)
            property_times = property.times
            for i in range(0, size):
                dt = format_date(str(property_times[i]), "%Y-%m-%dT%H:%M:%S", 19)
                if dt > dt_0:
                    if i == 0:
                        return float(log_data.getLogData(log_name).value[0])
                    else:
                        return float(log_data.getLogData(log_name).value[i - 1])

            # this gets executed if all entries is before the start-time
            return float(log_data.getLogData(log_name).value[size - 1])

    def apply_detector_logs(self, logvalues):
        # apply the corrections that came from the logs
        self.BENCH_ROT = float(logvalues["Bench_Rot"])
        self.corrections_applied = True
        if len(self._can_logs) > 0:
            self.check_can_logs(self._can_logs)

    def check_can_logs(self, new_logs):
        """
        Tests if applying the corrections from the passed logvalues
        would give the same result as the corrections that were
        already made
        @param new_logs: the new values to check are equivalent
        @return: True if the are the same False if not
        """
        # logger.warning("Entering check_can_logs")

        if not self.corrections_applied:
            # the check needs to wait until there's something to compare against
            self._can_logs = new_logs

        if len(new_logs) == 0:
            return False

        existing_values = []
        existing_values.append(self.BENCH_ROT)

        new_values = []
        new_values.append(float(new_logs["Bench_Rot"]))

        errors = 0
        corr_names = ["Bench_Rot"]
        for i in range(0, len(existing_values)):
            if math.fabs(existing_values[i] - new_values[i]) > 5e-04:
                sanslog.warning(
                    "values differ between sample and can runs: Sample "
                    + corr_names[i]
                    + " = "
                    + str(existing_values[i])
                    + ", can value is "
                    + str(new_values[i])
                )
                errors += 1

                self.append_marked(corr_names[i])

        # the check has been done clear up
        self._can_logs = {}

        return errors == 0

    def move_components(self, ws, xbeam, ybeam):
        # logger.warning("Entering move_components")
        self.move_all_components(ws)
        # logger.warning("Back from move_all_components")

        detBench = self.getDetector("rear")

        # get the bench rotation value from the instrument log
        BENCH_ROT = self.getDetValues(ws)[0]

        # use the scale factors from the parameter file to scale appropriately
        XSF = self.beam_centre_scale_factor1
        YSF = self.beam_centre_scale_factor2

        # in this case the x shift is actually a value of 2theta rotated about the sample stack centre
        # so... we need to do two moves first a shift in y and then a rotation
        yshift = -ybeam
        # zshift = ( detBanch.z_corr)/1000.
        # zshift -= self.REAR_DET_DEFAULT_SD_M
        xshift = 0
        zshift = 0
        sanslog.notice("Setup move " + str(xshift * XSF) + " " + str(yshift * YSF) + " " + str(zshift * 1000))
        MoveInstrumentComponent(ws, ComponentName=detBench.name(), X=xshift, Y=yshift, Z=zshift)

        # Deal with the angle value
        self._rotate_around_y_axis(workspace=ws, component_name=detBench.name(), x_beam=xbeam, x_scale_factor=XSF, bench_rotation=BENCH_ROT)

        # Set the beam centre position afte the move
        self.beam_centre_pos1_after_move = xbeam  # Need to provide the angle in 1000th of a degree
        self.beam_centre_pos2_after_move = ybeam

        # beam centre, translation, new beam position
        return [0.0, 0.0], [-xbeam, -ybeam]

    def elementary_displacement_of_single_component(
        self, workspace, component_name, coord1, coord2, coord1_scale_factor=1.0, coord2_scale_factor=1.0, relative_displacement=True
    ):
        """
        A simple elementary displacement of a single component.
        This provides the adequate displacement for finding the beam centre.
        @param workspace: the workspace which needs to have the move applied to it
        @param component_name: the name of the component which being displaced
        @param coord1: the first coordinate, which is x here
        @param coord2: the second coordinate, which is y here
        @param coord1_scale_factor: scale factor for the first coordinate
        @param coord2_scale_factor: scale factor for the second coordinate
        @param relative_displacement: If the displacement is to be relative (it normally should be)
        """
        # Shift the component in the y direction
        MoveInstrumentComponent(Workspace=workspace, ComponentName=component_name, Y=coord2, RelativePosition=relative_displacement)

        # Rotate around the y-axis.
        self._rotate_around_y_axis(
            workspace=workspace, component_name=component_name, x_beam=coord1, x_scale_factor=coord1_scale_factor, bench_rotation=0.0
        )

    def _rotate_around_y_axis(self, workspace, component_name, x_beam, x_scale_factor, bench_rotation):
        """
        Rotates the component of the workspace around the y axis or shift along x, depending on the run number
        @param workspace: a workspace name
        @param component_name: the component to rotate
        @param x_beam: either a shift in mm or a angle in degree
        @param x_scale_factor:
        """
        # in order to avoid rewriting old mask files from initial commissioning during 2014.
        ws_ref = mtd[workspace]

        # The angle value
        # Note that the x position gets converted from mm to m when read from the user file so we need to reverse this if X is now an angle
        if not LARMOR.is_run_new_style_run(ws_ref):
            # Initial commissioning before run 2217 did not pay much attention to making sure the bench_rot value was meaningful
            xshift = -x_beam
            sanslog.notice("Setup move " + str(xshift * x_scale_factor) + " " + str(0.0) + " " + str(0.0))
            MoveInstrumentComponent(workspace, ComponentName=component_name, X=xshift, Y=0.0, Z=0.0)
        else:
            # The x shift is in degree
            # IMPORTANT NOTE: It seems that the definition of positive and negative angles is different
            # between Mantid and the beam scientists. This explains the different signs for x_beam and
            # bench_rotation.
            xshift = bench_rotation - x_beam * x_scale_factor
            sanslog.notice("Setup rotate " + str(xshift * x_scale_factor) + " " + str(0.0) + " " + str(0.0))
            RotateInstrumentComponent(workspace, ComponentName=component_name, X=0, Y=1, Z=0, Angle=xshift)
        return xshift

    def append_marked(self, detNames):
        self._marked_dets.append(detNames)

    def get_marked_dets(self):
        return self._marked_dets

    def load_transmission_inst(self, ws_trans, ws_direct, beamcentre):
        """
        Larmor requires centralisation of the detectors of the transmission
        as well as the sample and can.
        """
        self.move_components(ws_trans, beamcentre[0], beamcentre[1])
        if ws_trans != ws_direct:
            self.move_components(ws_direct, beamcentre[0], beamcentre[1])

    def cur_detector_position(self, ws_name):
        """Return the position of the center of the detector bank"""
        # Unfortunately getting the angle of the bench does not work so we have to get bench and detector

        # logger.warning("Entering cur_detector_position")
        ws = mtd[ws_name]
        # define the vector along the beam axis
        a1 = V3D(0, 0, 1)
        # position of the detector itself
        pos = ws.getInstrument().getComponentByName("LARMORSANSDetector").getPos()
        # position of the bench
        pos2 = ws.getInstrument().getComponentByName(self.cur_detector().name()).getPos()
        # take the difference
        posdiff = pos - pos2
        deg2rad = 4.0 * math.atan(1.0) / 180.0
        # now finally find the angle between the vector for the difference and the beam axis
        angle = posdiff.angle(a1) / deg2rad

        # Get the angle of the rotation from the rotation quaternion
        # At this point we also need to take the sign of the axis into account
        instrument = ws.getInstrument()
        detector_bench = instrument.getComponentByName("DetectorBench")
        rot = detector_bench.getRotation()
        angle, axis = su.quaternion_to_angle_and_axis(rot)
        angle = copysign(angle, axis[1])

        # return the angle and the y displacement
        # logger.warning("Blah: angle=" + str(angle) + " Y displacement=" +str(-pos2.getY()) )
        return [-angle, -pos2.getY()]

    def on_load_sample(self, ws_name, beamcentre, isSample):
        """For Larmor in addition to the operations defined in on_load_sample of ISISInstrument
        it has to deal with the log, which defines some offsets for the movement of the
        detector bank.
        """
        # logger.warning("Entering on_load_sample")
        ws_ref = mtd[str(ws_name)]
        # in order to avoid problems with files from initial commissioning during 2014.
        # these didn't have the required log entries for the detector position

        if LARMOR.is_run_new_style_run(ws_ref):
            try:
                # logger.warning("Trying get_detector_log")
                log = self.get_detector_log(ws_ref)
                if log == "":
                    raise RuntimeError("Invalid log")
            except:
                if isSample:
                    run = ws_ref.run()
                    if not run.hasProperty("Bench_Rot"):
                        additional_message = (
                            "The Bench_Rot entry seems to be missing. There might be "
                            "an issue with your data acquisition. Make sure that the sample_log entry "
                            "Bench_Rot is available."
                        )
                    else:
                        additional_message = ""

                    raise RuntimeError("Sample logs cannot be loaded, cannot continue. {0}".format(additional_message))
                else:
                    logger.warning("Can logs could not be loaded, using sample values.")

            if isSample:
                su.check_has_bench_rot(ws_ref, log)
                self.apply_detector_logs(log)
            else:
                su.check_has_bench_rot(ws_ref, log)
                self.check_can_logs(log)

        ISISInstrument.on_load_sample(self, ws_name, beamcentre, isSample)

    @staticmethod
    def is_run_new_style_run(workspace_ref):
        """
        Checks if the run assiated with the workspace is pre or post 2217
        Original comment:
        In order to avoid problems with files from initial commissioning during 2014.
        these didn't have the required log entries for the detector position
        @param workspace_ref:: A handle to the workspace
        """
        try:
            run_num = LARMOR.get_run_number_from_workspace_reference(workspace_ref)
        except:
            # If the workspace does not contain logs from which we can get the run number
            # then we get the run number from the workspace name which has the form
            # [\d]+_[sans|trans]_[\d+]. The first set of digits is the one we are after.
            # The end set of digits corresponds to the period number.
            # Previously this method looked for the last set. It is not clear why.
            ws_name = workspace_ref.name()
            run_num = int(re.findall(r"\d+", str(ws_name))[0])
        if int(run_num) >= 2217:
            return True
        else:
            return False

    @staticmethod
    def get_run_number_from_workspace_reference(ws_ref):
        # If we are dealing with a WorkspaceGroup then this will not contain any run number information,
        # hence we need to access the first child workspace
        if isinstance(ws_ref, WorkspaceGroup):
            run_num = ws_ref[0].getRun().getLogData("run_number").value
        else:
            run_num = ws_ref.getRun().getLogData("run_number").value
        return run_num

    def get_m4_monitor_det_ID(self):
        return self._m4_det_id


if __name__ == "__main__":
    pass
