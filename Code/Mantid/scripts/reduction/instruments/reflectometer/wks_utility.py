from numpy import zeros, arctan2, arange
from mantidsimple import *
import math

h = 6.626e-34 #m^2 kg s^-1
m = 1.675e-27 #kg

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

def getIndex(value, array):
    """
    returns the index where the value has been found
    """
#    sz = len(array)
#    for i in range(sz):
#        if value == array[i]:
#            return i
#    return -1
    return array.searchsorted(value)

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
        _h, units = getSh(mt, 's1t', 's1b') 
        return _h, units
    return None, ''
    
def getS2h(mt=None):
    """    
        returns the height and units of the slit #2 
    """
    if mt != None:
        _h, units = getSh(mt, 's2t', 's2b') 
        return _h, units
    return None, None

def getPixelXPixelY(mt1, maxX=304, maxY=256):
    """
        returns the PixelX_vs_PixelY array of the workspace data specified
    """
    pixelX_vs_pixelY = zeros((maxY, maxX))
    for x in range(maxX):
        for y in range(maxY):
            _index = maxY * x + y
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

def getPixelXTOF(mt1, maxX=304, maxY=256):
    """
        returns the PixelX_vs_TOF array of the workspace data specified
    """
    _init = mt1.readY(0)[:]
    pixelX_vs_tof = zeros((maxY, len(_init)))
    for x in range(maxX):
        for y in range(maxY):
            _index = maxY * x + y
            _array = mt1.readY(_index)[:]
            pixelX_vs_tof[y, :] += _array
    return pixelX_vs_tof

def createIntegratedWorkspace(mt1, outputWorkspace,
                              fromXpixel, toXpixel,
                              fromYpixel, toYpixel,
                              maxX=304, maxY=256):
    """
        This creates the integrated workspace over the second pixel range (304 here) and
        returns the new workspace handle
    """
    _tof_axis = mt1.readX(0)[:]
    _y_axis = zeros((maxY, len(_tof_axis) - 1))
    _y_error_axis = zeros((maxY, len(_tof_axis) - 1))
    
    x_size = toXpixel - fromXpixel + 1 
    x_range = arange(x_size) + fromXpixel
    
    y_size = toYpixel - fromYpixel + 1
    y_range = arange(y_size) + fromYpixel
    
    for x in x_range:
        for y in y_range:
            _index = int((maxY) * x + y)
            _y_axis[y, :] += mt1.readY(_index)[:]
            _y_error_axis[y, :] += ((mt1.readE(_index)[:]) * (mt1.readE(_index)[:]))

    _y_axis = _y_axis.flatten()
    _y_error_axis = numpy.sqrt(_y_error_axis)
    #plot_y_error_axis = _y_error_axis #for output testing only    -> plt.imshow(plot_y_error_axis, aspect='auto', origin='lower')
    _y_error_axis = _y_error_axis.flatten()

    #normalization by proton charge
#    _y_axis /= (proton_charge * 1e-12)

    CreateWorkspace(OutputWorkspace=outputWorkspace, 
                    DataX=_tof_axis, DataY=_y_axis, DataE=_y_error_axis, Nspec=maxY,
                    UnitX="TOF",ParentWorkspace=mt1)
    mt2 = mtd[outputWorkspace]
    return mt2

def angleUnitConversion(value, from_units='degree', to_units='rad'):
    """
        This function converts the angle units
        
    """
    
    if (from_units == to_units):
        return value;

    from_factor = 1;
    #convert everything into rad
    if (from_units == 'degree'):
        from_factor = 1.745329252e-2;
    value_rad = from_factor * value;
    
    if (to_units == 'rad'):
        return value_rad;
    else:
        to_factor = 57.2957795;
        return to_factor * value_rad;
    
def convertToThetaVsLambda(_tof_axis,
                           _pixel_axis,
                           central_pixel,
                           pixel_size=0.0007,
                           theta= -1,
                           dSD= -1,
                           dMD= -1):
    """
    This function converts the pixel/tof array
    to theta/lambda
    
    """
    h = 6.626e-34 #m^2 kg s^-1
    m = 1.675e-27 #kg

    #convert tof_axis into seconds
    _tof_axis = _tof_axis * 1e-6

    vel_array = dMD / _tof_axis         #mm/ms = m/s
    _lambda = h / (m * vel_array)  #m
    _lambda = _lambda * 1e10  #angstroms
  
    d_vec = (_pixel_axis - central_pixel) * pixel_size
    theta_vec = arctan2(d_vec,dSD) + theta

    dico = {'lambda_vec': _lambda, 'theta_vec': theta_vec}
    
    return dico

def _convertToRvsQ(_tof_axis,
                  _pixel_axis_of_peak,
                  data_of_peak,
                  central_pixel, 
                  pixel_size=0.0007,
                  theta=-1,
                  dSD=-1,
                  dMD=-1):
    """
    This function converts the pixel/tof array to the R(Q) array
    
    """

    h = 6.626e-34 #m^2 kg s^-1
    m = 1.675e-27 #kg

    ##convert tof_axis into seconds
    #_tof_axis = _tof_axis * 1e-6

#    vel_array = dMD / _tof_axis         #mm/ms = m/s
#    _lambda = h / (m * vel_array)  #m
#    _lambda = _lambda * 1e10  #angstroms
  
    d_vec = (_pixel_axis_of_peak - central_pixel) * pixel_size
    theta_vec = arctan2(d_vec,dSD) + theta
    
    #create Q axis
    nbr_pixel = shape(_pixel_axis_of_peak)[0]
    nbr_tof = shape(_tof_axis)[0]
    q_array = zeros(nbr_tof)
    
#    print 'dMD: ' , dMD
    _const = float(4) * math.pi * m * dMD / h
#    print '_const: ' , _const
#    print 'theta: ' , theta
#    print _tof_axis
    
    for t in range(nbr_tof):
        _Q = _const * sin(theta) / _tof_axis[t]
        q_array[t] = _Q
    
    return q_array


def convertToRvsQ(dMD=-1,theta=-1,tof=None):
    """
    This function converts the pixel/TOF array to the R(Q) array
    using Q = (4.Pi.Mn)/h  *  L.sin(theta/2)/TOF
    with    L: distance central_pixel->source
            TOF: TOF of pixel
            theta: angle of detector
    """
    _const = float(4) * math.pi * m * dMD / h
    sz_tof = numpy.shape(tof)[0]
    q_array = zeros(sz_tof-1)
    for t in range(sz_tof-1):
        tof1 = tof[t]
        tof2 = tof[t+1]
        tofm = (tof1+tof2)/2.
        _Q = _const * math.sin(theta) / (tofm*1e-6)
        q_array[t] = _Q*1e-10
    
    return q_array
