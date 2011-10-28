from numpy import zeros
from pylab import *
from MantidFramework import *
from mantidsimple import *


def create_integrated_workspace(mt1, proton_charge, from_pixel=0, to_pixel=303):
    """
        This creates the integrated workspace over the second pixel range (304 here) and
        returns the new workspace handle
    """
    _x_axis = mt1.readX(0)[:]
    x_size = to_pixel - from_pixel + 1 
    _y_axis = zeros((256, len(_x_axis) - 1))
    _y_error_axis = zeros((256, len(_x_axis) - 1))
    y_range = arange(x_size) + from_pixel
    for x in range(304):
        for y in y_range:
            _index = int(256 * x + y)
            _y_axis[y, :] += mt1.readY(_index)[:]
            _y_error_axis[y, :] += ((mt1.readE(_index)[:]) * (mt1.readE(_index)[:]))

    _y_axis = _y_axis.flatten()
    _y_error_axis = sqrt(_y_error_axis)
    #plot_y_error_axis = _y_error_axis #for output testing only    -> plt.imshow(plot_y_error_axis, aspect='auto', origin='lower')
    _y_error_axis = _y_error_axis.flatten()

    #normalization by proton charge
    _y_axis /= (proton_charge * 1e-12)

    CreateWorkspace("IntegratedDataWks", DataX=_x_axis, DataY=_y_axis, DataE=_y_error_axis, Nspec=256)
    mt3 = mtd['IntegratedDataWks']
    return mt3