#pylint: disable=invalid-name
import datetime
import math
import os
import re
import sys
import time
import xml.dom.minidom

from mantid.simpleapi import *
from mantid.api import WorkspaceGroup, Workspace, ExperimentInfo
from mantid.kernel import Logger
import SANSUtility as su

sanslog = Logger("SANS")

class BaseInstrument(object):
    def __init__(self, instr_filen=None):
        """
            Reads the instrument definition xml file
            @param instr_filen: the name of the instrument definition file to read
            @raise IndexError: if any parameters (e.g. 'default-incident-monitor-spectrum') aren't in the xml definition
        """
        if instr_filen is None:
            instr_filen = self._NAME+'_Definition.xml'

        config = ConfigService.Instance()
        self._definition_file = os.path.join(config["instrumentDefinition.directory"], instr_filen)

        inst_ws_name = self.load_empty()
        self.definition = AnalysisDataService.retrieve(inst_ws_name).getInstrument()

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

    def view(self, workspace_name = None):
        """
            Opens Mantidplot's InstrumentView displaying the current instrument. This
            empty instrument created contained in the named workspace (a default name
            is generated if this the argument is left blank) unless the workspace already
            exists and then it's contents are displayed
            @param workspace_name: the name of the workspace to create and/or display
        """
        if workspace_name is None:
            workspace_name = self._NAME+'_instrument_view'
            self.load_empty(workspace_name)
        elif not AnalysisDataService.doesExist(workspace_name):
            self.load_empty(workspace_name)

        import mantidplot
        instrument_win = mantidplot.getInstrumentView(workspace_name)
        instrument_win.show()

        return workspace_name

    def load_empty(self, workspace_name = None):
        """
            Loads the instrument definition file into a workspace with the given name.
            If no name is given a hidden workspace is used
            @param workspace_name: the name of the workspace to create and/or display
            @return the name of the workspace that was created
        """
        if workspace_name is None:
            workspace_name = '__'+self._NAME+'_empty'

        LoadEmptyInstrument(Filename=self._definition_file, OutputWorkspace=workspace_name)

        return workspace_name


