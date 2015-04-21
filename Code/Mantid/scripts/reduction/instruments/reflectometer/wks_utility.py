#pylint: disable=invalid-name
from numpy import zeros, arctan2, arange, shape, sqrt, fliplr, asfarray, mean, sum, NAN
from mantid.simpleapi import *
# from MantidFramework import *
import math
import os.path

h = 6.626e-34 #m^2 kg s^-1
m = 1.675e-27 #kg
ref_date = '2014-10-01' #when the detector has been rotated

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
#        proton_charge = proton_charge_mtd_unit / 2.77777778e-10
        return proton_charge_mtd_unit
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
    if index == 2:
        isSi = False
        try:
            tag = 'SiVHeight'
            value = mt_run.getProperty(tag).value
            isSi = True
        except:
            tag = 'S2VHeight'
            value = mt_run.getProperty(tag).value
        return [isSi, value[0]]
    else:
        tag = 'S1VHeight'
        value = mt_run.getProperty(tag).value

    return value[0]

def getS1h(mt=None):
    """
        returns the height and units of the slit #1
    """
    if mt != None:
#        _h, units = getSh(mt, 's1t', 's1b')
        _h = getSheight(mt, 1)
        return _h
    return None

def getS2h(mt=None):
    """
        returns the height and units of the slit #2
    """
    if mt != None:
        [isSi, _h] = getSheight(mt, 2)
        return [isSi,_h]
    return [False, None]

def getSwidth(mt, index):
    """
        returns the width and units of the given index slits
        defined by the DAS hardware
    """
    mt_run = mt.getRun()
    if index==2:
        isSi = False
        try:
            tag = 'SiHWidth'
            value = mt_run.getProperty(tag).value
            isSi = True
        except:
            tag = 'S2HWidth'
            value = mt_run.getProperty(tag).value
        return [isSi, value[0]]
    else:
        tag = 'S1HWidth'
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
        _w = getSwidth(mt, 1)
        return _w
    return None

def getS2w(mt=None):
    """
        returns the width and units of the slit #2
    """
    if mt != None:
        [isSi, _w] = getSwidth(mt, 2)
        return [isSi,_w]
    return [False,None]


def getLambdaValue(mt_name):
    """
    return the lambdaRequest value
    """
    mt_run = mtd[mt_name].getRun()
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
        if _q_min > q_min:
            q_min = _q_min
        if _q_max < q_max:
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

