from numpy import zeros, arctan2, arange, shape
from mantidsimple import *
from MantidFramework import *
import math
import os.path

h = 6.626e-34 #m^2 kg s^-1
m = 1.675e-27 #kg

def getSequenceRuns(run_numbers):
    """
    This will return the sequence of runs
    ex:
        input: 10,11,12
        output: 10,11,12
        
        input: 10,13-15
        output: 10,13,14,15
    """
    final_list = []
    for _run in run_numbers:
        _run = str(_run)
        _result = _run.find('-')
        if _result == -1:
            final_list.append(_run)
        else:
            _split = _run.split('-')
            start = int(_split[0])
            end = int(_split[1])
            _range = arange(end-start+1)+start
            for _r in _range:
                final_list.append(_r)
    return final_list

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
    sh = math.fabs(float(sb[0]) - float(st[0]))
    units = mt_run.getProperty(top_tag).units
    return sh, units

def getSheight(mt, index):
    """
        return the DAS hardware slits height of slits # index
    """
    mt_run = mt.getRun()
    tag = 'S' + index + 'VHeight'
    value = mt_run.getProperty(tag).value
    return value[0]
        
def getS1h(mt=None):
    """    
        returns the height and units of the slit #1 
    """
    if mt != None:
#        _h, units = getSh(mt, 's1t', 's1b')
        _h = getSheight(mt, '1') 
        return _h
    return None
    
def getS2h(mt=None):
    """    
        returns the height and units of the slit #2 
    """
    if mt != None:
#        _h, units = getSh(mt, 's2t', 's2b')
        _h = getSheight(mt, '2') 
        return _h
    return None




def getSwidth(mt, index):
    """
        returns the width and units of the given index slits
        defined by the DAS hardware
    """
    mt_run = mt.getRun()
    tag = 'S' + index + 'HWidth'
    value = mt_run.getProperty(tag).value
    return value[0]

def getSw(mt, left_tag, right_tag):
    """
        returns the width and units of the given slits
    """
    mt_run = mt.getRun()
    sl = mt_run.getProperty(left_tag).value
    sr = mt_run.getProperty(right_tag).value
    sw = math.fabs(float(sl[0]) - float(sr[0]))
    units = mt_run.getProperty(left_tag).units
    return sw, units

def getS1w(mt=None):
    """    
        returns the width and units of the slit #1 
    """
    if mt != None:
#        _w, units = getSw(mt, 's1l', 's1r') 
        _w = getSwidth(mt, '1')
        return _w
    return None
    
def getS2w(mt=None):
    """    
        returns the width and units of the slit #2 
    """
    if mt != None:
#        _w, units = getSh(mt, 's2l', 's2r') 
        _w = getSwidth(mt, '2')
        return _w
    return None


def getLambdaValue(mt):
    """
    return the lambdaRequest value
    """
    mt_run = mt.getRun()
    _lambda = mt_run.getProperty('LambdaRequest').value
    return _lambda


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

def findQaxisMinMax(q_axis):
    """
        Find the position of the common Qmin and Qmax in 
        each q array
    """
    
    nbr_row = shape(q_axis)[0]
    nbr_col = shape(q_axis)[1]
    q_min = min(q_axis[0])
    q_max = max(q_axis[0])
    
    for i in arange(nbr_row - 1) + 1:
        _q_min = q_axis[i][-1]
        _q_max = q_axis[i][0]
        if (_q_min > q_min):
            q_min = _q_min
        if (_q_max < q_max):
            q_max = _q_max
    
    #find now the index of those min and max in each row
    _q_axis_min_max_index = zeros((nbr_row, 2))
    for i in arange(nbr_row):
        _q_axis = q_axis[i]
        for j in arange(nbr_col - 1):
            _q = q_axis[i, j]
            _q_next = q_axis[i, j + 1]
            if (_q >= q_max) and (_q_next <= q_max):
                _q_axis_min_max_index[i, 0] = j
            if (_q >= q_min) and (_q_next <= q_min):
                _q_axis_min_max_index[i, 1] = j

    return _q_axis_min_max_index


def cleanup_data(InputWorkspace=None,
                 OutputWorkspace=None,
                 maxY=256):
    mti = mtd[InputWorkspace]
    _tof_axis = mti.readX(0)[:]
    nbr_tof = shape(_tof_axis)[0]-1
    
    tof_range = range(nbr_tof-1)
    x_range = range(maxY)

    _new_y = zeros((maxY, nbr_tof))
    _new_e = zeros((maxY, nbr_tof))
    for px in x_range:
        for tof in tof_range:
            _y = mti.readY(px)[tof]
            if _y != 0:
                _e = mti.readE(px)[tof]
                _y2 = _y * _y
