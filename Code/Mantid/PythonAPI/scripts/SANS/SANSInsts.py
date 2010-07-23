import sys

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
        
        
class LOQ(Instrument):
    name = "LOQ"
    N_MONITORS = 2
    
    def __init__(self):
        super(LOQ, self).__init__()
        self.DETECTORS = {'LOW_ANGLE' : DetectorBank(('main-detector-bank', 'main'), 'LOQ', LOQ.N_MONITORS),
                          'HIGH_ANGLE' : DetectorBank(('HAB', 'HAB'), 'LOQ', (128**2)+LOQ.N_MONITORS)}


class SANS2D(Instrument): 
    name = "SANS2D"
    N_MONITORS = 4

    def __init__(self):
        super(SANS2D, self).__init__()
        self.DETECTORS = {'LOW_ANGLE' : DetectorBank(('rear-detector', 'rear'), 'SANS2D', SANS2D.N_MONITORS),
                          'HIGH_ANGLE' : DetectorBank(('front-detector', 'front'), 'SANS2D', (192**2)+SANS2D.N_MONITORS)}
