from MantidFramework import *
mtd.initialise()
import sys
import math
from mantidsimple import *

def instrument_factory(name):
    """
        Returns an instance of the instrument with the given class name
        @param name: name of the instrument class to instantiate
    """
    if name in globals():
        return globals()[name]()
    else:
        raise RuntimeError, "Instrument %s doesn't exist\n  %s" % (name, sys.exc_value)

class Instrument(object):
    def __init__(self):
        """
            Reads the instrument definition xml file
            @raise IndexError: if any parameters (e.g. 'default-incident-monitor-spectrum') aren't in the xml definition
        """ 
    
        self._definition_file = \
            mtd.getConfigProperty('instrumentDefinition.directory')+'/'+self._NAME+'_Definition.xml'
        temp_WS_name = '_'+self._NAME+'instrument_definition'
        #read the information about the instrument that stored in it's xml
        LoadEmptyInstrument(
            self._definition_file, temp_WS_name)
        definitionWS = mtd[temp_WS_name]  
        self.definition = definitionWS.getInstrument()
        mtd.deleteWorkspace(temp_WS_name)
        
        #the spectrum with this number is used to normalize the workspace data
        self._incid_monitor = int(self.definition.getNumberParameter(
            'default-incident-monitor-spectrum')[0])
        #this is used by suggest_incident_mntr() below 
        self._incid_monitor_lckd = False
        
    def suggest_incident_mntr(self, spectrum_number):
        """
            Only sets the monitor spectrum number if it isn't locked, used
            so MON/SPECTRUM in ISIS user files don't change MON/LENGTH settings
            @param spectrum_number: monitor's sectrum number
        """
        if not self._incid_monitor_lckd :
            self._incid_monitor = int(spectrum_number)
        
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
        self._incid_monitor_lckd = True
        
    def set_component_positions(self, ws, xbeam, ybeam): raise NotImplementedError
        
    def set_sample_offset(self, value):
        """
            @param value: sample value offset
        """
        self.SAMPLE_Z_CORR = float(value)/1000.
        
    def name(self):
        """
            Return the name of the instrument
        """
        return self._NAME
    
    def view(self, workspace_name = None):
        """
            Opens Mantidplot's InstrumentView displaying the current instrument
        """
        if workspace_name is None:
            workspace_name = self._NAME+'_instrument_view'
            
        LoadEmptyInstrument(self._definition_file, workspace_name)
        instrument_win = qti.app.mantidUI.getInstrumentView(workspace_name)
        instrument_win.showWindow()

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
                    raise LogicError('Number of pixels in the detector unknown, you must state the number of pixels for non-rectangular detectors')

                
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

    def __init__(self, instr, det_type) :
        self.parent = instr
        #detectors are known by many names, the 'uni' name is an instrument independent alias the 'long' name is the instrument view name and 'short' name often used for convenience 
        self._names = {
          'uni' : det_type,
          'long': instr.getStringParameter(det_type+'-detector-name')[0],
          'short': instr.getStringParameter(det_type+'-detector-short-name')[0]}
        #the bank is often also referred to by its location, as seen by the sample 
        if det_type.startswith('low'):
            position = 'rear'
        else:
            position = 'front'
        self._names['position'] = position
            
        spec_entry = instr.getNumberParameter('first-low-angle-spec-number')
        if len(spec_entry) > 0 :
            self.first_spec_num = int(spec_entry[0])
        else :
            #'first-low-angle-spec-number' is an optimal instrument parameter
            self.first_spec_num = 0
        
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
        
        
        
        #needed for compatibility with SANSReduction and SANSUtily, remove
        self.n_columns = width
        
        
        
        
        self.last_spec_num = self.first_spec_num + self._shape.n_pixels() - 1
        #this can be set to the name of a file with correction factor against wavelength
        correction_file = ''
        #this corrections are set by the mask file
        self.z_corr = 0.0
        self.x_corr = 0.0 
        self._y_corr = 0.0 
        self._rot_corr = 0.0
        
        #in the empty instrument detectors are laid out as below on loading a run the orientation becomes run dependent
        self._orientation = 'HorizontalFlipped'


    def disable_y_and_rot_corrs(self):
        """
            Not all corrections are supported on all detectors
        """
        self._y_corr = None
        self._rot_corr = None

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

    y_corr = property(get_y_corr, set_y_corr, None, None)
    rot_corr = property(get_rot_corr , set_rot_corr, None, None)

    def place_after(self, previousDet):
        self.first_spec_num = previousDet.last_spec_num + 1
        self.last_spec_num = self.first_spec_num + self._shape.n_pixels() - 1

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

    def clear_corrs(self):
        self.z_corr = self.x_corr = 0.0
        self.y_corr = 0.0
        self.rot_corr = 0.0

    def spectrum_block(self, ylow, xlow, ydim, xdim):
        """
            Compile a list of spectrum IDs for rectangular block of size xdim by ydim
        """
        if ydim == 'all':
            ydim = self._shape.height()
        if xdim == 'all':
            xdim = self._shape.width()
        det_dimension = self._shape.width()
        base = self.first_spec_num

        if not self._shape.isRectangle():
            mantid.sendLogMessage('::SANS::Warning: Attempting to block rows or columns in a non-rectangular detector, this is likely to give unexpected results!')
            
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
            start_spec = base + ylow*det_dimension + xlow
            for y in range(0,ydim):
                max_row = base + (y+1)*det_dimension - 1
                min_row = base + (y)*det_dimension
                for x in range(0,xdim):
                    std_i = start_spec + x + (y*det_dimension)
                    diff_s = std_i - min_row
                    output += str(max_row - diff_s) + ','

        return output.rstrip(",")
    
    # Used to constrain the possible values of orientation 
    _ORIENTED = {
        'Horizontal' : None,        #most runs have the detectors in this state
        'Vertical' : None,
        'Rotated' : None,
        'HorizontalFlipped' : None} # This is for the empty instrument
    
    def set_orien(self, orien):
        #throw if it's not in the list of allowed
        dummy = self._ORIENTED[orien]
        self._orientation = orien