def createIntegratedWorkspace(mt1,
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
    _y_error_axis = sqrt(_y_error_axis)
    _y_error_axis = _y_error_axis.flatten()

    outputWorkspace = CreateWorkspace(DataX=_tof_axis,
                                      DataY=_y_axis,
                                      DataE=_y_error_axis,
                                      Nspec=maxY,
                                      UnitX="TOF",
                                      ParentWorkspace=mt1.name())

    return outputWorkspace






def convertWorkspaceToQ(ws_data,
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

    mt1 = ws_data
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
            if _q_min != 0:
                _y_axis_tmp[0:_q_min] = 0
                _y_error_axis_tmp[0:_q_min] = 0

            _q_max = int(_q_axis_min_max_index[_q_index, 1])
            sz = shape(_y_axis_tmp)[0]
            if _q_max != sz:
                _index_q_max_range = arange(sz - _q_max) + _q_max
                for i in _index_q_max_range:
                    _y_axis_tmp[i] = 0
                    _y_error_axis_tmp[i] = 0

            _y_axis[_q_index, :] = _y_axis_tmp[::-1]
            _y_error_axis[_q_index, :] = _y_error_axis_tmp[::-1]

        x_axis = q_axis.flatten()
        y_axis = _y_axis.flatten()
        y_error_axis = _y_error_axis.flatten()

        outputWorkspace = CreateWorkspace(DataX=x_axis,
                                          DataY=y_axis,
                                          DataE=y_error_axis,
                                          Nspec=int(y_size),
                                          UnitX="MomentumTransfer",
                                          ParentWorkspace=mt1.name())

        outputWorkspace.setDistribution(True)

        outputWorkspace = Rebin(InputWorkspace=outputWorkspace,\
              Params=q_binning)

    else:

        if source_to_detector is not None and theta is not None:
            _const = float(4) * math.pi * m * source_to_detector / h
            _q_axis = 1e-10 * _const * math.sin(theta) / (_tof_axis * 1e-6)
        else:
            _q_axis = _tof_axis
            print 'should not reach this condition !'

        y_size = toYpixel - fromYpixel + 1
        y_range = arange(y_size) + fromYpixel

        _y_axis = zeros((y_size, len(_q_axis) -1 ))
        _y_error_axis = zeros((y_size, len(_q_axis) - 1))

        for y in range(y_size):

            a = y_range[y]

            _tmp_y_axis = mt1.readY(int(a))[:]
            _y_axis[int(y), :] = _tmp_y_axis
            _tmp_y_error_axis = mt1.readE(int(a))[:]
            _y_error_axis[int(y),:] = _tmp_y_error_axis

        _x_axis = _q_axis.flatten()
        _y_axis = _y_axis.flatten()
        _y_error_axis = _y_error_axis.flatten()

        # reverse order
        _x_axis = _x_axis[::-1]
        _y_axis = _y_axis[::-1]
        _y_error_axis = _y_error_axis[::-1]

        outputWorkspace = CreateWorkspace(DataX=_x_axis,
                                          DataY=_y_axis,
                                          DataE=_y_error_axis,
                                          Nspec=int(y_size),
                                          UnitX="MomentumTransfer",
                                          ParentWorkspace=mt1.name())

        outputWorkspace.setDistribution(True)

        outputWorkspace = Rebin(InputWorkspace=outputWorkspace,\
              Params=q_binning)

    return outputWorkspace

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

    if from_units == to_units:
        return value

    from_factor = 1
    #convert everything into rad
    if from_units == 'degree':
        from_factor = 1.745329252e-2
    value_rad = from_factor * value

    if to_units == 'rad':
        return value_rad
    else:
        to_factor = 57.2957795
        return to_factor * value_rad

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

def ref_beamdiv_correct(cpix, det_secondary,
                        pixel_index,
                        pixel_width = 0.0007,
                        first_slit_size = None,
                        last_slit_size = None):
    """
    This function calculates the acceptance diagram, determines pixel overlap
    and computes the offset to the scattering angle.
    """

    # This is currently set to the same number for both REF_L and REF_M
    epsilon = 0.5 * 1.3 * 1.0e-3

    # Set the center pixel
    if cpix is None:
        cpix = 133.5

#     first_slit_size = getSheight(mt, '1')
#     last_slit_size = getSheight(mt,'2')

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

    if len(int_poly_x) > 2:
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

#def applySF(InputWorkspace,
            #incidentMedium,
            #sfFile,
            #valuePrecision,
            #slitsWidthFlag):
    #"""
    #Function that apply scaling factor to data using sfCalculator.txt
    #file created by the sfCalculator procedure
    #"""

    ##check if config file is there
    #if os.path.isfile(sfFile):

        ##parse file and put info into array
        #f = open(sfFile, 'r')
        #sfFactorTable = []
        #for line in f.read().split('\n'):
            #if len(line) > 0 and line[0] != '#':
                #sfFactorTable.append(line.split(' '))
        #f.close()

        #sz_table = shape(sfFactorTable)
        #nbr_row = sz_table[0]

        #_incidentMedium = incidentMedium.strip()

        #_lr = getLambdaValue(mtd[InputWorkspace])
        #_lr_value = _lr[0]
        #_lr_value = float("{0:.2f}".format(_lr_value))

        ##retrieve s1h and s2h values
        #s1h = getS1h(mtd[InputWorkspace])
        #s2h = getS2h(mtd[InputWorkspace])

        #s1h_value = abs(s1h)
        #s2h_value = abs(s2h)

        ##retrieve s1w and s2w values
        #s1w = getS1w(mtd[InputWorkspace])
        #s2w = getS2w(mtd[InputWorkspace])

        #s1w_value = abs(s1w)
        #s2w_value = abs(s2w)

##        print sfFactorTable

        #print '--> Data Lambda Requested: {0:2f}'.format(_lr_value)
        #print '--> Data S1H: {0:2f}'.format(s1h_value)
        #print '--> Data S2H: {0:2f}'.format(s2h_value)
        #print '--> Data S1W: {0:2f}'.format(s1w_value)
        #print '--> Data S2W: {0:2f}'.format(s2w_value)

        #print 'mERDDEEEEDEDEED'
        #for i in range(nbr_row):

            #_file_incidentMedium = getFieldValue(sfFactorTable,i,0)
            #if _file_incidentMedium.strip() == _incidentMedium.strip():
                #print '--- incident medium match ---'
                #_file_lambdaRequested = getFieldValue(sfFactorTable,i,1)
                #if (isWithinPrecisionRange(_file_lambdaRequested,
                                           #_lr_value,
                                           #valuePrecision)):
                    #print '--- lambda requested match ---'
                    #_file_s1h = getFieldValue(sfFactorTable,i,2)
                    #if(isWithinPrecisionRange(_file_s1h,
                                              #s1h_value,
                                              #valuePrecision)):
                        #print '--- S1H match ---'
                        #_file_s2h = getFieldValue(sfFactorTable,i,3)
                        #if(isWithinPrecisionRange(_file_s2h,
                                                  #s2h_value,
                                                  #valuePrecision)):
                            #print '--- S2H match ---'
                            #if slitsWidthFlag:
                                #print '--- (with Width flag) ----'
                                #_file_s1w = getFieldValue(sfFactorTable,i,4)
                                #if(isWithinPrecisionRange(_file_s1w,
                                                          #s1w_value,
                                                          #valuePrecision)):
                                    #print '--- S1W match ---'
                                    #_file_s2w = getFieldValue(sfFactorTable,i,5)
                                    #if(isWithinPrecisionRange(_file_s2w,
                                                              #s2w_value,
                                                              #valuePrecision)):
                                        #print '--- S2W match ---'

                                        #print '--> Found a perfect match'
                                        #a = float(getFieldValue(sfFactorTable,i,6))
                                        #b = float(getFieldValue(sfFactorTable,i,7))
                                        #a_error = float(getFieldValue(sfFactorTable,i,8))
                                        #b_error = float(getFieldValue(sfFactorTable,i,9))

                                        #OutputWorkspace = _applySFtoArray(InputWorkspace,
                                                                          #a, b, a_error, b_error)

                                        #return OutputWorkspace

                            #else:

                                #print '--> Found a perfect match'
                                #a = float(getFieldValue(sfFactorTable,i,6))
                                #b = float(getFieldValue(sfFactorTable,i,7))
                                #a_error = float(getFieldValue(sfFactorTable,i,8))
                                #b_error = float(getFieldValue(sfFactorTable,i,9))

                                #OutputWorkspace = _applySFtoArray(InputWorkspace,
                                                                  #a, b, a_error, b_error)

                                #return OutputWorkspace

    #else:

        #print '-> scaling factor file for requested lambda NOT FOUND!'

    #return InputWorkspace

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

def loadNeXus(runNumbers, type):
    """
    will retrieve the data from the runNumbers specify and will
    add them or just return the workspace created
    """

    wks_name = ''
    if type == 'data':
        wks_name = 'ws_event_data'
    else:
        wks_name = 'ws_event_norm'

    print '-> loading ', type
    if (type == 'data') and len(runNumbers) > 1:

        _list = []
        for _run in runNumbers:
            _list.append(str(_run))
        list_run = ','.join(_list)
        print '--> working with runs: ' + str(list_run)

        _index = 0
        for _run in runNumbers:

        # Find full path to event NeXus data file
            try:
                data_file = FileFinder.findRuns("REF_L%d" %_run)[0]
            except RuntimeError:
                msg = "RefLReduction: could not find run %d\n" % _run
                msg += "Add your data folder to your User Data Directories in the File menu"
                raise RuntimeError(msg)

            if _index == 0:
                ws_event_data = LoadEventNexus(Filename=data_file,OutputWorskpace=wks_name)
                _index += 1
            else:
                tmp = LoadEventNexus(Filename=data_file)
                Plus(LHSWorkspace=ws_event_data,
                     RHSWorkspace=tmp,
                     OutputWorkspace=wks_name)
                DeleteWorkspace(tmp)
    else:

        print '--> Working with run: ' + str(runNumbers)

        try:
            data_file = FileFinder.findRuns("REF_L%d" %runNumbers)[0]
        except RuntimeError:
            msg = "RefLReduction: could not find run %d\n" %runNumbers[0]
            msg += "Add your data folder to your User Data Directories in the File menu"
            raise RuntimeError(msg)

        ws_event_data = LoadEventNexus(Filename=data_file, OutputWorkspace=wks_name)

    return ws_event_data

def rebinNeXus(inputWorkspace, params, type):
    """
    will rebin the event workspace according to the params
    params[0]: min value
    params[1]: bin size
    params[2]: max value
    """
    print '--> rebin ', type
    ws_histo_data = Rebin(InputWorkspace=inputWorkspace,
                          Params=params,
                          PreserveEvents=True)
    return ws_histo_data

def cropTOF(inputWorkspace, min, max, type):
    """
    will crop the nexus (workspace) using min and max value
    used here to crop the TOF range
    """
    print '--> crop ' , type , ' workspace in TOF'
    ws_histo_data = CropWorkspace(InputWorkspace = inputWorkspace,\
                                      XMin = min,\
                                      XMax = max)
    return ws_histo_data

def normalizeNeXus(inputWorkspace, type):
    """
    normalize nexus by proton charge
    """
    print '--> normalize ', type
    ws_histo_data = NormaliseByCurrent(InputWorkspace=inputWorkspace)
    return ws_histo_data

def integrateOverLowResRange(mt1,\
                            dataLowResRange,\
                            type,\
                            is_nexus_detector_rotated_flag):
    """
        This creates the integrated workspace over the low resolution range leaving
        us with a [256,nbr TOF] workspace
        returns the new workspace handle
        BUT this algorithm also makes sure that the error value is 1 when counts
        is 0 !
    """

    print '--> integrated over low res range of ', type
    _tof_axis = mt1.readX(0)[:].copy()
    nbr_tof = len(_tof_axis)
#     t_range = arange(nbr_tof-1)

    # -1 to work with index directly
    fromXpixel = min(dataLowResRange) - 1
    toXpixel = max(dataLowResRange) - 1

    if is_nexus_detector_rotated_flag:
        sz_y_axis = 304
        sz_x_axis = 256
    else:
        sz_y_axis = 256
        sz_x_axis = 304

    _y_axis = zeros((sz_y_axis, len(_tof_axis) - 1))
    _y_error_axis = zeros((sz_y_axis, len(_tof_axis) - 1))

    x_size = toXpixel - fromXpixel + 1
    x_range = arange(x_size) + fromXpixel

    y_range = range(sz_y_axis)

    for x in x_range:
        for y in y_range:
            _index = int((sz_y_axis) * x + y)
            _y_axis[y, :] += mt1.readY(_index)[:].copy()
            _tmp_error_axis = mt1.readE(_index)[:].copy()
            # 0 -> 1
#             index_where_0 = where(_tmp_error_axis == 0)
#             _tmp_error_axis[index_where_0] = 1

            _y_error_axis[y, :] += _tmp_error_axis * _tmp_error_axis
#             _y_error_axis[y, :] += ((mt1.readE(_index)[:]) * (mt1.readE(_index)[:]))

    _y_error_axis = sqrt(_y_error_axis)

    return [_tof_axis, _y_axis, _y_error_axis]

def substractBackground(tof_axis, y_axis, y_error_axis,
                        peakRange, backFlag, backRange,
                        error_0, type):
    """
    shape of y_axis : [sz_y_axis, nbr_tof]
    This routine will calculate the background, remove it from the peak
    and will return only the range of peak  -> [peak_size, nbr_tof]

    """

    # give a friendly name to peak and back ranges
    # -1 because we are working with 0 index arrays
    peakMin = peakRange[0]-1
    peakMax = peakRange[1]-1
    backMin = backRange[0]-1
    backMax = backRange[1]-1

    if not backFlag:
        print '---> no ', type, ' background requested!'
        return [y_axis[peakMin:peakMax+1,:], y_error_axis[peakMin:peakMax+1,:]]

    print '--> background subtraction of ', type

    # retrieve data
    _tofAxis = tof_axis
    nbrTof = len(_tofAxis)

    # size peak
    szPeak = peakMax - peakMin + 1

    # init arrays
    minBack = []
    minBackError = []
    maxBack = []
    maxBackError = []

    final_y_axis = zeros((szPeak, nbrTof))
    final_y_error_axis = zeros((szPeak, nbrTof))

#    final_y_axis = empty((szPeak, nbrTof))
#    final_y_error_axis = empty((szPeak, nbrTof))
#    final_y_axis[:] = NAN
#    final_y_error_axis[:] = NAN

    for t in range(nbrTof):

        # by default, no space for background subtraction below and above peak
        bMinBack = False
        bMaxBack = False

        if backMin < (peakMin):
            bMinBack = True
            _backMinArray = y_axis[backMin:peakMin, t]
            _backMinErrorArray = y_error_axis[backMin:peakMin, t]
            [_backMin, _backMinError] = weightedMean(_backMinArray,\
                                                          _backMinErrorArray, error_0)

        if (peakMax) < backMax:
            bMaxBack = True
            _backMaxArray = y_axis[peakMax+1:backMax+1, t]
            _backMaxErrorArray = y_error_axis[peakMax+1:backMax+1, t]
            [_backMax, _backMaxError] = weightedMean(_backMaxArray, _backMaxErrorArray, error_0)

        # if no max background use min background
        if not bMaxBack:
            background = _backMin
            background_error = _backMinError

        # if no min background use max background
        if not bMinBack:
            background = _backMax
            background_error = _backMaxError

        if bMinBack and bMaxBack:
            [background, background_error] = weightedMean([_backMin, _backMax], [_backMinError, _backMaxError], error_0)

        # remove background for each pixel of the peak
        for x in range(szPeak):
            final_y_axis[x,t] = float(y_axis[peakMin + x,t]) - float(background)
            final_y_error_axis[x,t] = float(math.sqrt(pow(y_error_axis[peakMin+x,t],2) + pow(background_error,2)))

#         if t == nbrTof-2:
#             print float(y_axis[peakMin + x,t]) - float(background)

    return [final_y_axis, final_y_error_axis]

def weightedMean(data_array, error_array, error_0):

    sz = len(data_array)

    # calculate the numerator of mean
    dataNum = 0
    for i in range(sz):
        if error_array[i] == 0:
            error_array[i] = error_0

        tmpFactor = float(data_array[i]) / float((pow(error_array[i],2)))
        dataNum += tmpFactor

    # calculate denominator
    dataDen = 0
    for i in range(sz):
        if error_array[i] == 0:
            error_array[i] = error_0
        tmpFactor = 1./float((pow(error_array[i],2)))
        dataDen += tmpFactor

    if dataDen == 0:
        data_mean = NAN
        mean_error = NAN
    else:
        data_mean = float(dataNum) / float(dataDen)
        mean_error = math.sqrt(1/dataDen)

    return [data_mean, mean_error]

def weightedMeanOfRange(norm_y_axis, norm_y_error_axis):
    """
    will calculate the weighted Mean of the region given
    """

    # get nbr tof
    dim = norm_y_axis.shape
    nbr_tof = dim[1]

    final_array = zeros(nbr_tof)
    final_array_error = zeros(nbr_tof)

    for t in range(nbr_tof):
        _tmp_range = norm_y_axis[:, t]
        _tmp_range_error = norm_y_error_axis[:,t]
        [_mean,_mean_error] = weightedMean(_tmp_range, _tmp_range_error)
        final_array[t] = _mean
        final_array_error[t] = _mean_error

    return [final_array, final_array_error]

def meanOfRange(norm_y_axis, norm_y_error_axis):
    """
    will calculate the mean of range
    """

    # get nbr tof
    dim = norm_y_axis.shape
    nbr_tof = dim[1]

    final_array = zeros(nbr_tof)
    final_array_error = zeros(nbr_tof)

    for t in range(nbr_tof):
        _tmp_range = norm_y_axis[:,t]
        _tmp_range_error = norm_y_error_axis[:,t]
        [_mean,_mean_error] = myMean(_tmp_range, _tmp_range_error)
        final_array[t] = _mean
        final_array_error[t] = _mean_error

    return [final_array, final_array_error]

def myMean(data_array, error_array):

    sz=size(data_array)

    _mean = mean(data_array)
    _mean_error = sqrt(sum(_mean*_mean))/float(sz[0])

    return [_mean, _mean_error]

def divideDataByNormalization(data_y_axis,
                              data_y_error_axis,
                              av_norm,
                              av_norm_error):

    print '-> divide data by normalization'

    data_size = data_y_axis.shape
    nbr_pixel = data_size[0]
    nbr_tof = data_size[1]

    new_data_y_axis = zeros((nbr_pixel, nbr_tof))
    new_data_y_error_axis = zeros((nbr_pixel, nbr_tof))

    for t in range(nbr_tof):
        for x in range(nbr_pixel):

            if (not av_norm[t] == 0) and (not data_y_axis[x,t] == 0) :

                tmp_value = float(data_y_axis[x,t]) / float(av_norm[t])

                tmp_error_1 = pow(float(data_y_error_axis[x,t]) / float(data_y_axis[x,t]),2)
                tmp_error_2 = pow(float(av_norm_error[t]) / float(av_norm[t]),2)
                tmp_error = sqrt(tmp_error_1 + tmp_error_2) * abs(float(data_y_axis[x,t]) / float(av_norm[t]))

                new_data_y_axis[x,t] = tmp_value
                new_data_y_error_axis[x,t] = tmp_error

    return [new_data_y_axis, new_data_y_error_axis]

def sumWithError(value, error):
    """ will sume the array of values and will return the sum and the
    error that goes with it
    """

    sum_value = sum(value)

    tmp_sum_error = 0
    for i in range(len(value)):
        tmp_value = pow(error[i],2)
        tmp_sum_error += tmp_value

    sum_error = math.sqrt(tmp_sum_error)

    return [sum_value, sum_error]

def integratedOverPixelDim(data_y_axis, data_y_error_axis):

    size = data_y_axis.shape
    nbr_pixel = size[0]
    nbr_tof = size[1]

    final_data = zeros(nbr_tof)
    final_data_error = zeros(nbr_tof)
    for t in range(nbr_tof):
        [data, error] = sumWithError(data_y_axis[:,t], data_y_error_axis[:,t])
        final_data[t] = data
        final_data_error[t] = error

    return [final_data, final_data_error]

def fullSumWithError(data_y_axis, data_y_error_axis):
    size = data_y_axis.shape
    nbr_pixel = size[0]
    nbr_tof = size[1]

    final_data = zeros(nbr_tof)
    final_data_error = zeros(nbr_tof)
#    final_data = empty(nbr_tof)
#    final_data_error = empty(nbr_tof)
#    final_data[:] = NAN
#    final_data_error[:] = NAN
    for t in range(nbr_tof):
        [data, error] = sumWithError(data_y_axis[:,t], data_y_error_axis[:,t])
        final_data[t] = data
        final_data_error[t] = error

    return [final_data, final_data_error]

def ouput_ascii_file(file_name,
                     x_axis,
                     y_axis,
                     y_error_axis):

    f=open(file_name,'w')

    sz_x_axis = len(x_axis)
    for i in range(sz_x_axis-1):
        f.write(str(x_axis[i]) + "," + str(y_axis[i]) + "," + str(y_error_axis[i]) + "\n")

    f.close

def ouput_big_ascii_file(file_name,
                         x_axis,
                         y_axis,
                         y_error_axis):

    f=open(file_name,'w')

    sz = y_axis.shape # (nbr_pixel, nbr_tof)
    nbr_tof = sz[1]
    nbr_pixel = sz[0]

    for t in range(nbr_tof):
        _tmp_str = str(x_axis[t])
        for x in range(nbr_pixel):
            _tmp_str += ' ,' + str(y_axis[x,t]) + " ," + str(y_error_axis[x,t])

        _tmp_str += '\n'
        f.write(_tmp_str)

    f.close



def ouput_big_Q_ascii_file(file_name,\
                         x_axis,\
                         y_axis,\
                         y_error_axis):

    f=open(file_name,'w')

    sz = y_axis.shape # (nbr_pixel, nbr_tof)
    nbr_tof = sz[1]
    nbr_pixel = sz[0]

    for t in range(nbr_tof):
        _tmp_str = ''
        for x in range(nbr_pixel):
            _tmp_str += str(x_axis[x,t]) +  ',' + str(y_axis[x,t]) + " ," + str(y_error_axis[x,t]) + ',,'
        _tmp_str += '\n'
        f.write(_tmp_str)

    f.close


def divideData1DbyNormalization(inte_data_y_axis,
                                inte_data_y_error_axis,
                                av_norm,
                                av_norm_error):

    print '-> divide data by normalization'

    nbrPixel = inte_data_y_axis.shape

    final_data = zeros(nbrPixel)
    final_data_error = zeros(nbrPixel)

    for x in range(nbrPixel[0]):
        if not av_norm[x] == 0:

            final_data[x] = inte_data_y_axis[x] / av_norm[x]

            tmp1 = pow(float(inte_data_y_error_axis[x]) / float(inte_data_y_axis[x]),2)
            tmp2 = pow(float(av_norm_error[x]) / float(av_norm[x]),2)
            tmp_error = sqrt(tmp1 + tmp2) * (float(inte_data_y_axis[x] / av_norm[x]))

            final_data_error[x] = tmp_error

    return [final_data, final_data_error]

def applyScalingFactor(tof_axis,
                       y_data,
                       y_data_error,
                       incident_medium,
                       sf_file,
                       valuePrecision,
                       slitsWidthFlag):
    """"
    function that apply scaling factor to data using sfCalculator.txt
    file created by the sfCalculator procedure
    """
    isSFfound = False

    #sf_file = 'NaN'
    if os.path.isfile(sf_file):

        print '-> scaling factor file FOUND! (', sf_file, ')'

        #parse file and put info into array
        f = open(sf_file, 'r')
        sfFactorTable = []
        for line in f.read().split('\n'):
            if len(line) > 0 and line[0] != '#':
                sfFactorTable.append(line.split(' '))
        f.close()

        sz_table = shape(sfFactorTable)
        nbr_row = sz_table[0]

        _incidentMedium = incident_medium.strip()

        _lr = getLambdaValue('ws_event_data')
        _lr_value = _lr[0]
        _lr_value = float("{0:.2f}".format(_lr_value))

        #retrieve s1h and s2h or sih values
        s1h = getS1h(mtd['ws_event_data'])
        [isSih, s2h] = getS2h(mtd['ws_event_data'])

        s1h_value = abs(s1h)
        s2h_value = abs(s2h)

        #retrieve s1w and s2w values
        s1w = getS1w(mtd['ws_event_data'])
        [isSiw, s2w] = getS2w(mtd['ws_event_data'])

        s1w_value = abs(s1w)
        s2w_value = abs(s2w)

        print '--> Data Lambda Requested: {0:2f}'.format(_lr_value)
        print '--> Data S1H: {0:2f}'.format(s1h_value)
        if isSih:
            print '--> Data SiH: {0:2f}'.format(s2h_value)
        else:
            print '--> Data S2H: {0:2f}'.format(s2h_value)
        print '--> Data S1W: {0:2f}'.format(s1w_value)
        if isSiw:
            print '--> Data SiW: {0:2f}'.format(s2w_value)
        else:
            print '--> Data S2W: {0:2f}'.format(s2w_value)

        for i in range(nbr_row):

            _file_incidentMedium = getFieldValue(sfFactorTable,i,0)
            if _file_incidentMedium.strip() == _incidentMedium.strip():
                print '*** incident medium match ***'
                _file_lambdaRequested = getFieldValue(sfFactorTable,i,1)
                if (isWithinPrecisionRange(_file_lambdaRequested,
                                           _lr_value,
                                           valuePrecision)):
                    print '*** lambda requested match ***'
                    _file_s1h = getFieldValue(sfFactorTable,i,2)
                    if(isWithinPrecisionRange(_file_s1h,
                                              s1h_value,
                                              valuePrecision)):
                        print '*** s1h match ***'
                        _file_s2h = getFieldValue(sfFactorTable,i,3)
                        if(isWithinPrecisionRange(_file_s2h,
                                                  s2h_value,
                                                  valuePrecision)):
                            print '*** s2h match ***'
                            if slitsWidthFlag:
                                print '*** (with slits width flag) ***'
                                _file_s1w = getFieldValue(sfFactorTable,i,4)
                                if(isWithinPrecisionRange(_file_s1w,
                                                          s1w_value,
                                                          valuePrecision)):
                                    print '*** s1w match ***'
                                    _file_s2w = getFieldValue(sfFactorTable,i,5)
                                    if(isWithinPrecisionRange(_file_s2w,
                                                              s2w_value,
                                                              valuePrecision)):
                                        print '*** s2w match ***'

                                        print '--> Found a perfect match'
                                        a = float(getFieldValue(sfFactorTable,i,6))
                                        b = float(getFieldValue(sfFactorTable,i,7))
                                        a_error = float(getFieldValue(sfFactorTable,i,8))
                                        b_error = float(getFieldValue(sfFactorTable,i,9))

                                        [y_data, y_data_error] = applyScalingFactorToArray(tof_axis,
                                                                                           y_data,
                                                                                           y_data_error,
                                                                                           a, b,
                                                                                           a_error, b_error)

                                        return [tof_axis, y_data, y_data_error, True]

                            else:

                                print '--> Found a perfect match'
                                a = float(getFieldValue(sfFactorTable,i,6))
                                b = float(getFieldValue(sfFactorTable,i,7))
                                a_error = float(getFieldValue(sfFactorTable,i,8))
                                b_error = float(getFieldValue(sfFactorTable,i,9))

                                [y_data, y_data_error] = applyScalingFactorToArray(tof_axis,
                                                                                   y_data,
                                                                                   y_data_error,
                                                                                   a, b,
                                                                                   a_error, b_error)
                                isSFfound = True

        return [tof_axis, y_data, y_data_error, isSFfound]

    else:

        print '-> scaling factor file for requested lambda NOT FOUND!'
        return [tof_axis, y_data, y_data_error]

def applyScalingFactorToArray(tof_axis, y_data, y_data_error, a, b, a_error, b_error):
    """
    This function will create for each x-axis value the corresponding
    scaling factor using the formula y=a+bx and
    """

    x_axis = tof_axis
    nbr_tof = len(x_axis)-1
    x_axis_factors = zeros(nbr_tof)
    x_axis_factors_error = zeros(nbr_tof)
#    x_axis_factors = empty(nbr_tof)
#    x_axis_factors_error = empty(nbr_tof)
#    x_axis_factors[:] = NAN
#    x_axis_factors_error[:] = NAN
    for i in range(nbr_tof):
        _x_value = float(x_axis[i])
        _factor = _x_value * b + a
        x_axis_factors[i] = _factor
        _factor_error = _x_value * b_error + a_error
        x_axis_factors_error[i] = _factor_error

    sz = y_data.shape
    nbr_pixel = sz[0]

    final_y_data = zeros((nbr_pixel, nbr_tof))
    final_y_data_error = zeros((nbr_pixel, nbr_tof))
#    final_y_data = empty((nbr_pixel, nbr_tof))
#    final_y_data_error = empty((nbr_pixel, nbr_tof))
#    final_y_data[:] = NAN
#    final_y_data_error[:] = NAN
    for x in range(nbr_pixel):

        [ratio_array, ratio_array_error] = divideArrays(y_data[x,:],
                                                        y_data_error[x,:],
                                                        x_axis_factors,
                                                        x_axis_factors_error)

        final_y_data[x,:] = ratio_array[:]
        final_y_data_error[x,:] = ratio_array_error

    return [final_y_data, final_y_data_error]

def divideArrays(num_array, num_error_array, den_array, den_error_array):
    """
    This function calculates the ratio of two arrays and calculate the
    respective error values
    """

    sz = num_array.shape
    nbr_elements = sz[0]

    # calculate the ratio array
    ratio_array = zeros(nbr_elements)
    for i in range(nbr_elements):
        if den_array[i] is 0:
            _tmp_ratio = 0
        else:
            _tmp_ratio = num_array[i] / den_array[i]
        ratio_array[i] = _tmp_ratio

    # calculate the error of the ratio array
    ratio_error_array = zeros(nbr_elements)
    for i in range(nbr_elements):

        if (num_array[i] == 0) or (den_array[i] == 0):
            ratio_error_array[i] = 0
        else:
            tmp1 = pow(num_error_array[i] / num_array[i],2)
            tmp2 = pow(den_error_array[i] / den_array[i],2)
            ratio_error_array[i] = sqrt(tmp1+tmp2)*(num_array[i]/den_array[i])

    return [ratio_array, ratio_error_array]

def getCentralPixel(ws_event_data, dataPeakRange, is_new_geometry):
    """
    This function will calculate the central pixel position
    """

    if is_new_geometry:
        _maxX = 256
        _maxY = 304
    else:
        _maxX = 304
        _maxY = 256

    pixelXtof_data = getPixelXTOF(ws_event_data, maxX=_maxX, maxY=_maxY)
    pixelXtof_1d = pixelXtof_data.sum(axis=1)
    # Keep only range of pixels
    pixelXtof_roi = pixelXtof_1d[dataPeakRange[0]:dataPeakRange[1]]
    sz = pixelXtof_roi.size
    _num = 0
    _den = 0
    start_pixel = dataPeakRange[0]
    for i in range(sz):
        _num += (start_pixel * pixelXtof_roi[i])
        start_pixel = start_pixel + 1
        _den += pixelXtof_roi[i]
    data_cpix = _num / _den
    print '--> central pixel is {0:.1f}'.format(data_cpix)

    return data_cpix

def getDistances(ws_event_data):
    """
    calculates the distance between the moderator and the detector (dMD)
    and the distance between the sample and the detector
    """

    print '--> calculating dMD (moderator-detector) and dSD (sample-detector)'
    sample = ws_event_data.getInstrument().getSample()
    source = ws_event_data.getInstrument().getSource()
    dSM = sample.getDistance(source)

    # Create array of distances pixel->sample
    dPS_array = zeros((256, 304))
    for x in range(304):
        for y in range(256):
            _index = 256 * x + y
            detector = ws_event_data.getDetector(_index)
            dPS_array[y, x] = sample.getDistance(detector)

    # Array of distances pixel->source
    dMP_array = dPS_array + dSM
    # Distance sample->center of detector
    dSD = dPS_array[256./2.,304./2.]
    # Distance source->center of detector
    dMD = dSD + dSM

    return [dMD, dSD]

def  getTheta(ws_event_data, angleOffsetDeg):
    """
    will calculate the theta angle offset
    """
    print '--> retrieving thi and tthd'
    mt_run = ws_event_data.getRun()
    thi_value = mt_run.getProperty('thi').value[0]
    thi_units = mt_run.getProperty('thi').units
    tthd_value = mt_run.getProperty('tthd').value[0]
    tthd_units = mt_run.getProperty('tthd').units
    thi_rad = angleUnitConversion(value=thi_value,
                                  from_units=thi_units,
                                  to_units='rad')
    print '---> thi (rad): ', thi_rad
    tthd_rad = angleUnitConversion(value=tthd_value,
                                   from_units=tthd_units,
                                   to_units='rad')
    print '---> tthd (rad): ', tthd_rad

    theta = math.fabs(tthd_rad - thi_rad)/2.
    angleOffsetRad = (angleOffsetDeg * math.pi) / 180.
    theta += angleOffsetRad
    print '---> theta (rad): ', theta

    return theta

def getSlitsSize(mt):
    print '---> retrieving slits size'
    first_slit_size = getSheight(mt, '1')
    last_slit_size = getSheight(mt,'2')
    print '----> first_slit_size: ' , first_slit_size
    print '----> last_slit_size: ' , last_slit_size
    return [first_slit_size, last_slit_size]

def getQrange(ws_histo_data, theta, dMD, q_min, q_step):
    """
    will determine the true q axis according to the qMin and qStep specified
    and the geometry of the instrument
    """
    print '---> calculating Qrange'
    _tof_axis = ws_histo_data.readX(0)
    _const = float(4) * math.pi * m * dMD / h
    sz_tof = shape(_tof_axis)[0]
    _q_axis = zeros(sz_tof-1)
    for t in range(sz_tof-1):
        tof1 = _tof_axis[t]
        tof2 = _tof_axis[t+1]
        tofm = (tof1+tof2)/2.
        _Q = _const * math.sin(theta) / (tofm*1e-6)
        _q_axis[t] = _Q*1e-10
    q_max = max(_q_axis)
    if q_min >= q_max:
        q_min = min(_q_axis)
    print '----> q_min: ', q_min
    print '----> q_step: ', q_step
    print '----> q_max: ', q_max

    return [q_min, q_step, q_max]

def convertToQ(tof_axis,
               y_axis,
               y_error_axis,
               peak_range = None,
               central_pixel = None,
               source_to_detector_distance = None,
               sample_to_detector_distance = None,
               theta = None,
               first_slit_size = None,
               last_slit_size = None):
    """
    will convert the tof_axis into q_axis according to q range specified
    """

    y_size = (peak_range[1] - peak_range[0] + 1)
    y_range = arange(y_size) + peak_range[0]
    _q_axis = getQaxis(source_to_detector_distance,
                       sample_to_detector_distance,
                       theta,
                       tof_axis,
                       y_range,
                       central_pixel,
                       first_slit_size,
                       last_slit_size)

    _q_axis_min_max_index = findQaxisMinMax(_q_axis)


    # now we need to put the various counts from y_axis into the right
    # boxes
    _y_axis = zeros((y_size, len(tof_axis)-1))
    _y_error_axis = zeros((y_size, len(tof_axis)-1))
#    _y_axis = empty((y_size, len(tof_axis)-1))
#    _y_error_axis = empty((y_size, len(tof_axis)-1))
#    _y_axis[:] = NAN
#    _y_error_axis[:] = NAN

    # now determine the _y_axis and _y_error_axis
    for _y_index in range(y_size):

        # get the q_axis of the given peak pixel
        _tmp_q_axis = _q_axis[_y_index]
        q_axis = _tmp_q_axis[::-1] #reverse the axis (now in increasing order)

        _tmp_peak_pixel = y_range[_y_index]
        _y_axis_tmp = y_axis[_y_index,:]
        _y_error_axis_tmp = y_error_axis[_y_index,:]

        # keep only the overlap region of Qs
        _q_min = _q_axis_min_max_index[_y_index, 0]
        if _q_min != 0:
            _y_axis_tmp[0:_q_min] = 0
            _y_error_axis_tmp[0:_q_min] = 0

        _q_max = int(_q_axis_min_max_index[_y_index, 1])
        sz = shape(_y_axis_tmp)[0]
        if _q_max != sz:
            _index_q_max_range = arange(sz - _q_max) + _q_max
            for i in _index_q_max_range:
                _y_axis_tmp[i] = 0
                _y_error_axis_tmp[i] = 0

        _y_axis[_y_index, :] = _y_axis_tmp[::-1]
        _y_error_axis[_y_index, :] = _y_error_axis_tmp[::-1]

    # reverse the _q_axis here as well
    q_axis_reverse = reverseQAxis(_q_axis)

    return [q_axis_reverse, _y_axis, _y_error_axis]

def convertToQWithoutCorrection(tof_axis,\
               y_axis,\
               y_error_axis,\
               peak_range = None,\
               source_to_detector_distance = None,\
               sample_to_detector_distance = None,\
               theta = None,\
               first_slit_size = None,\
               last_slit_size = None):
    """
    will convert the tof_axis into q_axis according to q range specified
    but without using any geometry correction
    """

    _const = float(4) * math.pi * m * source_to_detector_distance / h
    _q_axis = 1e-10 * _const * math.sin(theta) / (tof_axis[0:-1] * 1e-6)

    sz = y_axis.shape
    nbr_pixel = sz[0]

    sz_q_axis = _q_axis.shape
    nbr_q = sz_q_axis[0]

    q_axis_2d = zeros((nbr_pixel, nbr_q))
    for p in range(nbr_pixel):
        q_axis_2d[p,:] = _q_axis

    q_axis_reverse = reverseQAxis(q_axis_2d)
    y_axis_reverse = fliplr(y_axis)
    y_error_axis_reverse = fliplr(y_error_axis)

    return [q_axis_reverse, y_axis_reverse, y_error_axis_reverse]

def reverseQAxis(q_axis):
    """
    will reverse each q_axis for the respective pixels
    """
    new_q_axis = fliplr(q_axis)
    return new_q_axis

def getQaxis(dMD, dSD, theta,
             tof_axis, y_range, central_pixel,
             first_slit_size,
             last_slit_size):
    """
    This function converts the pixel/TOF array to the R(Q) array
    using Q = (4.Pi.Mn)/h  *  L.sin(theta/2)/TOF
    with    L: distance central_pixel->source
            TOF: TOF of pixel
            theta: angle of detector
    """

    _const = float(4) * math.pi * m * dMD / h
    sz_tof = len(tof_axis)
    tmp_q_axis = zeros(sz_tof)
    q_array = zeros((len(y_range), sz_tof))

    index_y = range(len(y_range))
    for y in index_y:
        _px = y_range[y]
        dangle = ref_beamdiv_correct(central_pixel,
                                     dSD,
                                     _px,
                                     0.0007,
                                     first_slit_size,
                                     last_slit_size)

        if dangle is not None:
            _theta = theta + dangle
        else:
            _theta = theta

        for t in range(sz_tof):
#            tof1 = tof_axis[t]
#            tof2 = tof_axis[t+1]
#            tofm = (tof1+tof2)/2.
            tof = tof_axis[t]
#            _Q = _const * math.sin(_theta) / (tofm*1e-6)
            _Q = _const * math.sin(_theta) / (tof*1e-6)
            q_array[y, t] = _Q * 1e-10

    return q_array

def integrateOverPeakRange(wks, dataPeakRange):
    """
        getting just the mean of the peak
    """

    final_x_axis = wks.readX(0)[:]
    sz = final_x_axis.shape
    nbr_q = sz[0]

    # make temp big array
    nbrPixel = dataPeakRange[1] - dataPeakRange[0] + 1
    bigY = zeros((nbrPixel, nbr_q))
    bigE = zeros((nbrPixel, nbr_q))
#    bigY = empty((nbrPixel, nbr_q))
#    bigE = empty((nbrPixel, nbr_q))
#    bigY[:]= NAN
#    bigE[:]= NAN
    for x in range(nbrPixel):
        _tmp_y = wks.readY(x)[:]
        bigY[x,:] = _tmp_y
        _tmp_e = wks.readE(x)[:]
        bigE[x,:] = _tmp_e

    final_y_axis = zeros(nbr_q)
    final_y_error_axis = zeros(nbr_q)
#
#    final_y_axis = empty(nbr_q)
#    final_y_error_axis = empty(nbr_q)
#    final_y_axis[:] = NAN
#    final_y_error_axis[:] = NAN

    # range(nbr_q -2) + 1 to get rid of first and last q values (edge effect)
    rangeOfQ = range(nbr_q-1)
#    for q in rangeOfQ[1:-1]:
    for q in rangeOfQ:

        _tmp_y = bigY[:,q]
        _tmp_y_error = bigE[:,q]

#         [_y, _y_error] = myMean(_tmp_y, _tmp_y_error)
        [_y, _y_error] = sumWithError(_tmp_y, _tmp_y_error)

        final_y_axis[q] = _y
        final_y_error_axis[q] = _y_error

    return [final_x_axis, final_y_axis, final_y_error_axis]

def createQworkspace(q_axis, y_axis, y_error_axis):

    sz = q_axis.shape
    nbr_pixel = sz[0]
    nbr_tof = sz[1]

    q_axis_1d = q_axis.flatten()
    y_axis_1d = y_axis.flatten()
    y_error_axis_1d = y_error_axis.flatten()

    q_workspace = CreateWorkspace(DataX=q_axis_1d,\
                           DataY=y_axis_1d,\
                           DataE=y_error_axis_1d,\
                           Nspec=nbr_pixel,\
                           UnitX="Wavelength")
    q_workspace.setDistribution(True)

    return q_workspace

def createFinalWorkspace(q_axis, final_y_axis, final_error_axis, name_output_ws, parent_workspace):

    final_workspace = CreateWorkspace(OutputWorkspace=name_output_ws,
                                      DataX=q_axis,
                                      DataY=final_y_axis,
                                      DataE=final_error_axis,
                                      Nspec=1,
                                      UnitX="Wavelength",
                                      ParentWorkspace=parent_workspace)
    final_workspace.setDistribution(True)

    return final_workspace

def cropAxisToOnlyNonzeroElements(q_rebin, dataPeakRange):
    """
    This function will only keep the range of Q that have only nonzero counts
    """
    nbrPixel = dataPeakRange[1] - dataPeakRange[0] + 1

    x_axis = q_rebin.readX(0)[:]
    sz = x_axis.shape[0]-1

    index_first_non_zero_value = sz
    index_last_non_zero_value = 0

    for x in range(nbrPixel):
        _pixel_axis = q_rebin.readY(x)[:]

        for t in range(sz):
            _value = _pixel_axis[t]
            if _value != float(0):
                if index_first_non_zero_value > t:
                    index_first_non_zero_value = t
                break

        for t in range(sz-1,-1,-1):
            _value = _pixel_axis[t]
            if _value != float(0):
                if index_last_non_zero_value < t:
                    index_last_non_zero_value = t
                break

    # crop data
    new_x_axis = x_axis[index_first_non_zero_value:index_last_non_zero_value+1]
    new_xrange = index_last_non_zero_value - index_first_non_zero_value + 1

    new_y_axis = zeros((nbrPixel, new_xrange))
    new_y_error_axis = zeros((nbrPixel, new_xrange))
#    new_y_axis = empty((nbrPixel, new_xrange))
#    new_y_error_axis = empty((nbrPixel, new_xrange))
#    new_y_axis[:] = NAN
#    new_y_error_axis[:] = NAN

    for x in range(nbrPixel):
        _tmp = q_rebin.readY(x)[:]
        _tmp_E = q_rebin.readE(x)[:]
        new_y_axis[x,:] = _tmp[index_first_non_zero_value:index_last_non_zero_value+1]
        new_y_error_axis[x,:] = _tmp_E[index_first_non_zero_value:index_last_non_zero_value+1]

    new_y_axis = new_y_axis.flatten()
    new_y_error_axis = new_y_error_axis.flatten()

    new_x_axis = asfarray(new_x_axis)
    new_y_axis = asfarray(new_y_axis)
    new_y_error_axis = asfarray(new_y_error_axis)

    nonzero_q_rebin_wks = CreateWorkspace(DataX=new_x_axis,
                                          DataY=new_y_axis,
                                          DataE=new_y_error_axis,
                                          Nspec=int(nbrPixel),
                                          UnitX="Wavelength")

    return nonzero_q_rebin_wks

def cleanupData(final_data_y_axis, final_data_y_error_axis):

    sz = final_data_y_axis.shape
    nbrPixel = sz[0]
    nbrQ = sz[1]

    for x in range(nbrPixel):
        for q in range(nbrQ):

            _data = final_data_y_axis[x,q]
            _error = final_data_y_error_axis[x,q]

            # if error is > value, remove point
            if _error >= _data:
                _data = 0
                _error = 1

            # if value is below 10^-12
            if _data < 1e-12:
                _data = 0
                _error = 1

            final_data_y_axis[x,q] = _data
            final_data_y_error_axis[x,q] = _error

    return [final_data_y_axis, final_data_y_error_axis]



def cleanupData1D(final_data_y_axis, final_data_y_error_axis):

    sz = final_data_y_axis.shape
    nbrTof = sz[0]

    notYetRemoved = True

    for t in range(nbrTof):

        _data = final_data_y_axis[t]
        _error = final_data_y_error_axis[t]

        if _data > 0 and notYetRemoved:
            notYetRemoved = False
            final_data_y_axis[t] = 0
            final_data_y_error_axis[t] = 1
            continue

        # if error is > value, remove point
        if abs(_error) >= abs(_data):
            _data_tmp = 0
            _error_tmp = 1
        elif _data< 1e-12:
        # if value is below 10^-12
            _data_tmp = 0
            _error_tmp = 1
        else:
            _data_tmp = _data
            _error_tmp = _error

        final_data_y_axis[t] = _data_tmp
        final_data_y_error_axis[t] = _error_tmp

#        print 'final_data_y_axis[t]: ' , _data_tmp , ' final_data_y_error_axis[t]: ' , _error_tmp

    return [final_data_y_axis, final_data_y_error_axis]

def isNexusTakeAfterRefDate(nexus_date):
    '''
   This function parses the output.date and returns true if this date is after the ref date
   '''
    nexus_date_acquistion = nexus_date.split('T')[0]

    if nexus_date_acquistion > ref_date:
        return True
    else:
        return False