#                if _y < _e:
                if _y < 0 or _y < _e:
                    _y = 0.
                    _e = 0.
                _new_y[px,tof] = float(_y)
                _new_e[px,tof] = float(_e)

    _y_error_axis = _new_e.flatten()
    _y_axis = _new_y.flatten()

    CreateWorkspace(OutputWorkspace=OutputWorkspace,
                    DataX=_tof_axis,
                    DataY=_y_axis,
                    DataE=_y_error_axis,
                    Nspec=maxY,
                    UnitX="TOF",
                    ParentWorkspace=mti)

def createIntegratedWorkspace(mt1, outputWorkspace,
                              fromXpixel, toXpixel,
                              fromYpixel, toYpixel,
                              maxX=304, maxY=256,
                              bCleaning=False):
    """
        This creates the integrated workspace over the second pixel range (304 here) and
        returns the new workspace handle
    """

    _tof_axis = mt1.readX(0)[:]
    nbr_tof = len(_tof_axis)
    t_range = arange(nbr_tof-1)
    
    _fromXpixel = min([fromXpixel, toXpixel])
    _toXpixel = max([fromXpixel, toXpixel])
    fromXpixel = _fromXpixel
    toXpixel = _toXpixel

    _fromYpixel = min([fromYpixel, toYpixel])
    _toYpixel = max([fromYpixel, toYpixel])
    fromYpixel = _fromYpixel
    toYpixel = _toYpixel

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
    _y_error_axis = _y_error_axis.flatten()

    CreateWorkspace(OutputWorkspace=outputWorkspace,
                    DataX=_tof_axis,
                    DataY=_y_axis,
                    DataE=_y_error_axis,
                    Nspec=maxY,
                    UnitX="TOF",
                    ParentWorkspace=mt1)
    
