#  This file is part of the mantid package
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
import numpy
import mantid.kernel
import mantid.api
import mantid.dataobjects


def _getWkspIndexDistAndLabel(workspace, **kwargs):
    '''
    Get workspace index, whether the workspace is a distribution,
    and label for the spectrum

    :param workspace: a Workspace2D or an EventWorkspace
    '''
    # get the special arguments out of kwargs
    specNum = kwargs.pop('specNum', None)
    wkspIndex = kwargs.pop('wkspIndex', None)

    # don't worry if there is only one spectrum
    if workspace.getNumberHistograms() == 1:
        specNum = None
        wkspIndex = 0

    # error check input parameters
    if (specNum is not None) and (wkspIndex is not None):
        raise RuntimeError('Must specify only specNum or wkspIndex')
    if (specNum is None) and (wkspIndex is None):
        raise RuntimeError('Must specify either specNum or wkspIndex')

    # convert the spectrum number to a workspace index and vice versa
    if specNum is not None:
        wkspIndex = workspace.getIndexFromSpectrumNumber(int(specNum))
    else:
        specNum = workspace.getSpectrum(wkspIndex).getSpectrumNo()

    # create a label if it isn't already specified
    if 'label' not in kwargs:
        kwargs['label'] = 'spec {0}'.format(specNum)

    (distribution, kwargs) = _getDistribution(workspace, **kwargs)

    return (wkspIndex, distribution, kwargs)


def _getDistribution(workspace, **kwargs):
    '''Determine whether or not the data is a distribution. The value in
    the kwargs wins.'''
    distribution = kwargs.pop('distribution', workspace.isDistribution())
    return (bool(distribution), kwargs)


def _getNormalization(mdworkspace, **kwargs):
    '''gets the normalization flag of an MDHistoWorkspace'''
    normalization = kwargs.pop('normalization', mdworkspace.displayNormalizationHisto())
    return (normalization,kwargs)


def getAxesLabels(workspace):
    ''' get axis labels from a Workspace2D or an MDHistoWorkspace'''
    if isinstance(workspace,mantid.dataobjects.MDHistoWorkspace):
        axes = ['Intensity']
        dims = workspace.getNonIntegratedDimensions()
        for d in dims:
            axis_title = d.name.replace('DeltaE','$\Delta E$')
            axis_unit = d.getUnits().replace('Angstrom^-1','$\AA^{-1}$')
            axis_unit = axis_unit.replace('DeltaE','meV')
            axis_unit = axis_unit.replace('Angstrom','$\AA$')
            axis_unit = axis_unit.replace('MomentumTransfer','$\AA^{-1}$')
            axes.append('{0} ({1})'.format(axis_title,axis_unit))
    else:
        '''For matrix workspaces, return a tuple of ``(YUnit, <other units>)``'''
        axes = [workspace.YUnit()] #TODO: deal with distribution
        for index in range(workspace.axes()):
            axis = workspace.getAxis(index)
            unit = axis.getUnit()
            if len(str(unit.symbol())) > 0:
                unit = '{} (${}$)'.format(unit.caption(), unit.symbol().latex())
            else:
                unit = unit.caption()
            axes.append(unit)
    return tuple(axes)


def _setLabels1D(axes, workspace):
    labels = getAxesLabels(workspace)
    axes.set_xlabel(labels[1])
    axes.set_ylabel(labels[0])


def _setLabels2D(axes, workspace):
    labels = getAxesLabels(workspace)
    axes.set_xlabel(labels[1])
    axes.set_ylabel(labels[2])


def _getSpectrum(workspace, wkspIndex, distribution, withDy=False, withDx=False):
    '''Extract a single spectrum and process the data into a frequency'''
    x = workspace.readX(wkspIndex)
    y = workspace.readY(wkspIndex)
    dy = None
    dx = None

    if withDy:
        dy = workspace.readE(wkspIndex)
    if withDx and workspace.getSpectrum(wkspIndex).hasDx():
        dx = workspace.readDx(wkspIndex)

    if workspace.isHistogramData():
        if (not distribution) and (mantid.kernel.config['graph1d.autodistribution']=='On'):
            y = y / (x[1:] - x[0:-1])
            if dy is not None:
                dy = dy / (x[1:] - x[0:-1])
        x = points_from_boundaries(x)

    return (x,y,dy,dx)


