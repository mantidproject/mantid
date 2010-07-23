##########################################################################
# holds infomration that is unlikely change about SANS2D and LOQ and
# functions and classes to make accessing that data easier 
##########################################################################
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
#######################################################################
# Holds data about an instrument, like SANS2D, and DetectorStats objects
# that corrospond to its detector banks
class Instrument :
    def __init__(self, inst) :
        self.name = inst
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

_loqInfo = Instrument('LOQ')
_loqInfo.N_MONITORS = 2
_loqInfo.DETECTORS = {'LOW_ANGLE' : DetectorBank(('main-detector-bank', 'main'), 'LOQ', _loqInfo.N_MONITORS) }
_loqInfo.DETECTORS['HIGH_ANGLE'] = DetectorBank(('HAB', 'HAB'), 'LOQ', (128**2)+_loqInfo.N_MONITORS)

_sans2DInfo = Instrument('SANS2D')
_sans2DInfo.N_MONITORS = 4
_sans2DInfo.DETECTORS = {'LOW_ANGLE' : DetectorBank(('rear-detector', 'rear'), 'SANS2D', _sans2DInfo.N_MONITORS)}
_sans2DInfo.DETECTORS['HIGH_ANGLE'] = DetectorBank(('front-detector', 'front'), 'SANS2D', (192**2)+_sans2DInfo.N_MONITORS)
    
allInsts = {'LOQ' : _loqInfo, 'SANS2D' : _sans2DInfo}
#remove this copy
all = allInsts

#####################################################################################
#module level variables that hold the currently selected instrument and detector
#####################################################################################
_curInst = allInsts['SANS2D']
_curDetector = _curInst.curDetector()

#to make the code more robust to errors use the following getter and set methods instead of setting _curInst, etc
def getCurDetector() :
    return _curDetector

def getCurInst() :
    return _curInst
#######################################################################################################
# can rasie KeyError if the name that was passed is not in the instrument dictonary
######################################################################################################
def setCurInst(name) :
    global _curInst
    _curInst = allInsts[name]

def setDetector(detName) :
    global _curInst
    if _curInst.setDetector(detName) :
        global curDetector
        curDetector = _curInst.curDetector()
        return True
    return False

def setDefaultDetector() :
   _curInst.setDefaultDetector()
