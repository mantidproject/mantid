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

class DetectorBank :
    def __init__(self, instr, det_type) :
        self.parent = instr
        #detectors are known by many names
        self._names = {
          'uni' : det_type,
          'long': instr.getStringParameter(det_type+'-detector-name')[0],
          'short': instr.getStringParameter(det_type+'-detector-short-name')[0]}
        
        spec_entry = instr.getNumberParameter('first-low-angle-spec-number')
        if len(spec_entry) > 0 :
            self.first_spec_num = int(spec_entry[0])
        else :
            #'first-low-angle-spec-number' is an optimal instrument parameter
            self.first_spec_num = 0
        
        self.n_columns = int(
            instr.getNumberParameter('low-angle-detector-num-columns')[0])
        n_rows = int(
            instr.getNumberParameter('low-angle-detector-num-rows')[0])

        self._num_pixels = int(self.n_columns*n_rows)
        self.last_spec_num = self.first_spec_num + self._num_pixels - 1

    def place_after(self, previousDet):
        self.first_spec_num = previousDet.last_spec_num + 1
        self.last_spec_num = self.first_spec_num + self._num_pixels - 1

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
    
class Instrument(object) :
    def __init__(self) :
    
        temp_WS_name = '_'+self._NAME+'instrument_definition'
        #read the information about the instrument that stored in it's xml
        LoadEmptyInstrument(
            mtd.getConfigProperty('instrumentDefinition.directory')+'/'+self._NAME+'_Definition.xml',
            temp_WS_name)
        definitionWS = mtd[temp_WS_name]  
        self.definition = definitionWS.getInstrument()
        mtd.deleteWorkspace(temp_WS_name)
        
        #the spectrum with this number is used to normalise the workspace data
        self._scattering_mon = int(self.definition.getNumberParameter(
            'default-scattering-monitor-spec')[0])
        #this is used by suggest_scattering_mon() below 
        self._scattering_mon_locked = False
        
    def suggest_scattering_mon(self, scattering_mon):
        """
            Only sets the monitor spectrum number if it isn't locked, used
            so MON/SPECTRUM in user files don't change MON/LENGTH settings
            @param the set the monitor spectrum number to be that with this number, regardless of lock 
        """
        if not self._scattering_mon_locked :
            self._scattering_mon = int(scattering_mon)
        
    def get_scattering_mon(self):
        """
            @return the spectrum number of the scattering monitor
        """
        return self._scattering_mon
        
    def set_scattering_mon(self, scattering_mon):
        """
            @param the set the monitor spectrum number to be that with this number, regardless of lock 
        """
        self._scattering_mon = int(scattering_mon)
        self._scattering_mon_locked = True
        
    def set_component_positions(self, ws, xbeam, ybeam): raise NotImplemented
        
    def set_sample_offset(self, value):
        """
            @param value: sample value offset
        """
        self.SAMPLE_Z_CORR = float(value)/1000.
        