def _dim2array(d):
    '''
    Create a numpy array containing bin centers along the dimension d
    input: d - IMDDimension
    returns: bin boundaries for dimension d
    '''
    dmin=d.getMinimum()
    dmax=d.getMaximum()
    return numpy.linspace(dmin,dmax,d.getNBins()+1)


def boundaries_from_points(input_array):
    assert isinstance(input_array,numpy.ndarray),'Not a numpy array'
    if len(input_array)==0:
        raise ValueError('could not extend array with no elements')
    if len(input_array)==1:
        return numpy.array([input_array[0]-0.5,input_array[0]+0.5])
    return numpy.concatenate(([(3*input_array[0]-input_array[1])*0.5],
                           (input_array[1:]+input_array[:-1])*0.5,
                           [(3*input_array[-1]-input_array[-2])*0.5]))

    
def points_from_boundaries(input_array):
    assert isinstance(input_array,numpy.ndarray),'Not a numpy array'
    if len(input_array)<2:
        raise ValueError('could not get centers from less than two boundaries')
    return (.5*(input_array[0:-1]+input_array[1:]))


def _getMDData(workspace,normalization,withError=False):
    dims=workspace.getNonIntegratedDimensions()
    dimarrays=[_dim2array(d) for d in dims]
    #get data
    data=workspace.getSignalArray()*1.
    if normalization==mantid.api.MDNormalization.NumEventsNormalization:
        nev=workspace.getNumEventsArray()
        data/=nev
    err=None
    if withError:
        err2=workspace.getErrorSquaredArray()*1.
        if normalization==mantid.api.MDNormalization.NumEventsNormalization:
            err2/=(nev*nev)
        err=numpy.sqrt(err2)
    data=data.squeeze()
    if err is not None:
        err=err.squeeze()
    return (dimarrays,data,err)


def _getMDData1D(workspace,normalization):
    coordinate,data,err=_getMDData(workspace,normalization,withError=True)
    assert len(coordinate)==1, 'The workspace is not 1D'
    coordinate=points_from_boundaries(coordinate[0])
    return (coordinate,data,err)


def _getMDData2D_bin_bounds(workspace,normalization,withError=False):
    coordinate,data,err=_getMDData(workspace,normalization,withError)
    assert len(coordinate)==2, 'The workspace is not 2D'
    return (coordinate,data,err)

def _getMDData2D_bin_centers(workspace,normalization,withError=False):
    coordinate,data,err=_getMDData2D_bin_bounds(workspace,normalization,withError)
    coordinate[0]=points_from_boundaries(coordinate[0])
    coordinate[1]=points_from_boundaries(coordinate[1])
    return (coordinate,data,err)

def _commonX(arr):
    return numpy.all(arr==arr[0,:],axis=(1,0))

def _getMatrix2DData(workspace, distribution,histogram2D=False):
    try:
        _=workspace.blocksize()
    except RuntimeError:
        raise ValueError('The spectra are not the same length. Try using pcolor, pcolorfast, or pcolormesh instead')
    x = workspace.extractX()
    y = workspace.getAxis(1).extractValues()
    z = workspace.extractY()

    if workspace.isHistogramData():
        if not distribution:
            z = z / (x[:,1:] - x[:,0:-1])
        if histogram2D:
            if len(y)==z.shape[0]:
                y = boundaries_from_points(y)
            x = numpy.vstack((x,x[-1]))
        else:
            x = .5*(x[:,0:-1]+x[:,1:])
            if len(y)==z.shape[0]+1:
                y=points_from_boundaries(y)
    else:
        if histogram2D:
            if _commonX(x):
                x=numpy.broadcast_to(numpy.expand_dims(boundaries_from_points(x[0]),0),(z.shape[0]+1,z.shape[1]+1))
            else:
                x = numpy.vstack((x,x[-1]))
                x = numpy.array([boundaries_from_points(xi) for xi in x])
            if len(y)==z.shape[0]:
                y=boundaries_from_points(y)
        else:
            if len(y)==z.shape[0]+1:
                y=points_from_boundaries(y)
    y = numpy.broadcast_to(numpy.expand_dims(y,1),x.shape)
    return (x,y,z)