class ISISInstrument(Instrument):
    def __init__(self,) :
        Instrument.__init__(self)
    
        firstDetect = DetectorBank(self.definition, 'low-angle')
        firstDetect.disable_y_and_rot_corrs()
        secondDetect = DetectorBank(self.definition, 'high-angle')
        secondDetect.place_after(firstDetect)
        self.DETECTORS = {'low-angle' : firstDetect}
        self.DETECTORS['high-angle'] = secondDetect

        self.setDefaultDetector()
        # if this is set InterpolationRebin will be used on the monitor spectrum used to normalize the sample, useful because wavelength resolution in the monitor spectrum can be course in the range of interest 
        self._use_interpol_norm = False
        self.use_interpol_trans_calc = False

        self.SAMPLE_Z_CORR = 0
        
        # Detector position information for SANS2D
        self.FRONT_DET_RADIUS = 306.0
        self.FRONT_DET_DEFAULT_SD_M = 4.0
        self.FRONT_DET_DEFAULT_X_M = 1.1
        self.REAR_DET_DEFAULT_SD_M = 4.0

        # LOG files for SANS2D will have these encoder readings  
        self.FRONT_DET_X = 0.0
        self.FRONT_DET_Z = 0.0
        self.FRONT_DET_ROT = 0.0
        self.REAR_DET_Z = 0.0
        
        # Rear_Det_X  Will Be Needed To Calc Relative X Translation Of Front Detector 
        self.REAR_DET_X = 0

        #used in transmission calculations
        self.trans_monitor = int(self.definition.getNumberParameter(
            'default-transmission-monitor-spectrum')[0])
        self.incid_mon_4_trans_calc = self._incid_monitor

    def name(self):
        return self._NAME
        
    def is_interpolating_norm(self):
        return self._use_interpol_norm
     
    def set_interpolating_norm(self):
        """
            This method sets that the monitor spectrum should be interpolated,
            there is currently no unset method but its off by default    
        """
        self._use_interpol_norm = True
     
    def suggest_interpolating_norm(self):
        if not self._incid_monitor_lckd:
            self._use_interpol_norm = True
     
    def cur_detector(self):
        if self.lowAngDetSet : return self.DETECTORS['low-angle']
        else : return self.DETECTORS['high-angle']
    
    def other_detector(self) :
        if not self.lowAngDetSet : return self.DETECTORS['low-angle']
        else : return self.DETECTORS['high-angle']
    
    def getDetector(self, requested) :
        for n, detect in self.DETECTORS.iteritems():
            if detect.isAlias(requested):
                return detect

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
        if self.other_detector().isAlias(detName) :
            self.lowAngDetSet = not self.lowAngDetSet
            return True
        else:
            #there are only two detectors, they must have selected the current one so no change is need
            if self.cur_detector().isAlias(detName):
                return True
            else:
                mantid.sendLogMessage("::SANS::setDetector: Detector not found")
                mantid.sendLogMessage("::SANS::setDetector: Detector set to " + self.cur_detector().name() + ' in ' + self.name())
                

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

        
class LOQ(ISISInstrument):
    
    # Number of digits in standard file name
    run_number_width = 5
    WAV_RANGE_MIN = 2.2
    WAV_RANGE_MAX = 10.0
    
    def __init__(self):
        self._NAME = 'LOQ'
        super(LOQ, self).__init__()


    def set_component_positions(self, ws, xbeam, ybeam):
        """
            @param ws: workspace containing the instrument information
            @param xbeam: x-position of the beam
            @param ybeam: y-position of the beam
        """
        MoveInstrumentComponent(ws, 'some-sample-holder', Z = self.SAMPLE_Z_CORR, RelativePosition="1")
        
        xshift = (317.5/1000.) - xbeam
        yshift = (317.5/1000.) - ybeam
        MoveInstrumentComponent(ws, self.cur_detector().name(), X = xshift, Y = yshift, RelativePosition="1")
        # LOQ instrument description has detector at 0.0, 0.0
        return [xshift, yshift], [xshift, yshift]

    def get_marked_dets(self):
        raise NotImplementedError('The marked detector list isn\'t stored for instrument '+self._NAME)

    def set_up_for_sample(self, base_runno):
        """
            Needs to run whenever a sample is loaded
        """
        self.DETECTORS['low-angle'].set_orien('Horizontal')
        self.DETECTORS['high-angle'].set_orien('Horizontal')

    def load_transmission_inst(self, workspace):
        """
            Loads information about the setup used for LOQ transmission runs
        """
        trans_definition_file = mtd.getConfigProperty('instrumentDefinition.directory')
        trans_definition_file += '/'+self._NAME+'_trans_Definition.xml'
        LoadInstrument(workspace, trans_definition_file)