def convertWorkspaceToQ(ws_data,
                        outputWorkspace,
                        fromYpixel, toYpixel,
                        maxX=304, maxY=256,
                        cpix=None,
                        source_to_detector=None,
                        sample_to_detector=None,
                        theta=None,
                        geo_correction=False,
                        q_binning=None):
    """
        This creates the integrated workspace over the second pixel range (304 here) and
        returns the new workspace handle
    """

    mt1 = mtd[ws_data]
    _tof_axis = mt1.readX(0)[:]    
    _fromYpixel = min([fromYpixel, toYpixel])
    _toYpixel = max([fromYpixel, toYpixel])
    fromYpixel = _fromYpixel
    toYpixel = _toYpixel

    if geo_correction:
        
        yrange = arange(toYpixel - fromYpixel + 1) + fromYpixel
        _q_axis = convertToRvsQWithCorrection(mt1,
                                              dMD=source_to_detector,
                                              theta=theta,
                                              tof=_tof_axis,
                                              yrange=yrange,
                                              cpix=cpix)

        #find the common Qmin and Qmax values and their index (position)
        #in each _q_axis row
        _q_axis_min_max_index = findQaxisMinMax(_q_axis)
        
        #replace the _q_axis of the yrange of interest by the new 
        #individual _q_axis
        y_size = toYpixel - fromYpixel + 1
        y_range = arange(y_size) + fromYpixel
 
        _y_axis = zeros((y_size, len(_tof_axis) - 1))
        _y_error_axis = zeros((y_size, len(_tof_axis) - 1))
           
        #now determine the y_axis    
        for _q_index in range(y_size):
            
            _tmp_q_axis = _q_axis[_q_index]
            q_axis = _tmp_q_axis[::-1] #reverse the axis (now increasing order)

            _a = yrange[_q_index]
            _y_axis_tmp = list(mt1.readY(int(_a))[:])
            _y_error_axis_tmp = list(mt1.readE(int(_a))[:])

            #keep only the overlap region of Qs
            _q_min = _q_axis_min_max_index[_q_index, 0]
            if (_q_min != 0):
                _y_axis_tmp[0:_q_min] = 0
                _y_error_axis_tmp[0:_q_min] = 0

            _q_max = int(_q_axis_min_max_index[_q_index, 1])
            sz = shape(_y_axis_tmp)[0]
            if (_q_max != sz):
                _index_q_max_range = arange(sz - _q_max) + _q_max
                for i in _index_q_max_range:
                    _y_axis_tmp[i] = 0
                    _y_error_axis_tmp[i] = 0

            _y_axis[_q_index, :] = _y_axis_tmp[::-1]
            _y_error_axis[_q_index, :] = _y_error_axis_tmp[::-1]
            
        x_axis = q_axis.flatten()        
        y_axis = _y_axis.flatten()
        y_error_axis = _y_error_axis.flatten()
            
        CreateWorkspace(OutputWorkspace=outputWorkspace,
                        DataX=x_axis,
                        DataY=y_axis,
                        DataE=y_error_axis,
                        Nspec=y_size,
                        UnitX="MomentumTransfer",
                        ParentWorkspace=mt1)
        
        mtd[outputWorkspace].setDistribution(True)
        
        Rebin(InputWorkspace=outputWorkspace,
              OutputWorkspace=outputWorkspace,
              Params=q_binning)

    else:
        
        if source_to_detector is not None and theta is not None:
            _const = float(4) * math.pi * m * source_to_detector / h
            _q_axis = 1e-10 * _const * math.sin(theta) / (_tof_axis * 1e-6)
        else:
            _q_axis = _tof_axis

        _y_axis = zeros((maxY, len(_q_axis) - 1))
        _y_error_axis = zeros((maxY, len(_q_axis) - 1))
    
        y_size = toYpixel - fromYpixel + 1
        y_range = arange(y_size) + fromYpixel
    
        for y in y_range:
            _y_axis[int(y), :] = mt1.readY(int(y))[:]
            _y_error_axis[int(y), :] = mt1.readE(int(y))[:]

        _y_axis = _y_axis.flatten()
        _y_error_axis = _y_error_axis.flatten()

        _q_axis = _q_axis[::-1]
        _y_axis = _y_axis[::-1]
        _y_error_axis = _y_error_axis[::-1]

        CreateWorkspace(OutputWorkspace=outputWorkspace,
                        DataX=_q_axis,
                        DataY=_y_axis,
                        DataE=_y_error_axis,
                        Nspec=maxY,
                        UnitX="MomentumTransfer",
                        ParentWorkspace=mt1)

        mtd[outputWorkspace].setDistribution(True)

        Rebin(InputWorkspace=outputWorkspace,
              OutputWorkspace=outputWorkspace,
              Params=q_binning)        
            
def create_grouping(workspace=None, xmin=0, xmax=None, filename=".refl_grouping.xml"):
    # This should be read from the 
    npix_x = 304
    npix_y = 256
    if workspace is not None:
        if mtd[workspace].getInstrument().hasParameter("number-of-x-pixels"):
            npix_x = int(mtd[workspace].getInstrument().getNumberParameter("number-of-x-pixels")[0])
        if mtd[workspace].getInstrument().hasParameter("number-of-y-pixels"):
            npix_y = int(mtd[workspace].getInstrument().getNumberParameter("number-of-y-pixels")[0])
    
    f = open(filename, 'w')
    f.write("<detector-grouping description=\"Integrated over X\">\n")
    
    if xmax is None:
        xmax = npix_x
        
    for y in range(npix_y):
        # index = max_y * x + y
        indices = []
        for x in range(xmin, xmax + 1):
            indices.append(str(npix_y * x + y))
        
        # Detector IDs start at zero, but spectrum numbers start at 1
        # Grouping works on spectrum numbers
        indices_str = ','.join(indices)
        f.write("  <group name='%d'>\n" % y)
        f.write("    <ids val='%s'/>\n" % indices_str)
        f.write("  </group>\n")
    
    f.write("</detector-grouping>\n")
    f.close()

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
    theta_vec = arctan2(d_vec, dSD) + theta

    dico = {'lambda_vec': _lambda, 'theta_vec': theta_vec}
    
    return dico