def _getUnevenData(workspace, distribution):
    z=[]
    x=[]
    y=[]
    nhist=workspace.getNumberHistograms()
    yvals=workspace.getAxis(1).extractValues()
    if len(yvals)==(nhist):
        yvals=boundaries_from_points(yvals)
    for index in range(nhist):
        xvals=workspace.readX(index)
        zvals=workspace.readY(index)
        if workspace.isHistogramData():
            if not distribution:
                zvals = zvals / (xvals[1:] - xvals[0:-1])
        else:
            xvals=boundaries_from_points(xvals)
        z.append(zvals)
        x.append(xvals)
        y.append([yvals[index],yvals[index+1]])
    return(x,y,z)


####TODO: delete
def _getContour(workspace, distribution):
    x = workspace.extractX()
    z = workspace.extractY()

    if workspace.isHistogramData():
        if not distribution:
            z = z / (x[:,1:] - x[:,0:-1])
        x = .5*(x[:,0:-1]+x[:,1:])

    # y axis is held differently
    y = workspace.getAxis(1).extractValues()
    if len(y) == x.shape[0]+1:
        y = .5*(y[0:-1]+y[1:])
    y = numpy.tile(y, (x.shape[1], 1)).transpose()

    return (x,y,z)





def plot(axes, workspace, *args, **kwargs):
    '''
    Unpack mantid workspace and render it with matplotlib. ``args`` and
    ``kwargs`` are passed to :py:meth:`matplotlib.axes.Axes.plot` after special
    keyword arguments are removed. This will automatically label the
    line according to the spectrum number unless specified otherwise.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param specNum:   spectrum number to plot
    :param wkspIndex: workspace index to plot
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a histogram.

    For matrix workspaces with more than one spectra, either ``specNum`` or ``wkspIndex``
    needs to be specified. Giving both will generate a :class:`RuntimeError`. There is no similar
    keyword for MDHistoWorkspaces. These type of workspaces have to have exactly one non integrated
    dimension
    '''
    if isinstance(workspace,mantid.dataobjects.MDHistoWorkspace):
        (normalization,kwargs)=_getNormalization(workspace, **kwargs)
        (x,y,_)=_getMDData1D(workspace,normalization)
    else:
        (wkspIndex, distribution, kwargs) = _getWkspIndexDistAndLabel(workspace, **kwargs)
        (x, y, _, _) = _getSpectrum(workspace, wkspIndex, distribution, withDy=False, withDx=False)
    _setLabels1D(axes, workspace)
    return axes.plot(x, y, *args, **kwargs)


def errorbar(axes, workspace, *args, **kwargs):
    '''
    Unpack mantid workspace and render it with matplotlib. ``args`` and
    ``kwargs`` are passed to :py:meth:`matplotlib.axes.Axes.errorbar` after special
    keyword arguments are removed. This will automatically label the
    line according to the spectrum number unless specified otherwise.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    :param specNum:   spectrum number to plot
    :param wkspIndex: workspace index to plot
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a histogram.

    Either ``specNum`` or ``wkspIndex`` needs to be specified. Giving
    both will generate a :class:`RuntimeError`.
    '''
    (wkspIndex, distribution, kwargs) = _getWkspIndexDistAndLabel(workspace, kwargs)
    (x, y, dy, dx) = _getSpectrum(workspace, wkspIndex, distribution, withDy=True, withDx=True)
    return axes.errorbar(x, y, dy, dx, *args, **kwargs)


def scatter(axes, workspace, *args, **kwargs):
    '''
    Unpack mantid workspace and render it with matplotlib. ``args`` and
    ``kwargs`` are passed to :py:meth:`matplotlib.axes.Axes.scatter` after special
    keyword arguments are removed. This will automatically label the
    line according to the spectrum number unless specified otherwise.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    :param specNum:   spectrum number to plot
    :param wkspIndex: workspace index to plot
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a histogram.

    Either ``specNum`` or ``wkspIndex`` needs to be specified. Giving
    both will generate a :class:`RuntimeError`.
    '''
    (wkspIndex, distribution, kwargs) = _getWkspIndexDistAndLabel(workspace, kwargs)
    (x, y, _, _) = _getSpectrum(workspace, wkspIndex, distribution)
    return axes.scatter(x, y, *args, **kwargs)