class SANS2D(ISISInstrument): 

    # Number of digits in standard file name
    run_number_width = 8
    WAV_RANGE_MIN = 2.0
    WAV_RANGE_MAX = 14.0

    def __init__(self):
        self._NAME = 'SANS2D'
        super(SANS2D, self).__init__()
        
        self._marked_dets = []
    
    def set_up_for_sample(self, base_runno):
        """
            Handles changes required when a sample is loaded, both generic
            and run specific
        """
        low = self.DETECTORS['low-angle']
        high = self.DETECTORS['high-angle']
        if base_runno < 568:
            self.set_incident_mon(73730)
            low.set_orien('Vertical')
            low.first_spec_num = 1
            low.last_spec_num = self._num_pixels*self._num_pixels
            high.set_orien('Vertical')
            high.first_spec_num = (self._num_pixels*self._num_pixels) + 1 
            high.last_spec_num = self._num_pixels*self._num_pixels*2
        elif (base_runno >= 568 and base_runno < 684):
            low.set_orien('Rotated')
            high.set_orien('Rotated')
        else:
            low.set_orien('Horizontal')
            high.set_orien('Horizontal')

    def set_component_positions(self, ws, xbeam, ybeam):
        """
            @param ws: workspace containing the instrument information
            @param xbeam: x-position of the beam
            @param ybeam: y-position of the beam
            
            #TODO: move the instrument parameters into this class
        """
        MoveInstrumentComponent(ws, 'some-sample-holder', Z = self.SAMPLE_Z_CORR, RelativePosition="1")
        
        if self.cur_detector().name() == 'front-detector':
            rotateDet = (-self.FRONT_DET_ROT - self.cur_detector().rot_corr)
            RotateInstrumentComponent(ws, self.cur_detector().name(), X="0.", Y="1.0", Z="0.", Angle=rotateDet)
            RotRadians = math.pi*(self.FRONT_DET_ROT + self.cur_detector().rot_corr)/180.
            xshift = (self.REAR_DET_X + self.other_detector().x_corr - self.FRONT_DET_X - self.cur_detector().x_corr + self.FRONT_DET_RADIUS*math.sin(RotRadians) )/1000. - self.FRONT_DET_DEFAULT_X_M - xbeam
            yshift = (self.cur_detector().y_corr/1000.  - ybeam)
            # default in instrument description is 23.281m - 4.000m from sample at 19,281m !
            # need to add ~58mm to det1 to get to centre of detector, before it is rotated.
            zshift = (self.FRONT_DET_Z + self.cur_detector().z_corr + self.FRONT_DET_RADIUS*(1 - math.cos(RotRadians)) )/1000.
            zshift -= self.FRONT_DET_DEFAULT_SD_M
            MoveInstrumentComponent(ws, self.cur_detector().name(), X = xshift, Y = yshift, Z = zshift, RelativePosition="1")
            return [0.0, 0.0], [0.0, 0.0]
        else:
            xshift = -xbeam
            yshift = -ybeam
            zshift = (self.REAR_DET_Z + self.cur_detector().z_corr)/1000.
            zshift -= self.REAR_DET_DEFAULT_SD_M
            mantid.sendLogMessage("::SANS:: Setup move "+str(xshift*1000.)+" "+str(yshift*1000.)+" "+str(zshift*1000.))
            MoveInstrumentComponent(ws, self.cur_detector().name(), X = xshift, Y = yshift, Z = zshift, RelativePosition="1")
            return [0.0,0.0], [xshift, yshift]
        
    # Load the detector logs
    def load_detector_logs(self,log_name,file_path):
    # Adding runs produces a 1000nnnn or 2000nnnn. For less copying, of log files doctor the filename
        self._marked_dets = []
        log_name = log_name[0:6] + '0' + log_name[7:]
        filename = os.path.join(file_path, log_name + '.log')

        # Build a dictionary of log data 
        logvalues = {}
        logvalues['Rear_Det_X'] = '0.0'
        logvalues['Rear_Det_Z'] = '0.0'
        logvalues['Front_Det_X'] = '0.0'
        logvalues['Front_Det_Z'] = '0.0'
        logvalues['Front_Det_Rot'] = '0.0'
        try:
            file_handle = open(filename, 'r')
        except IOError:
            mantid.sendLogMessage("::SANS::load_detector_logs: Log file \"" + filename + "\" could not be loaded.")
            return None
        
        for line in file_handle:
            parts = line.split()
            if len(parts) != 3:
                mantid.sendLogMessage('::SANS::load_detector_logs: Incorrect structure detected in logfile "' + filename + '" for line \n"' + line + '"\nEntry skipped')
            component = parts[1]
            if component in logvalues.keys():
                logvalues[component] = parts[2]

        file_handle.close()
        
        self.FRONT_DET_Z = float(logvalues['Front_Det_Z'])
        self.FRONT_DET_X = float(logvalues['Front_Det_X'])
        self.FRONT_DET_ROT = float(logvalues['Front_Det_Rot'])
        self.REAR_DET_Z = float(logvalues['Rear_Det_Z'])
        self.REAR_DET_X = float(logvalues['Rear_Det_X'])

        return logvalues
    
    def append_marked(self, detNames):
        self._marked_dets.append(detNames)

    def get_marked_dets(self):
        return self._marked_dets

if __name__ == '__main__':
    pass