class ISISInstrument(Instrument) :
    def __init__(self) :
        Instrument.__init__(self)
    
        firstDetect = DetectorBank(self.definition, 'low-angle')
        secondDetect = DetectorBank(self.definition, 'high-angle')
        secondDetect.place_after(firstDetect)
        self.DETECTORS = {'low-angle' : firstDetect}
        self.DETECTORS['high-angle'] = secondDetect
    
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

        # MASK file stuff ==========================================================
        # correction terms to SANS2d encoders - store in MASK file ?
        self.FRONT_DET_Z_CORR = 0.0
        self.FRONT_DET_Y_CORR = 0.0 
        self.FRONT_DET_X_CORR = 0.0 
        self.FRONT_DET_ROT_CORR = 0.0
        self.REAR_DET_Z_CORR = 0.0 
        self.REAR_DET_X_CORR = 0.0

        #the low anlge detector is some times called the main detector, some times the rear detector
        self.lowAngDetSet = True
        # if this is set InterpolationRebin will be used on the monitor spectrum used to normalise the sample
        self._scattering_mon_interp = False

    def name(self):
        return self._NAME
        
    def is_scattering_mon_interp(self):
        return self._scattering_mon_interp
     
    def interp_scattering_mon(self):
        """
            This method sets that the monitor spectrum should be interpolated, there is currently no unset setting the instrument will need to be reset for that    
        """
        self._scattering_mon_interp = True
     
    def suggest_interp_scattering_mon(self):
        if not self._scattering_mon_locked :
            self._scattering_mon_interp = True
     
    def cur_detector(self) :
        if self.lowAngDetSet : return self.DETECTORS['low-angle']
        else : return self.DETECTORS['high-angle']
    
    def otherDetector(self) :
        if not self.lowAngDetSet : return self.DETECTORS['low-angle']
        else : return self.DETECTORS['high-angle']
    
    def listDetectors(self) :
        return self.cur_detector().name(), self.otherDetector().name()
        
    def isHighAngleDetector(self, detName) :
        if self.DETECTORS['high-angle'].isAlias(detName) :
            return True

    def isDetectorName(self, detName) :
        if self.otherDetector().isAlias(detName) :
            return True
        
        return self.cur_detector().isAlias(detName)

    def setDetector(self, detName) :
        if self.otherDetector().isAlias(detName) :
            self.lowAngDetSet = not self.lowAngDetSet
            return True
        
        return self.cur_detector().isAlias(detName)

    def setDefaultDetector(self):
        self.lowAngDetSet = True
        
class LOQ(ISISInstrument):
    def __init__(self):
        self._NAME = "LOQ"
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
        
class SANS2D(ISISInstrument): 

    def __init__(self):
        self._NAME = "SANS2D"
        super(SANS2D, self).__init__()
        
    def set_component_positions(self, ws, xbeam, ybeam):
        """
            @param ws: workspace containing the instrument information
            @param xbeam: x-position of the beam
            @param ybeam: y-position of the beam
            
            #TODO: move the instrument parameters into this class
        """
        MoveInstrumentComponent(ws, 'some-sample-holder', Z = self.SAMPLE_Z_CORR, RelativePosition="1")
        
        if self.cur_detector().name() == 'front-detector':
            rotateDet = (-self.FRONT_DET_ROT - self.FRONT_DET_ROT_CORR)
            RotateInstrumentComponent(ws, self.cur_detector().name(), X="0.", Y="1.0", Z="0.", Angle=rotateDet)
            RotRadians = math.pi*(self.FRONT_DET_ROT + self.FRONT_DET_ROT_CORR)/180.
            xshift = (self.REAR_DET_X + self.REAR_DET_X_CORR - self.FRONT_DET_X - self.FRONT_DET_X_CORR + self.FRONT_DET_RADIUS*math.sin(RotRadians) )/1000. - self.FRONT_DET_DEFAULT_X_M - xbeam
            yshift = (self.FRONT_DET_Y_CORR /1000.  - ybeam)
            # default in instrument description is 23.281m - 4.000m from sample at 19,281m !
            # need to add ~58mm to det1 to get to centre of detector, before it is rotated.
            zshift = (self.FRONT_DET_Z + self.FRONT_DET_Z_CORR + self.FRONT_DET_RADIUS*(1 - math.cos(RotRadians)) )/1000. - self.FRONT_DET_DEFAULT_SD_M
            MoveInstrumentComponent(ws, self.cur_detector().name(), X = xshift, Y = yshift, Z = zshift, RelativePosition="1")
            return [0.0, 0.0], [0.0, 0.0]
        else:
            xshift = -xbeam
            yshift = -ybeam
            zshift = (self.REAR_DET_Z + self.REAR_DET_Z_CORR)/1000. - self.REAR_DET_DEFAULT_SD_M
            mantid.sendLogMessage("::SANS:: Setup move "+str(xshift*1000.)+" "+str(yshift*1000.))
            MoveInstrumentComponent(ws, self.cur_detector().name(), X = xshift, Y = yshift, Z = zshift, RelativePosition="1")
            return [0.0,0.0], [xshift, yshift]


# The following should be refactored away:
all = {"LOQ" : LOQ(), "SANS2D" : SANS2D()}
  
if __name__ == '__main__':
    test = SANS2D()
    print test.listDetectors()