def contour(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`matplotlib.axes.Axes.contour`
    but calculates the countour levels. Currently this only works with
    workspaces that have a constant number of bins between spectra.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a histogram.
    '''
    (distribution, kwargs) = _getDistribution(workspace, **kwargs)
    (x,y,z) = _getContour(workspace, distribution)
    _setLabels2D(axes, workspace)

    return axes.contour(x, y, z, *args, **kwargs)


def contourf(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`matplotlib.axes.Axes.contourf`
    but calculates the countour levels. Currently this only works with
    workspaces that have a constant number of bins between spectra.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a histogram.
    '''
    (distribution, kwargs) = _getDistribution(workspace, **kwargs)
    (x,y,z) = _getContour(workspace, distribution)
    _setLabels2D(axes, workspace)

    return axes.contourf(x, y, z, *args, **kwargs)

def pcolor(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`matplotlib.axes.Axes.pcolor`
    but calculates the countour levels. Currently this only works with
    workspaces that have a constant number of bins between spectra.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a histogram.
    '''
    (distribution, kwargs) = _getDistribution(workspace, **kwargs)
    (x,y,z) = _getContour(workspace, distribution)
    _setLabels2D(axes, workspace)

    return axes.pcolor(x, y, z, *args, **kwargs)

def pcolorfast(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`matplotlib.axes.Axes.pcolorfast`
    but calculates the countour levels. Currently this only works with
    workspaces that have a constant number of bins between spectra.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a histogram.
    '''
    (distribution, kwargs) = _getDistribution(workspace, **kwargs)
    (x,y,z) = _getContour(workspace, distribution)
    _setLabels2D(axes, workspace)

    return axes.pcolorfast(x, y, z, *args, **kwargs)

def pcolormesh(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`matplotlib.axes.Axes.pcolormesh`
    but calculates the countour levels. Currently this only works with
    workspaces that have a constant number of bins between spectra.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a histogram.
    '''
    (distribution, kwargs) = _getDistribution(workspace, **kwargs)
    (x,y,z) = _getContour(workspace, distribution)
    _setLabels2D(axes, workspace)

    return axes.pcolormesh(x, y, z, *args, **kwargs)

def tripcolor(axes, workspace, *args, **kwargs):
    '''
    To be used with non-uniform grids. Currently this only works with workspaces
    that have a constant number of bins between spectra.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a histogram.

    See :meth:`matplotlib.axes.Axes.tripcolor` for more information.
    '''
    (distribution, kwargs) = _getDistribution(workspace, **kwargs)
    (x,y,z) = _getContour(workspace, distribution)
    _setLabels2D(axes, workspace)

    return axes.tripcolor(x.ravel(), y.ravel(), z.ravel(), *args, **kwargs)

def triplot(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`mantid.plots.pcolor`, but works
    for non-uniform grids. Currently this only works with workspaces
    that have a constant number of bins between spectra.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a histogram.

    See :meth:`matplotlib.axes.Axes.triplot` for more information.
    '''
    (distribution, kwargs) = _getDistribution(workspace, **kwargs)
    (x,y,z) = _getContour(workspace, distribution)
    _setLabels2D(axes, workspace)

    return axes.triplot(x.ravel(), y.ravel(), z.ravel(), *args, **kwargs)

def tricontour(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`mantid.plots.contour`, but works
    for non-uniform grids. Currently this only works with workspaces
    that have a constant number of bins between spectra.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a histogram.

    See :meth:`matplotlib.axes.Axes.tricontour` for more information.
    '''
    (distribution, kwargs) = _getDistribution(workspace, **kwargs)
    (x,y,z) = _getContour(workspace, distribution)
    _setLabels2D(axes, workspace)

    return axes.tricontour(x.ravel(), y.ravel(), z.ravel(), *args, **kwargs)


def tricontourf(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`mantid.plots.contourf`, but works
    for non-uniform grids. Currently this only works with workspaces
    that have a constant number of bins between spectra.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a histogram.

    See :meth:`matplotlib.axes.Axes.tricontourf` for more information.
    '''
    (distribution, kwargs) = _getDistribution(workspace, **kwargs)
    (x,y,z) = _getContour(workspace, distribution)
    _setLabels2D(axes, workspace)

    return axes.tricontourf(x.ravel(), y.ravel(), z.ravel(), *args, **kwargs)
