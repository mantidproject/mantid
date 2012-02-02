from numpy import zeros, arctan2, arange, shape
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
    _tof_axis = mt1.readX(0)[:]
    
    if geo_correction:
        
        yrange = arange(toYpixel-fromYpixel+1) + fromYpixel
        _q_axis = convertToRvsQWithCorrection(mt1, 
                                              dMD=source_to_detector,
                                              theta=theta,
                                              tof=_tof_axis, 
                                              yrange=yrange, 
                                              cpix=cpix)

        #replace the _q_axis of the yrange of interest by the new 
        #individual _q_axis
        
        y_size = toYpixel - fromYpixel + 1
        y_range = arange(y_size) + fromYpixel

        _y_axis = zeros((maxY, len(_tof_axis) - 1))
        _y_error_axis = zeros((maxY, len(_tof_axis) - 1))
    
        x_size = toXpixel - fromXpixel + 1 
        x_range = arange(x_size) + fromXpixel
        
        for x in x_range:
            for y in y_range:
                _index = int((maxY) * x + y)
                _y_axis[y, :] += mt1.readY(_index)[:]
                _y_error_axis[y, :] += ((mt1.readE(_index)[:]) * (mt1.readE(_index)[:]))

        for _q_index in range(y_size):
            
            _tmp_q_axis = _q_axis[_q_index]
            q_axis = _tmp_q_axis[::-1]

            _outputWorkspace = 'tmpOWks_' + str(_q_index)

            _y_axis_tmp = _y_axis[yrange[_q_index],:]
            _y_axis_tmp = _y_axis_tmp.flatten()

            _y_error_axis_tmp = _y_error_axis[yrange[_q_index],:]
            _y_error_axis_tmp = numpy.sqrt(_y_error_axis_tmp)
            _y_error_axis_tmp = _y_error_axis_tmp.flatten()

            _y_axis_tmp = _y_axis_tmp[::-1]
            _y_error_axis_tmp = _y_error_axis_tmp[::-1]
            
            CreateWorkspace(OutputWorkspace=_outputWorkspace,
                            DataX=q_axis,
                            DataY=_y_axis_tmp,
                            DataE=_y_error_axis_tmp,
                            Nspec=1,
                            UnitX="MomentumTransfer")

            if _q_index == 0:
                mt_tmp = mtd[_outputWorkspace]

            _outputWorkspace_rebin = 'tmpOWks_' + str(_q_index)
            
            Rebin(InputWorkspace=_outputWorkspace,
                  OutputWorkspace=_outputWorkspace_rebin,
                  Params=q_binning)

            if _q_index == 0:
                mt_tmp = mtd[_outputWorkspace_rebin]


        _mt = mtd['tmpOWks_0']
        _x_array = _mt.readX(0)[:]

        #create big y_array of the all the pixel of interest (yrange)
        big_y_array = zeros((maxY, len(_x_array)))
        big_y_error_array = zeros((maxY, len(_x_array)))
        
        for _q_index in range(y_size):
            
            _wks = 'tmpOWks_' + str(_q_index)
            _mt = mtd[_wks]
            _tmp_y = _mt.readY(0)[:]
            _tmp_y_error = _mt.readE(0)[:]

            big_y_array[yrange[_q_index],:] = _tmp_y
            big_y_error_array[yrange[_q_index],:] = _tmp_y_error

        _x_axis = _x_array.flatten()        
        _y_axis = big_y_array.flatten()
        _y_error_axis = big_y_error_array.flatten()
        
        CreateWorkspace(OutputWorkspace=outputWorkspace,
                        DataX=_x_axis,
                        DataY=_y_axis,
                        DataE=_y_error_axis,
                        Nspec=maxY,
                        UnitX="MomentumTransfer",
                        ParentWorkspace=mt1)
    
    else:
        
        if source_to_detector is not None and theta is not None:
            _const = float(4) * math.pi * m * source_to_detector / h
            _q_axis = 1e-10 * _const * math.sin(theta) / (_tof_axis*1e-6)
        else:
            _q_axis = _tof_axis

        _y_axis = zeros((maxY, len(_q_axis) - 1))
        _y_error_axis = zeros((maxY, len(_q_axis) - 1))
    
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