class DetectorBank:
    class _DectShape:
        """
            Stores the dimensions of the detector, normally this is a square
            which is easy, but it can have a hole in it which is harder!
        """
        def __init__(self, width, height, isRect = True, n_pixels = None):
            """
                Sets the dimensions of the detector
                @param width: the detector's width, spectra numbers along the width should increase in intervals of one
                @param height: the detector's height, spectra numbers along the down the height should increase in intervals of width
                @param isRect: true for rectangular or square detectors, i.e. number of pixels = width * height
                @param n_pixels: optional for rectangular shapes because if it is not given it is calculated from the height and width in that case
            """
            self._width = width
            self._height = height
            self._isRect = bool(isRect)
            self._n_pixels = n_pixels
            if n_pixels is None:
                if self._isRect:
                    self._n_pixels = self._width*self._height
                else:
                    raise AttributeError('Number of pixels in the detector unknown, you must state the number of pixels for non-rectangular detectors')


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

    class _RescaleAndShift:
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

            if self.qMin == None or self.qMax == None:
                self.qRangeUserSelected = False
            else:
                self.qRangeUserSelected = True


    def __init__(self, instr, det_type):
        #detectors are known by many names, the 'uni' name is an instrument independent alias the 'long' name is the instrument view name and 'short' name often used for convenience
        self._names = {\
          'uni' : det_type,\
          'long': instr.getStringParameter(det_type+'-detector-name')[0],\
          'short': instr.getStringParameter(det_type+'-detector-short-name')[0]}
        #the bank is often also referred to by its location, as seen by the sample
        if det_type.startswith('low'):
            position = 'rear'
        else:
            position = 'front'
        self._names['position'] = position

        cols_data = instr.getNumberParameter(det_type+'-detector-num-columns')
        if len(cols_data) > 0 :
            rectanglar_shape = True
            width = int(cols_data[0])
        else :
            rectanglar_shape = False
            width = instr.getNumberParameter(det_type+'-detector-non-rectangle-width')[0]

        rows_data = instr.getNumberParameter(det_type+'-detector-num-rows')
        if len(rows_data) > 0 :
            height = int(rows_data[0])
        else:
            rectanglar_shape = False
            height = instr.getNumberParameter(det_type+'-detector-non-rectangle-height')[0]

        n_pixels = None
        n_pixels_override = instr.getNumberParameter(det_type+'-detector-num-pixels')
        if len(n_pixels_override) > 0 :
            n_pixels = int(n_pixels_override[0])
        #n_pixels is normally None and calculated by DectShape but LOQ (at least) has a detector with a hole
        self._shape = self._DectShape(width, height, rectanglar_shape, n_pixels)

        spec_entry = instr.getNumberParameter('first-low-angle-spec-number')
        if len(spec_entry) > 0 :
            self.set_first_spec_num(int(spec_entry[0]))
        else :
            #'first-low-angle-spec-number' is an optimal instrument parameter
            self.set_first_spec_num(0)

        #needed for compatibility with SANSReduction and SANSUtily, remove
        self.n_columns = width

        #this can be set to the name of a file with correction factor against wavelength
        self.correction_file = ''
        #this corrections are set by the mask file
        self.z_corr = 0.0
        self.x_corr = 0.0
        self._y_corr = 0.0
        self._rot_corr = 0.0
        #23/3/12 RKH add 2 more variables
        self._radius_corr = 0.0
        self._side_corr =0.0

        # hold rescale and shift object _RescaleAndShift
        self.rescaleAndShift = self._RescaleAndShift()

        #in the empty instrument detectors are laid out as below on loading a run the orientation becomes run dependent
        self._orientation = 'HorizontalFlipped'

    def disable_y_and_rot_corrs(self):
        """
            Not all corrections are supported on all detectors
        """
        self._y_corr = None
        self._rot_corr = None
        #23/3/12 RKH add 2 more variables
        self._radius_corr = None
        self._side_corr = None

    def get_y_corr(self):
        if not self._y_corr is None:
            return self._y_corr
        else:
            raise NotImplementedError('y correction is not used for this detector')

    def set_y_corr(self, value):
        """
            Only set the value if it isn't disabled
            @param value: set y_corr to this value, unless it's disabled
        """
        if not self._y_corr is None:
            self._y_corr = value

    def get_rot_corr(self):
        if not self._rot_corr is None:
            return self._rot_corr
        else:
            raise NotImplementedError('rot correction is not used for this detector')

    def set_rot_corr(self, value):
        """
            Only set the value if it isn't disabled
            @param value: set rot_corr to this value, unless it's disabled
        """
        if not self._rot_corr is None:
            self._rot_corr = value

    # 22/3/12 RKH added two new variables radius_corr, side_corr
    def get_radius_corr(self):
        if not self._radius_corr is None:
            return self._radius_corr
        else:
            raise NotImplementedError('radius correction is not used for this detector')

    def set_radius_corr(self, value):
        """
            Only set the value if it isn't disabled
            @param value: set radius_corr to this value, unless it's disabled
        """
        if not self._rot_corr is None:
            self._radius_corr = value

    def get_side_corr(self):
        if not self._side_corr is None:
            return self._side_corr
        else:
            raise NotImplementedError('side correction is not used for this detector')

    def set_side_corr(self, value):
        """
            Only set the value if it isn't disabled
            @param value: set side_corr to this value, unless it's disabled
        """
        if not self._side_corr is None:
            self._side_corr = value


    y_corr = property(get_y_corr, set_y_corr, None, None)
    rot_corr = property(get_rot_corr , set_rot_corr, None, None)
    # 22/3/12 RKH added 2 new variables
    radius_corr = property(get_radius_corr , set_radius_corr, None, None)
    side_corr = property(get_side_corr , set_side_corr, None, None)

    def get_first_spec_num(self):
        return self._first_spec_num

    def set_first_spec_num(self, value):
        self._first_spec_num = value
        self.last_spec_num = self._first_spec_num + self._shape.n_pixels() - 1

    def place_after(self, previousDet):
        self.set_first_spec_num(previousDet.last_spec_num + 1)

    def name(self, form = 'long') :
        if form.lower() == 'inst_view' : form = 'long'
        if not self._names.has_key(form) : form = 'long'

        return self._names[form]

    def isAlias(self, guess) :
        """
            Detectors are often referred to by more than one name, check
            if the supplied name is in the list
            @param guess: this name will be searched for in the list
            @return : True if the name was found, otherwise false
        """
        for name in self._names.values() :
            if guess.lower() == name.lower() :
                return True
        return False

    def spectrum_block(self, ylow, xlow, ydim, xdim):
        """
            Compile a list of spectrum IDs for rectangular block of size xdim by ydim
        """
        if ydim == 'all':
            ydim = self._shape.height()
        if xdim == 'all':
            xdim = self._shape.width()
        det_dimension = self._shape.width()
        base = self._first_spec_num

        if not self._shape.isRectangle():
            sanslog.warning('Attempting to block rows or columns in a non-rectangular detector, this is likely to give unexpected results!')

        output = ''
        if self._orientation == 'Horizontal':
            start_spec = base + ylow*det_dimension + xlow
            for y in range(0, ydim):
                for x in range(0, xdim):
                    output += str(start_spec + x + (y*det_dimension)) + ','
        elif self._orientation == 'Vertical':
            start_spec = base + xlow*det_dimension + ylow
            for x in range(det_dimension - 1, det_dimension - xdim-1,-1):
                for y in range(0, ydim):
                    std_i = start_spec + y + ((det_dimension-x-1)*det_dimension)
                    output += str(std_i ) + ','
        elif self._orientation == 'Rotated':
            # This is the horizontal one rotated so need to map the xlow and vlow to their rotated versions
            start_spec = base + ylow*det_dimension + xlow
            max_spec = det_dimension*det_dimension + base - 1
            for y in range(0, ydim):
                for x in range(0, xdim):
                    std_i = start_spec + x + (y*det_dimension)
                    output += str(max_spec - (std_i - base)) + ','
        elif self._orientation == 'HorizontalFlipped':
            for y in range(ylow,ylow+ydim):
                max_row = base + (y+1)*det_dimension - 1
                min_row = base + (y)*det_dimension
                for x in range(xlow,xlow+xdim):
                    std_i = base + x + (y*det_dimension)
                    diff_s = std_i - min_row
                    output += str(max_row - diff_s) + ','

        return output.rstrip(",")

    # Used to constrain the possible values of the orientation of the detector bank against the direction that spectrum numbers increase in
    _ORIENTED = {
        'Horizontal' : None,        #most runs have the detectors in this state
        'Vertical' : None,
        'Rotated' : None,
        'HorizontalFlipped' : None} # This is for the empty instrument

    def set_orien(self, orien):
        """
            Sets to relationship between the detectors and the spectra numbers. The relationship
            is given by an orientation string and this function throws if the string is not recognised
            @param orien: the orienation string must be a string contained in the dictionary _ORIENTED
        """
        dummy = self._ORIENTED[orien]
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
            #Is it really necessary to crop?
            if wki.getNumberHistograms() != self.last_spec_num - self.get_first_spec_num() + 1:
                CropWorkspace(InputWorkspace=input_name,OutputWorkspace= output_name,
                              StartWorkspaceIndex = self.get_first_spec_num() - 1,
                              EndWorkspaceIndex = self.last_spec_num - 1)
        except :
            raise ValueError('Can not find spectra for %s in the workspace %s [%d,%d]\nException:'
                             %(self.name(), input_name,self.get_first_spec_num(),self.last_spec_num)
                             + str(sys.exc_info()))