def convertToRvsQWithCorrection(mt, dMD= -1, theta= -1, tof=None, yrange=None, cpix=None):
    """
    This function converts the pixel/TOF array to the R(Q) array
    using Q = (4.Pi.Mn)/h  *  L.sin(theta/2)/TOF
    with    L: distance central_pixel->source
            TOF: TOF of pixel
            theta: angle of detector
    """

    h = 6.626e-34 #m^2 kg s^-1
    m = 1.675e-27 #kg

    sample = mt.getInstrument().getSample()
    source = mt.getInstrument().getSource()
    dSM = sample.getDistance(source)

    maxX = 304
    maxY = 256
    
    dPS_array = zeros(maxY)
    for y in range(maxY):
        detector = mt.getDetector(y)
        dPS_array[y] = sample.getDistance(detector)

    #array of distances pixel->source
    dMP_array = dPS_array + dSM
    #distance sample->center of detector
    dSD = dPS_array[maxY / 2]

    _const = float(4) * math.pi * m * dMD / h
    sz_tof = len(tof)
    q_array = zeros((len(yrange), sz_tof - 1))

    yrange = range(len(yrange))
    for y in yrange:
        _px = yrange[y]
        dangle = ref_beamdiv_correct(cpix, mt, dSD, _px)
        
        if dangle is not None:
            _theta = theta + dangle
        else:
            _theta = theta

        for t in range(sz_tof - 1):
            tof1 = tof[t]
            tof2 = tof[t+1]
            tofm = (tof1+tof2)/2.
            _Q = _const * math.sin(_theta) / (tofm*1e-6)
#            _Q = _const * math.sin(_theta) / (tof1 * 1e-6)
            q_array[y, t] = _Q * 1e-10
    
    return q_array

def getQHisto(source_to_detector, theta, tof_array):
    _const = float(4) * math.pi * m * source_to_detector / h
    sz_tof = len(tof_array)
    q_array = zeros(sz_tof)
    for t in range(sz_tof):
        _Q = _const * math.sin(theta) / (tof_array[t] * 1e-6)
        q_array[t] = _Q * 1e-10
    
    return q_array

def ref_beamdiv_correct(cpix, mt, det_secondary,
                        pixel_index,
                        pixel_width=0.0007):
    """
    This function calculates the acceptance diagram, determines pixel overlap
    and computes the offset to the scattering angle.
    """
    
    # This is currently set to the same number for both REF_L and REF_M
    epsilon = 0.5 * 1.3 * 1.0e-3
    
    # Set the center pixel
    if cpix is None:
        cpix = 133.5
        
#    first_slit_size = getS1h(mt)
    first_slit_size = getSheight(mt, '1')