def create_grouping(workspace=None, xmin=0, xmax=None, filename=".refl_grouping.xml"):
    # This should be read from the 
    npix_x = 304
    npix_y = 256
    if workspace is not None:
        if mtd[workspace].getInstrument().hasParameter("number-of-x-pixels"):
            npix_x = int(mtd[workspace].getInstrument().getNumberParameter("number-of-x-pixels")[0])
        if mtd[workspace].getInstrument().hasParameter("number-of-y-pixels"):
            npix_y = int(mtd[workspace].getInstrument().getNumberParameter("number-of-y-pixels")[0])
    
    f = open(filename,'w')
    f.write("<detector-grouping description=\"Integrated over X\">\n")
    
    if xmax is None:
        xmax = npix_x
        
    for y in range(npix_y):
        # index = max_y * x + y
        indices = []
        for x in range(xmin, xmax+1):
            indices.append(str(npix_y*x + y))
        
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
    print 'dMD: '
    print dMD
    print
    
    _const = float(4) * math.pi * m * dMD / h
    sz_tof = numpy.shape(tof)[0]
    q_array = zeros(sz_tof-1)
    for t in range(sz_tof-1):
        tof1 = tof[t]
        tof2 = tof[t+1]
        tofm = (tof1+tof2)/2.
        _Q = _const * math.sin(theta) / (tofm*1e-6)
        q_array[t] = _Q*1e-10

    print 'q_array'
    print q_array
    return q_array
        
def convertToRvsQWithCorrection(mt, dMD=-1, theta=-1,tof=None, yrange=None, cpix=None):
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
    
    dPS_array = zeros((maxY, maxX))
    for x in range(maxX):
        for y in range(maxY):
            _index = maxY * x + y
            detector = mt.getDetector(_index)
            dPS_array[y, x] = sample.getDistance(detector)
    #array of distances pixel->source
    dMP_array = dPS_array + dSM
    #distance sample->center of detector
    dSD = dPS_array[maxY / 2, maxX / 2]

    _const = float(4) * math.pi * m * dMD / h
    sz_tof = len(tof)
    q_array = zeros((len(yrange), sz_tof-1))

    xrange = range(len(yrange))
    for x in xrange:
        _px = yrange[x]
        dangle = ref_beamdiv_correct(cpix, mt, dSD, _px)
        
        print 'dangle:'
        print dangle
        
        if dangle is not None:
            _theta = theta + dangle
        else:
            _theta = theta
        
        for t in range(sz_tof-1):
            tof1 = tof[t]
            tof2 = tof[t+1]
            tofm = (tof1+tof2)/2.
            _Q = _const * math.sin(_theta) / (tofm*1e-6)
            q_array[x,t] = _Q*1e-10

        print 'q_array'
        print q_array
        
    print

    return q_array

def getQHisto(source_to_detector, theta, tof_array):
    _const = float(4) * math.pi * m * source_to_detector / h
    sz_tof = len(tof_array)
    q_array = zeros(sz_tof)
    for t in range(sz_tof):
        _Q = _const * math.sin(theta) / (tof_array[t]*1e-6)
        q_array[t] = _Q*1e-10
    
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
        
    first_slit_size = getS1h(mt)
    last_slit_size = getS2h(mt)
    
    last_slit_dist = 0.654 #m
    slit_dist = 0.885000050068 #m
    
    first_slit_size = float(first_slit_size[0]) * 0.001
    last_slit_size = float(last_slit_size[0]) * 0.001
    
    _y = 0.5 * (first_slit_size + last_slit_size)
    _x = slit_dist
    gamma_plus = math.atan2( _y, _x)
    
    _y = 0.5 * (first_slit_size - last_slit_size)
    _x = slit_dist 
    gamma_minus = math.atan2( _y, _x)
    
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
        area += (x_coord[i] * (y_coord[i+1] - y_coord[i-1]))
    return area/2.
    
def calc_center_of_mass(arr_x, arr_y, A):
    """
    Function that calculates the center-of-mass for the given polygon
    
    @param arr_x: The array of polygon x coordinates
    @param arr_y: The array of polygon y coordinates
    @param A: The signed area of the polygon
    
    @return: The polygon center-of-mass
    """
    
    center_of_mass = 0.0
    SIXTH = 1./6.
    for j in arange(len(arr_x)-2):
        center_of_mass += (arr_x[j] + arr_x[j+1]) \
                * ((arr_x[j] * arr_y[j+1]) - \
                   (arr_x[j+1] * arr_y[j]))
        
    if A != 0.0:
        return (SIXTH * center_of_mass) / A
    else:
        return 0.0
    
            