class ISISInstrument(BaseInstrument):
    def __init__(self, filename=None):
        """
            Reads the instrument definition xml file
            @param filename: the name of the instrument definition file to read
            @raise IndexError: if any parameters (e.g. 'default-incident-monitor-spectrum') aren't in the xml definition
        """
        super(ISISInstrument, self).__init__(instr_filen=filename)

        self.idf_path = self._definition_file

        #the spectrum with this number is used to normalize the workspace data
        self._incid_monitor = int(self.definition.getNumberParameter(
            'default-incident-monitor-spectrum')[0])
        self.cen_find_step = float(self.definition.getNumberParameter(
            'centre-finder-step-size')[0])

        firstDetect = DetectorBank(self.definition, 'low-angle')
        #firstDetect.disable_y_and_rot_corrs()
        secondDetect = DetectorBank(self.definition, 'high-angle')
        secondDetect.place_after(firstDetect)
        #add det_selection variable that will receive the DET/ REAR/FRONT/BOTH/MERGED
        self.det_selection = 'REAR'
        self.DETECTORS = {'low-angle' : firstDetect}
        self.DETECTORS['high-angle'] = secondDetect

        self.setDefaultDetector()
        # if this is set InterpolationRebin will be used on the monitor spectrum used to normalize the sample, useful because wavelength resolution in the monitor spectrum can be course in the range of interest
        self._use_interpol_norm = False
        #remove use_interpol_trans_calc once the beam centre finder has been converted
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

        #spectrum number of the monitor used to as the incidient in the transmission calculations
        self.default_trans_spec = int(self.definition.getNumberParameter(
            'default-transmission-monitor-spectrum')[0])
        self.incid_mon_4_trans_calc = self._incid_monitor

        isis = config.getFacility('ISIS')
        # Number of digits in standard file name
        self.run_number_width = isis.instrument(self._NAME).zeroPadding(0)

        #this variable isn't used again and stops the instrument from being deep copied if this instance is deep copied
        self.definition = None

        #remove this function
        self._del_incidient_set = False

        #it is possible to set the TOF regions that is assumed to be background for each monitors
        self._back_ground = {}
        # the default start region, used for any monitors that a specific one wasn't set for
        self._back_start = None
        # default end region
        self._back_end = None
        #if the user moves a monitor to this z coordinate (with MON/LENGTH ...) this will be recorded here. These are overridden lines like TRANS/TRANSPEC=4/SHIFT=-100
        self.monitor_zs = {}
        # Used when new calibration required.
        self._newCalibrationWS = None

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
            remove this function and the data memember it uses
        """
        if not self._del_incidient_set:
            self.set_incident_mon(spectrum_number)

    def set_sample_offset(self, value):
        """
            @param value: sample value offset
        """
        self.SAMPLE_Z_CORR = float(value)/1000.

    def is_interpolating_norm(self):
        return self._use_interpol_norm

    def set_interpolating_norm(self, on=True):
        """
            This method sets that the monitor spectrum should be interpolated before
            normalisation
        """
        self._use_interpol_norm = on

    def cur_detector(self):
        if self.lowAngDetSet : return self.DETECTORS['low-angle']
        else : return self.DETECTORS['high-angle']

    def get_low_angle_detector(self):
        """ Provide a direct way to get the low bank detector.
        This method does not require to pass the name of the detector bank.
        """
        return self.DETECTORS['low-angle']

    def get_high_angle_detector(self):
        """ Provide a direct way to get the high bank detector
        This method does not require to pass the name of the detector bank.
        """
        return self.DETECTORS['high-angle']

    def other_detector(self) :
        if not self.lowAngDetSet : return self.DETECTORS['low-angle']
        else : return self.DETECTORS['high-angle']

    def getDetector(self, requested) :
        for n, detect in self.DETECTORS.iteritems():
            if detect.isAlias(requested):
                return detect
        sanslog.notice("getDetector: Detector " + requested + "not found")

    def listDetectors(self) :
        return self.cur_detector().name(), self.other_detector().name()

    def isHighAngleDetector(self, detName) :
        if self.DETECTORS['high-angle'].isAlias(detName) :
            return True

    def isDetectorName(self, detName) :
        if self.other_detector().isAlias(detName) :
            return True

        return self.cur_detector().isAlias(detName)

    def setDetector(self, detName) :
        self.det_selection = detName
        if self.other_detector().isAlias(detName) :
            self.lowAngDetSet = not self.lowAngDetSet
            return True
        else:
            #there are only two detectors, they must have selected the current one so no change is need
            if self.cur_detector().isAlias(detName):
                return True
            else:
                sanslog.notice("setDetector: Detector not found")
                sanslog.notice("setDetector: Detector set to " + self.cur_detector().name() + ' in ' + self.name())

    def setDefaultDetector(self):
        self.lowAngDetSet = True

    def copy_correction_files(self):
        """
            Check if one of the efficiency files hasn't been set and assume the other is to be used
        """
        a = self.cur_detector()
        b = self.other_detector()
        if a.correction_file == '' and b.correction_file != '':
            a.correction_file = b.correction_file != ''
        if b.correction_file == '' and a.correction_file != '':
            b.correction_file = a.correction_file != ''

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
        if self._back_ground.has_key(monitor):
            return self._back_ground[int(monitor)]['start'], \
                self._back_ground[int(monitor)]['end']
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
        if start != None:
            start = float(start)
        if end != None:
            end = float(end)

        if monitor:
            self._back_ground[int(monitor)] = { 'start' : start, 'end' : end }
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
            if self._back_ground.has_key(monitor):
                del self._back_ground[int(monitor)]
        else:
            self._back_ground = {}
            self._back_start = None
            self._back_end = None

    def move_all_components(self, ws):
        """
            Move the sample object to the location set in the logs or user settings file
            @param ws: the workspace containing the sample to move
        """
        MoveInstrumentComponent(Workspace=ws,ComponentName= 'some-sample-holder', Z = self.SAMPLE_Z_CORR, RelativePosition=True)

        for i in self.monitor_zs.keys():
            #get the current location
            component = self.monitor_names[i]
            ws = mtd[str(ws)]
            mon = ws.getInstrument().getComponentByName(component)
            z_loc = mon.getPos().getZ()
            #now the relative move
            offset = (self.monitor_zs[i]/1000.) - z_loc
            MoveInstrumentComponent(Workspace=ws,ComponentName= component, Z = offset,
                                    RelativePosition=True)

    def move_components(self, ws, beamX, beamY):
        """Define how to move the bank to position beamX and beamY must be implemented"""
        raise RuntimeError("Not Implemented")

    def cur_detector_position(self, ws_name):
        """Return the position of the center of the detector bank"""
        raise RuntimeError("Not Implemented")

    def on_load_sample(self, ws_name, beamcentre, isSample):
        """It will be called just after loading the workspace for sample and can

        It configures the instrument for the specific run of the workspace for handle historical changes in the instrument.

        It centralizes the detector bank to teh beamcentre (tuple of two values)
        """
        ws_ref = mtd[str(ws_name)]
        try:
            run_num = ws_ref.getRun().getLogData('run_number').value
        except:
            run_num = int(re.findall(r'\d+',str(ws_name))[-1])

        if isSample:
            self.set_up_for_run(run_num)

        if self._newCalibrationWS:
            self.changeCalibration(ws_name)

        # centralize the bank to the centre
        self.move_components(ws_name, beamcentre[0], beamcentre[1])

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



class LOQ(ISISInstrument):
    """
        Contains all the LOQ specific data and functions
    """
    _NAME = 'LOQ'
    #minimum wavelength of neutrons assumed to be measurable by this instrument
    WAV_RANGE_MIN = 2.2
    #maximum wavelength of neutrons assumed to be measurable by this instrument
    WAV_RANGE_MAX = 10.0

    def __init__(self):
        """
            Reads LOQ's instrument definition xml file
            @raise IndexError: if any parameters (e.g. 'default-incident-monitor-spectrum') aren't in the xml definition
        """
        super(LOQ, self).__init__('LOQ_Definition_20020226-.xml')
        #relates the numbers of the monitors to their names in the instrument definition file
        self.monitor_names = {1 : 'monitor1',
                              2 : 'monitor2'}

    def move_components(self, ws, xbeam, ybeam):
        """
            Move the locations of the sample and detector bank based on the passed beam center
            and information from the sample workspace logs
            @param ws: workspace containing the instrument information
            @param xbeam: x-position of the beam
            @param ybeam: y-position of the beam
            @return: the locations of (in the new coordinates) beam center, center of detector bank
        """
        self.move_all_components(ws)

        xshift = (317.5/1000.) - xbeam
        yshift = (317.5/1000.) - ybeam
        MoveInstrumentComponent(Workspace=ws,ComponentName= self.cur_detector().name(), X = xshift, Y = yshift, RelativePosition="1")

        # Have a separate move for x_corr, y_coor and z_coor just to make it more obvious in the
        # history, and to expert users what is going on
        det = self.cur_detector()
        if det.x_corr != 0.0 or det.y_corr != 0.0 or det.z_corr != 0.0:
            MoveInstrumentComponent(Workspace=ws,ComponentName= det.name(), X = det.x_corr/1000.0, Y = det.y_corr/1000.0, Z = det.z_corr/1000.0, RelativePosition="1")
            xshift = xshift + det.x_corr/1000.0
            yshift = yshift + det.y_corr/1000.0

        return [xshift, yshift], [xshift, yshift]

    def get_marked_dets(self):
        raise NotImplementedError('The marked detector list isn\'t stored for instrument '+self._NAME)

    def set_up_for_run(self, base_runno):
        """
            Needs to run whenever a sample is loaded
        """
        first = self.DETECTORS['low-angle']
        second = self.DETECTORS['high-angle']

        first.set_orien('Horizontal')
        #probably _first_spec_num was already set to this when the instrument parameter file was loaded
        first.set_first_spec_num(3)
        second.set_orien('Horizontal')
        second.place_after(first)

    def load_transmission_inst(self, ws_trans, ws_direct, beamcentre):
        """
            Loads information about the setup used for LOQ transmission runs
        """
        trans_definition_file = os.path.join(config.getString('instrumentDefinition.directory'), self._NAME+'_trans_Definition.xml')
        LoadInstrument(Workspace=ws_trans,Filename= trans_definition_file, RewriteSpectraMap=False)
        LoadInstrument(Workspace=ws_direct, Filename = trans_definition_file, RewriteSpectraMap=False)

    def cur_detector_position(self, ws_name):
        """Return the position of the center of the detector bank"""
        ws = mtd[ws_name]
        pos = ws.getInstrument().getComponentByName(self.cur_detector().name()).getPos()
        cent_pos = 317.5/1000.0
        return [cent_pos - pos.getX(), cent_pos - pos.getY()]

class SANS2D(ISISInstrument):
    """
        The SANS2D instrument has movable detectors whose locations have to
        be read in from the workspace logs (Run object)
    """
    _NAME = 'SANS2D'
    WAV_RANGE_MIN = 2.0
    WAV_RANGE_MAX = 14.0

    def __init__(self, idf_path=None):
        super(SANS2D, self).__init__(idf_path)

        self._marked_dets = []
        # set to true once the detector positions have been moved to the locations given in the sample logs
        self.corrections_applied = False
        # a warning is issued if the can logs are not the same as the sample
        self._can_logs = {}
        #The user can set the distance between monitor 4 and the rear detector in millimetres, should be negative
        self.monitor_4_offset = None
        #relates the numbers of the monitors to their names in the instrument definition file
        self.monitor_names = {1 : 'monitor1',
                              2 : 'monitor2',
                              3 : 'monitor3',
                              4 : 'monitor4'}

    def set_up_for_run(self, base_runno):
        """
            Handles changes required when a sample is loaded, both generic
            and run specific
        """
        first = self.DETECTORS['low-angle']
        second = self.DETECTORS['high-angle']

        try:
            base_runno = int(base_runno)
            #first deal with some special cases
            if base_runno < 568:
                self.set_incident_mon(73730)
                first.set_first_spec_num(1)
                first.set_orien('Vertical')
                second.set_orien('Vertical')
            elif base_runno >= 568 and base_runno < 684:
                first.set_first_spec_num(9)
                first.set_orien('Rotated')
                second.set_orien('Rotated')
            else:
                #this is the default case
                first.set_first_spec_num(9)
                first.set_orien('Horizontal')
                second.set_orien('Horizontal')
        except ValueError:
            #this is the default case
            first.set_first_spec_num(9)
            first.set_orien('Horizontal')
            # empty instrument number spectra differently.
            if base_runno == 'emptyInstrument':
                second.set_orien('HorizontalFlipped')
            else:
                second.set_orien('Horizontal')

        #as spectrum numbers of the first detector have changed we'll move those in the second too
        second.place_after(first)

    def getDetValues(self, ws_name):
        """
        Retrive the values of Front_Det_Z, Front_Det_X, Front_Det_Rot, Rear_Det_Z and Rear_Det_X from
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
        for name in ('Front_Det_Z', 'Front_Det_X', 'Front_Det_Rot',
                     'Rear_Det_Z','Rear_Det_X'):
            try:
                var = run_info.get(name).value
                if hasattr(var, '__iter__'):
                    var = var[-1]
                values[ind] = float(var)
            except:
                pass # ignore, because we do have a default value
            ind+=1
        #return these variables
        return tuple(values)


    def  move_components(self, ws, xbeam, ybeam):
        """
            Move the locations of the sample and detector bank based on the passed beam center
            and information from the sample workspace logs. If the location of the monitor was
            set with TRANS/TRANSPEC=4/SHIFT=... this function does the move instrument
            @param ws: workspace containing the instrument information
            @param xbeam: x-position of the beam in meters
            @param ybeam: y-position of the beam in meters
            @return: the locations of (in the new coordinates) beam center, center of detector bank
        """
        frontDet = self.getDetector('front')
        rearDet = self.getDetector('rear')

        FRONT_DET_Z, FRONT_DET_X, FRONT_DET_ROT, REAR_DET_Z, REAR_DET_X = self.getDetValues(ws)

        # Deal with front detector
        # 9/1/2  this all dates to Richard Heenan & Russell Taylor's original python development for SANS2d
    	# the rotation axis on the SANS2d front detector is actually set front_det_radius = 306mm behind the detector.
    	# Since RotateInstrumentComponent will only rotate about the centre of the detector, we have to to the rest here.
        # rotate front detector according to value in log file and correction value provided in user file
        rotateDet = (-FRONT_DET_ROT - frontDet.rot_corr)
        RotateInstrumentComponent(Workspace=ws,ComponentName= self.getDetector('front').name(), X="0.", Y="1.0", Z="0.", Angle=rotateDet)
        RotRadians = math.pi*(FRONT_DET_ROT + frontDet.rot_corr)/180.
        # The rear detector is translated to the beam position using the beam centre coordinates in the user file.
    	# (Note that the X encoder values in NOT used for the rear detector.)
    	# The front detector is translated using the difference in X encoder values, with a correction from the user file.
    	# 21/3/12 RKH [In reality only the DIFFERENCE in X encoders is used, having separate X corrections for both detectors is unnecessary,
    	# but we will continue with this as it makes the mask file smore logical and avoids a retrospective change.]
    	# 21/3/12 RKH add .side_corr    allows rotation axis of the front detector being offset from the detector X=0
    	# this inserts *(1.0-math.cos(RotRadians)) into xshift, and
    	# - frontDet.side_corr*math.sin(RotRadians) into zshift.
    	# (Note we may yet need to introduce further corrections for parallax errors in the detectors, which may be wavelength dependent!)
        xshift = (REAR_DET_X + rearDet.x_corr -frontDet.x_corr - FRONT_DET_X  -frontDet.side_corr*(1-math.cos(RotRadians)) + (self.FRONT_DET_RADIUS +frontDet.radius_corr)*math.sin(RotRadians) )/1000. - self.FRONT_DET_DEFAULT_X_M - xbeam
        yshift = (frontDet.y_corr/1000.  - ybeam)
        # Note don't understand the comment below (9/1/12 these are comments from the original python code, you may remove them if you like!)
        # default in instrument description is 23.281m - 4.000m from sample at 19,281m !
        # need to add ~58mm to det1 to get to centre of detector, before it is rotated.
    	# 21/3/12 RKH add .radius_corr
        zshift = (FRONT_DET_Z + frontDet.z_corr + (self.FRONT_DET_RADIUS +frontDet.radius_corr)*(1 - math.cos(RotRadians)) - frontDet.side_corr*math.sin(RotRadians))/1000.
        zshift -= self.FRONT_DET_DEFAULT_SD_M
        MoveInstrumentComponent(Workspace=ws,ComponentName= self.getDetector('front').name(), X = xshift, Y = yshift, Z = zshift, RelativePosition="1")


        # deal with rear detector

        xshift = -xbeam
        yshift = -ybeam
        zshift = (REAR_DET_Z + rearDet.z_corr)/1000.
        zshift -= self.REAR_DET_DEFAULT_SD_M
        sanslog.notice("Setup move "+str(xshift*1000.)+" "+str(yshift*1000.)+" "+str(zshift*1000.))
        MoveInstrumentComponent(Workspace=ws,ComponentName= rearDet.name(), X = xshift, Y = yshift, Z = zshift, RelativePosition="1")


        self.move_all_components(ws)

        #this implements the TRANS/TRANSPEC=4/SHIFT=... line, this overrides any other monitor move
        if self.monitor_4_offset:
            #get the current location of the monitor
            component = 'monitor4'
            ws = mtd[str(ws)]
            mon = ws.getInstrument().getComponentByName(component)
            z_orig = mon.getPos().getZ()

            #the location is relative to the rear-detector, get its location
            det = ws.getInstrument().getComponentByName(self.cur_detector().name())
            det_z = det.getPos().getZ()

            monitor_4_offset = self.monitor_4_offset/1000.
            z_new = det_z + monitor_4_offset
            z_move = z_new - z_orig
            MoveInstrumentComponent(Workspace=ws,ComponentName= component, Z=z_move,
                                    RelativePosition=True)
            sanslog.notice('Monitor 4 is at z = ' + str(z_new) )

        # Are these returned values used anywhere?
        if self.cur_detector().name() == 'front-detector':
            beam_cen = [0.0, 0.0]
            det_cen = [0.0, 0.0]
        else:
            beam_cen = [0.0,0.0]
            det_cen = [-xbeam, -ybeam]

        return beam_cen, det_cen

    def get_detector_log(self, wksp):
        """
            Reads information about the state of the instrument on the information
            stored in the sample
            @param logs: a workspace pointer
            @return the values that were read as a dictionary
        """
        self._marked_dets = []
        wksp = su.getWorkspaceReference(wksp)
        #assume complete log information is stored in the first entry, it isn't stored in the group workspace itself
        if isinstance(wksp, WorkspaceGroup):
            wksp = wksp[0]

        samp = wksp.getRun()

        logvalues = {}
        logvalues['Front_Det_Z'] = self._get_const_num(samp, 'Front_Det_Z')
        logvalues['Front_Det_X'] = self._get_const_num(samp, 'Front_Det_X')
        logvalues['Front_Det_Rot'] = self._get_const_num(samp, 'Front_Det_Rot')
        logvalues['Rear_Det_Z'] = self._get_const_num(samp, 'Rear_Det_Z')
        logvalues['Rear_Det_X'] = self._get_const_num(samp, 'Rear_Det_X')

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
            # Python 2.4 doesn't have datetime.strptime...
            def format_date(date_string, format, date_str_len):
                if len(date_string)>date_str_len:
                    date_string = date_string[:date_str_len]
                from datetime import datetime
                if sys.version_info[0] == 2 and sys.version_info[1] <  5:
                    import time
                    return datetime(*(time.strptime(date_string, format)[0:6]))
                else:
                    return datetime.strptime(date_string, format)

            # if the value was stored as a time series we have an array here
            property = log_data.getLogData(log_name)

            size = len(property.value)
            if size == 1:
                return float(log_data.getLogData(log_name).value[0])

            start = log_data.getLogData('run_start')
            dt_0 = format_date(start.value,"%Y-%m-%dT%H:%M:%S",19)
            for i in range(0, size):
                dt = format_date(str(property.times[i]),"%Y-%m-%dT%H:%M:%S",19)
                if dt > dt_0:
                    if i == 0:
                        return float(log_data.getLogData(log_name).value[0])
                    else:
                        return float(log_data.getLogData(log_name).value[i-1])

            # this gets executed if all entries is before the start-time
            return float(log_data.getLogData(log_name).value[size-1])

    def apply_detector_logs(self, logvalues):
        #apply the corrections that came from the logs
        self.FRONT_DET_Z = float(logvalues['Front_Det_Z'])
        self.FRONT_DET_X = float(logvalues['Front_Det_X'])
        self.FRONT_DET_ROT = float(logvalues['Front_Det_Rot'])
        self.REAR_DET_Z = float(logvalues['Rear_Det_Z'])
        self.REAR_DET_X = float(logvalues['Rear_Det_X'])
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
            #the check needs to wait until there's something to compare against
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
        new_values.append(float(new_logs['Front_Det_Z']))
        new_values.append(float(new_logs['Front_Det_X']))
        new_values.append(float(new_logs['Front_Det_Rot']))
        new_values.append(float(new_logs['Rear_Det_Z']))
        new_values.append(float(new_logs['Rear_Det_X']))

        errors = 0
        corr_names = ['Front_Det_Z', 'Front_Det_X','Front_Det_Rot', 'Rear_Det_Z', 'Rear_Det_X']
        for i in range(0, len(existing_values)):
            if math.fabs(existing_values[i] - new_values[i]) > 5e-04:
                sanslog.warning('values differ between sample and can runs: Sample ' + corr_names[i] + ' = ' + str(existing_values[i]) + \
                    ', can value is ' + str(new_values[i]))
                errors += 1

                self.append_marked(corr_names[i])

        #the check has been done clear up
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
                raise "Invalid log"
        except:
            if isSample:
                raise RuntimeError('Sample logs cannot be loaded, cannot continue')
            else:
                logger.warning("Can logs could not be loaded, using sample values.")


        if isSample:
            self.apply_detector_logs(log)
        else:
            self.check_can_logs(log)


        ISISInstrument.on_load_sample(self, ws_name, beamcentre,  isSample)