#    last_slit_size = getS2h(mt)
    last_slit_size = getSheight(mt,'2')
    
    last_slit_dist = 0.654 #m
    slit_dist = 0.885000050068 #m
    
    first_slit_size = float(first_slit_size) * 0.001
    last_slit_size = float(last_slit_size) * 0.001
    
    _y = 0.5 * (first_slit_size + last_slit_size)
    _x = slit_dist
    gamma_plus = math.atan2(_y, _x)
    
    _y = 0.5 * (first_slit_size - last_slit_size)
    _x = slit_dist 
    gamma_minus = math.atan2(_y, _x)
    
    half_last_aperture = 0.5 * last_slit_size
    neg_half_last_aperture = -1.0 * half_last_aperture
    
    last_slit_to_det = last_slit_dist + det_secondary
    dist_last_aper_det_sin_gamma_plus = last_slit_to_det * math.sin(gamma_plus)
    dist_last_aper_det_sin_gamma_minus = last_slit_to_det * math.sin(gamma_minus)
    
    #set the delta theta coordinates of the acceptance polygon
    accept_poly_x = []
    accept_poly_x.append(-1.0 * gamma_minus)
    accept_poly_x.append(gamma_plus)
    accept_poly_x.append(gamma_plus)
    accept_poly_x.append(gamma_minus)
    accept_poly_x.append(-1.0 * gamma_plus)
    accept_poly_x.append(-1.0 * gamma_plus)
    accept_poly_x.append(accept_poly_x[0])
    
    #set the z coordinates of the acceptance polygon
    accept_poly_y = []
    accept_poly_y.append(half_last_aperture - dist_last_aper_det_sin_gamma_minus + epsilon)
    accept_poly_y.append(half_last_aperture + dist_last_aper_det_sin_gamma_plus + epsilon)
    accept_poly_y.append(half_last_aperture + dist_last_aper_det_sin_gamma_plus - epsilon)
    accept_poly_y.append(neg_half_last_aperture + dist_last_aper_det_sin_gamma_minus - epsilon)
    accept_poly_y.append(neg_half_last_aperture - dist_last_aper_det_sin_gamma_plus - epsilon)
    accept_poly_y.append(neg_half_last_aperture - dist_last_aper_det_sin_gamma_plus + epsilon)
    accept_poly_y.append(accept_poly_y[0])
    
    cur_index = pixel_index
    
    #set the z band for the pixel
    xMinus = (cur_index - cpix - 0.5) * pixel_width
    xPlus = (cur_index - cpix + 0.5) * pixel_width
    
    #calculate the intersection
    yLeftCross = -1
    yRightCross = -1
    
    xI = accept_poly_x[0]
    yI = accept_poly_y[0]
    
    int_poly_x = []
    int_poly_y = []
    
    for i in range(len(accept_poly_x)):
        
        xF = accept_poly_y[i]
        yF = accept_poly_x[i]
        
        if xI < xF:
            
            if xI < xMinus and xF > xMinus:
                yLeftCross = yI + (yF - yI) * (xMinus - xI) / (xF - xI)
                int_poly_x.append(yLeftCross)
                int_poly_y.append(xMinus)
            
            if xI < xPlus and xF >= xPlus:
                yRightCross = yI + (yF - yI) * (xPlus - xI) / (xF - xI)
                int_poly_x.append(yRightCross)
                int_poly_y.append(xPlus)
    
        else:
            
            if xF < xPlus and xI >= xPlus:
                yRightCross = yI + (yF - yI) * (xPlus - xI) / (xF - xI)
                int_poly_x.append(yRightCross)
                int_poly_y.append(xPlus)
                
            if xF < xMinus and xI >= xMinus:
                yLeftCross = yI + (yF - yI) * (xMinus - xI) / (xF - xI)
                int_poly_x.append(yLeftCross)
                int_poly_y.append(xMinus)
                
        #This catches points on the polygon inside the range of interest
        if xF >= xMinus and xF < xPlus:
            int_poly_x.append(yF)
            int_poly_y.append(xF)
            
        xI = xF
        yI = yF
        
    if (len(int_poly_x) > 2):
        int_poly_x.append(int_poly_x[0])
        int_poly_y.append(int_poly_y[0])
        int_poly_x.append(int_poly_x[1])
        int_poly_y.append(int_poly_y[1])
    else:
        #Intersection polygon is null, point or line, so has no area
        #therefore there is no angle corrction
        return None

    #Calculate intersection polygon area
    area = calc_area_2D_polygon(int_poly_x,
                                int_poly_y,
                                len(int_poly_x) - 2)

    center_of_mass = calc_center_of_mass(int_poly_x,
                                         int_poly_y,
                                         area)
        
    return center_of_mass

def calc_area_2D_polygon(x_coord, y_coord, size_poly):
    """
    Calculation of the area defined by the 2D polygon
    """
    _range = arange(size_poly) + 1
    area = 0
    for i in _range:
        area += (x_coord[i] * (y_coord[i + 1] - y_coord[i - 1]))
    return area / 2.
    
def calc_center_of_mass(arr_x, arr_y, A):
    """
    Function that calculates the center-of-mass for the given polygon
    
    @param arr_x: The array of polygon x coordinates
    @param arr_y: The array of polygon y coordinates
    @param A: The signed area of the polygon
    
    @return: The polygon center-of-mass
    """
    
    center_of_mass = 0.0
    SIXTH = 1. / 6.
    for j in arange(len(arr_x) - 2):
        center_of_mass += (arr_x[j] + arr_x[j + 1]) \
                * ((arr_x[j] * arr_y[j + 1]) - \
                   (arr_x[j + 1] * arr_y[j]))
        
    if A != 0.0:
        return (SIXTH * center_of_mass) / A
    else:
        return 0.0

def getFieldValue(table, row, column):
    _tag_value = table[row][column]
    _tag_value_split = _tag_value.split('=')
    return _tag_value_split[1]

def isWithinPrecisionRange(value_file, value_run, precision):
    diff = abs(float(value_file)) - abs(float(value_run))
    if abs(diff) <= precision:
        return True
    else:
        return False

