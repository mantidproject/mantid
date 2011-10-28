from numpy import zeros
from pylab import *
import matplotlib.pyplot as plt

def getProtonCharge(st=None):
    """
        Returns the proton charge of the given workspace in picoCoulomb
    """
    if st is not None:
        mt_run = st.getRun()
        proton_charge_mtd_unit = mt_run.getProperty('gd_prtn_chrg').value
        proton_charge = proton_charge_mtd_unit / 2.77777778e-10
        return proton_charge
    return None  

def get_index(value, array):
    """
    returns the index where the value has been found
    """
    sz = len(array)
    for i in range(sz):
        if value == array[i]:
            return i
    return -1

def getSh(mt, top_tag, bottom_tag):
    """
        returns the height and units of the given slit#
    """
    mt_run = mt.getRun()
    st = mt_run.getProperty(top_tag).value
    sb = mt_run.getProperty(bottom_tag).value
    sh = float(sb[0]) - float(st[0])
    units = mt_run.getProperty(top_tag).units
    return sh, units
    
def getS1h(mt=None):
    """    
        returns the height and units of the slit #1 
    """
    if mt != None:
        _h,units = getSh(mt,'s1t','s1b') 
        return _h,units
    return None,''
    
def getS2h(mt=None):
    """    
        returns the height and units of the slit #2 
    """
    if mt != None:
        _h,units = getSh(mt,'s2t','s2b') 
        return _h,units
    return None,None

def getPixelXPixelY(mt1):
    """
        returns the PixelX_vs_PixelY array of the workspace data specified
    """
    pixelX_vs_pixelY = zeros((256, 304))
    for x in range(304):
        for y in range(256):
            _index = 256 * x + y
            _sum = sum(mt1.readY(_index)[:])
            pixelX_vs_pixelY[y, x] = _sum
    return pixelX_vs_pixelY

def getPixelXPixelYError(mt1):
    """
        returns the PixelX_vs_PixelY_error array of the workspace data specified
    """
    pixel_error = zeros((256, 304))
    for x in range(304):
        for y in range(256):
            _index = 256 * x + y
            _sum = sum(mt1.readE(_index)[:])
            pixel_error[y, x] = _sum       
    return pixel_error

def getPixelXTOF(mt1):
    """
        returns the PixelX_vs_TOF array of the workspace data specified
    """
    _init = mt1.readY(0)[:]
    pixelX_vs_tof = zeros((256, len(_init)))
    for x in range(304):
        for y in range(256):
            _index = 256 * x + y
            _array = mt1.readY(_index)[:]
            pixelX_vs_tof[y, :] += _array
    return pixelX_vs_tof

