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
    def __init__(self, longAndShort, inst, initialSpectrum) :
        self._names = dict([('long', longAndShort[0]), ('short', longAndShort[1])])
        self.belongsTo = inst
        self.firstSpec = initialSpectrum

    def name(self, form = 'long') :
        if form.lower() == 'inst_view' : form = 'long'
        if not self._names.has_key(form) : form = 'long'
        
        return self._names[form]
        
    ############################################################
    # detectors are often refered to by more than one name, check
    # if the supplied name is in the list
    ##############################################################
    def isAlias(self, guess) :
        for name in self._names.values() :
            if guess.lower() == name.lower() :
                return True
        return False
    
class Instrument(object) :
    def __init__(self) :
        
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

    def curDetector(self) :
        if self.lowAngDetSet : return self.DETECTORS['LOW_ANGLE']
        else : return self.DETECTORS['HIGH_ANGLE']
    
    def otherDetector(self) :
        if not self.lowAngDetSet : return self.DETECTORS['LOW_ANGLE']
        else : return self.DETECTORS['HIGH_ANGLE']
    
    def listDetectors(self) :
        return self.curDetector().name(), self.otherDetector().name()
        
    def isHighAngleDetector(self, detName) :
        if self.DETECTORS['HIGH_ANGLE'].isAlias(detName) :
            return True

    def isDetectorName(self, detName) :
        if self.otherDetector().isAlias(detName) :
            return True
        
        return self.curDetector().isAlias(detName)

    def setDetector(self, detName) :
        if self.otherDetector().isAlias(detName) :
            self.lowAngDetSet = not self.lowAngDetSet
            return True
        
        return self.curDetector().isAlias(detName)

    def setDefaultDetector(self):
        self.lowAngDetSet = True
        
    def set_component_positions(self, ws, xbeam, ybeam): raise NotImplemented
        
    def set_sample_offset(self, value):
        """
            @param value: sample value offset
        """
        self.SAMPLE_Z_CORR = float(value)/1000.
        
class LOQ(Instrument):
    name = "LOQ"
    N_MONITORS = 2
    
    def __init__(self):
        super(LOQ, self).__init__()
        self.DETECTORS = {'LOW_ANGLE' : DetectorBank(('main-detector-bank', 'main'), 'LOQ', LOQ.N_MONITORS),
                          'HIGH_ANGLE' : DetectorBank(('HAB', 'HAB'), 'LOQ', (128**2)+LOQ.N_MONITORS)}

    def set_component_positions(self, ws, xbeam, ybeam):
        """
            @param ws: workspace containing the instrument information
            @param xbeam: x-position of the beam
            @param ybeam: y-position of the beam
        """
        MoveInstrumentComponent(ws, 'some-sample-holder', Z = self.SAMPLE_Z_CORR, RelativePosition="1")
        
        xshift = (317.5/1000.) - xbeam
        yshift = (317.5/1000.) - ybeam
        MoveInstrumentComponent(ws, self.curDetector().name(), X = xshift, Y = yshift, RelativePosition="1")
        # LOQ instrument description has detector at 0.0, 0.0
        return [xshift, yshift], [xshift, yshift] 
        
        

class SANS2D(Instrument): 
    name = "SANS2D"
    N_MONITORS = 4

    def __init__(self):
        super(SANS2D, self).__init__()        
        self.DETECTORS = {'LOW_ANGLE' : DetectorBank(('rear-detector', 'rear'), 'SANS2D', SANS2D.N_MONITORS),
                          'HIGH_ANGLE' : DetectorBank(('front-detector', 'front'), 'SANS2D', (192**2)+SANS2D.N_MONITORS)}

    def set_component_positions(self, ws, xbeam, ybeam):
        """
            @param ws: workspace containing the instrument information
            @param xbeam: x-position of the beam
            @param ybeam: y-position of the beam
            
            #TODO: move the instrument parameters into this class
        """
        MoveInstrumentComponent(ws, 'some-sample-holder', Z = self.SAMPLE_Z_CORR, RelativePosition="1")
        
        if self.curDetector().name() == 'front-detector':
            rotateDet = (-self.FRONT_DET_ROT - self.FRONT_DET_ROT_CORR)
            RotateInstrumentComponent(ws, self.curDetector().name(), X="0.", Y="1.0", Z="0.", Angle=rotateDet)
            RotRadians = math.pi*(self.FRONT_DET_ROT + self.FRONT_DET_ROT_CORR)/180.
            xshift = (self.REAR_DET_X + self.REAR_DET_X_CORR - self.FRONT_DET_X - self.FRONT_DET_X_CORR + self.FRONT_DET_RADIUS*math.sin(RotRadians) )/1000. - self.FRONT_DET_DEFAULT_X_M - xbeam
            yshift = (self.FRONT_DET_Y_CORR /1000.  - ybeam)
            # default in instrument description is 23.281m - 4.000m from sample at 19,281m !
            # need to add ~58mm to det1 to get to centre of detector, before it is rotated.
            zshift = (self.FRONT_DET_Z + self.FRONT_DET_Z_CORR + self.FRONT_DET_RADIUS*(1 - math.cos(RotRadians)) )/1000. - self.FRONT_DET_DEFAULT_SD_M
            MoveInstrumentComponent(ws, self.curDetector().name(), X = xshift, Y = yshift, Z = zshift, RelativePosition="1")
            return [0.0, 0.0], [0.0, 0.0]
        else:
            xshift = -xbeam
            yshift = -ybeam
            zshift = (self.REAR_DET_Z + self.REAR_DET_Z_CORR)/1000. - self.REAR_DET_DEFAULT_SD_M
            mantid.sendLogMessage("::SANS:: Setup move "+str(xshift*1000.)+" "+str(yshift*1000.))
            MoveInstrumentComponent(ws, self.curDetector().name(), X = xshift, Y = yshift, Z = zshift, RelativePosition="1")
            return [0.0,0.0], [xshift, yshift]



# The following should be refactored away:
all = {"LOQ" : LOQ(), "SANS2D" : SANS2D()}