def applySF(InputWorkspace,
            incidentMedium,
            sfFile,
            valuePrecision,
            slitsWidthFlag):
    """
    Function that apply scaling factor to data using sfCalculator.txt
    file created by the sfCalculator procedure
    """
    
    #check if config file is there
    if (os.path.isfile(sfFile)):
    
        #parse file and put info into array
        f = open(sfFile, 'r')
        sfFactorTable = []
        for line in f.read().split('\n'):
            if (len(line) > 0 and line[0] != '#'):
                sfFactorTable.append(line.split(' '))
        f.close()
    
        sz_table = shape(sfFactorTable)
        nbr_row = sz_table[0]
        
        _incidentMedium = incidentMedium.strip()

        _lr = getLambdaValue(mtd[InputWorkspace])
        _lr_value = _lr[0]
        _lr_value = float("{0:.2f}".format(_lr_value))

        #retrieve s1h and s2h values
        s1h = getS1h(mtd[InputWorkspace])
        s2h = getS2h(mtd[InputWorkspace])
        
        s1h_value = abs(s1h)
        s2h_value = abs(s2h)
     
        #retrieve s1w and s2w values
        s1w = getS1w(mtd[InputWorkspace])
        s2w = getS2w(mtd[InputWorkspace])
        
        s1w_value = abs(s1w)
        s2w_value = abs(s2w)
        
#        print sfFactorTable

        print '--> Data Lambda Requested: {0:2f}'.format(_lr_value)
        print '--> Data S1H: {0:2f}'.format(s1h_value)
        print '--> Data S2H: {0:2f}'.format(s2h_value)
        print '--> Data S1W: {0:2f}'.format(s1w_value)
        print '--> Data S2W: {0:2f}'.format(s2w_value)

        for i in range(nbr_row):
            
            _file_incidentMedium = getFieldValue(sfFactorTable,i,0)
            if (_file_incidentMedium.strip() == _incidentMedium.strip()):
                _file_lambdaRequested = getFieldValue(sfFactorTable,i,1)
                if (isWithinPrecisionRange(_file_lambdaRequested, 
                                           _lr_value, 
                                           valuePrecision)):
                    _file_s1h = getFieldValue(sfFactorTable,i,2)
                    if(isWithinPrecisionRange(_file_s1h,
                                              s1h_value,
                                              valuePrecision)):
                        _file_s2h = getFieldValue(sfFactorTable,i,3)
                        if(isWithinPrecisionRange(_file_s2h,
                                                  s2h_value,
                                                  valuePrecision)):
                            if (slitsWidthFlag):
                                _file_s1w = getFieldValue(sfFactorTable,i,4)
                                if(isWithinPrecisionRange(_file_s1w,
                                                          s1w_value,
                                                          valuePrecision)):
                                    _file_s2w = getFieldValue(sfFactorTable,i,5)
                                    if(isWithinPrecisionRange(_file_s2w,
                                                              s2w_value,
                                                              valuePrecision)):
                            
                                        print '--> Found a perfect match'
                                        a = float(getFieldValue(sfFactorTable,i,6))
                                        b = float(getFieldValue(sfFactorTable,i,7))  
                                        a_error = float(getFieldValue(sfFactorTable,i,8))
                                        b_error = float(getFieldValue(sfFactorTable,i,9))
                
                                        OutputWorkspace = _applySFtoArray(InputWorkspace,
                                                                          a, b, a_error, b_error)

                                        return OutputWorkspace

                            else:
                                    
                                print '--> Found a perfect match'
                                a = float(getFieldValue(sfFactorTable,i,6))
                                b = float(getFieldValue(sfFactorTable,i,7))  
                                a_error = float(getFieldValue(sfFactorTable,i,8))
                                b_error = float(getFieldValue(sfFactorTable,i,9))
                
                                OutputWorkspace = _applySFtoArray(InputWorkspace,
                                                                  a, b, a_error, b_error)

                                return OutputWorkspace

    else:
        
        print '-> scaling factor file for requested lambda NOT FOUND!'

    return InputWorkspace

def _applySFtoArray(workspace, a, b, a_error, b_error):
    """
    This function will create for each x-axis value the corresponding
    scaling factor using the formula y=a+bx and 
    """
    
    mt = mtd[workspace]
    x_axis = mt.readX(0)[:]    
    sz = len(x_axis)
    x_axis_factors = zeros(sz)
    x_axis_factors_error = zeros(sz)
    for i in range(sz):
        _x_value = float(x_axis[i])
        _factor = _x_value * b + a
        x_axis_factors[i] = _factor
        _factor_error = _x_value * b_error + a_error
        x_axis_factors_error[i] = _factor_error 

    #create workspace
    CreateWorkspace(OutputWorkspace='sfWorkspace',
                    DataX=x_axis,
                    DataY=x_axis_factors,
                    DataE=x_axis_factors_error,
                    Nspec=1,
                    UnitX="TOF")
    
    mt_before = mtd[workspace]

    Divide(workspace, 'sfWorkspace', workspace)

    mt_after = mtd[workspace]

    return workspace
        