class LARMOR(ISISInstrument):
    _NAME = 'LARMOR'
    WAV_RANGE_MIN = 2.2
    WAV_RANGE_MAX = 10.0
    def __init__(self):
        super(LARMOR,self).__init__('LARMOR_Definition.xml')
        self.monitor_names = dict()

        for i in range(1,6):
            self.monitor_names[i] = 'monitor'+str(i)

    def set_up_for_run(self, base_runno):
        """
            Needs to run whenever a sample is loaded
        """
        first = self.DETECTORS['low-angle']
        second = self.DETECTORS['high-angle']

        first.set_orien('Horizontal')
        first.set_first_spec_num(10)
        second.set_orien('Horizontal')
        second.place_after(first)

    def move_components(self, ws, xbeam, ybeam):
        self.move_all_components(ws)

        detBanch = self.getDetector('rear')

        xshift = -xbeam
        yshift = -ybeam
        #zshift = ( detBanch.z_corr)/1000.
        #zshift -= self.REAR_DET_DEFAULT_SD_M
        zshift = 0
        sanslog.notice("Setup move " + str(xshift*1000) + " " + str(yshift*1000) + " " + str(zshift*1000))
        MoveInstrumentComponent(ws, ComponentName=detBanch.name(), X=xshift,
                                Y=yshift, Z=zshift)
        # beam centre, translation
        return [0.0, 0.0], [-xbeam, -ybeam]

    def cur_detector_position(self, ws_name):
        """Return the position of the center of the detector bank"""
        ws = mtd[ws_name]
        pos = ws.getInstrument().getComponentByName(self.cur_detector().name()).getPos()

        return [-pos.getX(), -pos.getY()]

class LARMOR(ISISInstrument):
    _NAME = 'LARMOR'
    WAV_RANGE_MIN = 2.2
    WAV_RANGE_MAX = 10.0
    def __init__(self):
        super(LARMOR,self).__init__('LARMOR_Definition.xml')
        self.monitor_names = dict()

        for i in range(1,6):
            self.monitor_names[i] = 'monitor'+str(i)

    def set_up_for_run(self, base_runno):
        """
            Needs to run whenever a sample is loaded
        """
        first = self.DETECTORS['low-angle']
        second = self.DETECTORS['high-angle']

        first.set_orien('Horizontal')
        first.set_first_spec_num(10)
        second.set_orien('Horizontal')
        second.place_after(first)

    def move_components(self, ws, xbeam, ybeam):
        self.move_all_components(ws)

        detBanch = self.getDetector('rear')

        xshift = -xbeam
        yshift = -ybeam
        #zshift = ( detBanch.z_corr)/1000.
        #zshift -= self.REAR_DET_DEFAULT_SD_M
        zshift = 0
        sanslog.notice("Setup move " + str(xshift*1000) + " " + str(yshift*1000) + " " + str(zshift*1000))
        MoveInstrumentComponent(ws, ComponentName=detBanch.name(), X=xshift,
                                Y=yshift, Z=zshift)
        # beam centre, translation
        return [0.0, 0.0], [-xbeam, -ybeam]

    def load_transmission_inst(self, ws_trans, ws_direct, beamcentre):
        """
            Not required for SANS2D
        """
        pass

    def cur_detector_position(self, ws_name):
        """Return the position of the center of the detector bank"""
        ws = mtd[ws_name]
        pos = ws.getInstrument().getComponentByName(self.cur_detector().name()).getPos()

        return [-pos.getX(), -pos.getY()]



if __name__ == '__main__':
    pass
