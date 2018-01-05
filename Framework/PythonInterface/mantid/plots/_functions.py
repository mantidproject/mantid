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
import matplotlib.colors


def _getDistribution(workspace, **kwargs):
    '''Determine whether or not the data is a distribution. The value in
    the kwargs wins. Applies to Matrix workspaces only
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from'''
    distribution = kwargs.pop('distribution', workspace.isDistribution())
    return (bool(distribution), kwargs)


def _getNormalization(mdworkspace, **kwargs):
    '''gets the normalization flag of an MDHistoWorkspace. For workspaces
    derived similar to MSlice/Horace, one needs to average data, the so-called
    "number of events" normalization.
    :param mdworkspace: :class:`mantid.dataobjects.MDHistoWorkspace` to extract the data from'''
    normalization = kwargs.pop('normalization', mdworkspace.displayNormalizationHisto())
    return (normalization,kwargs)


def getAxesLabels(workspace):
    ''' get axis labels from a Workspace2D or an MDHistoWorkspace
    Returns a tuple. The first element is the quantity label, such as "Intensity" or "Counts".
    All other elements in the tuple are labels for axes.
    Some of them are latef formatted already.'''
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


def _getSpectrum(workspace, wkspIndex, distribution, withDy=False, withDx=False):
    '''Extract a single spectrum and process the data into a frequency
    :param workspace: a Workspace2D or an EventWorkspace
    :param wkspIndex: workspace index
    :param distribution: flag to divide the data by bin width. It happens only
        when this flag is False, the workspace contains histogram data, and
        the mantid configuration is set up to divide such workspaces by bin
        width. The same effect can be obtained by running the
        :ref:`algm-ConvertToDistribution` algorithm
    :param withDy: if True, it will return the error in the "counts", otherwise None
    :param with Dx: if True, and workspace has them, it will return errors
        in the x coordinate, otherwise None
    Note that for workspaces containing bin boundaries, this function will return
    the bin centers for x.
    To be used in 1D plots (plot, scatter, errorbar)
    '''
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


def _commonX(arr):
    '''
    Helper functionto check if all rows in a 2d :class:`numpy.ndarray` are identical
    '''
    return numpy.all(arr==arr[0,:],axis=(1,0))


def _getMatrix2DData(workspace, distribution,histogram2D=False):
    '''
    Get all data from a Matrix workspace that has the same number of bins
    in every spectrum. It is used for 2D plots
    :param workspace: Matrix workspace to extract the data from
    :param distribution: if False, and the workspace contains histogram data,
        the intensity will be divided by the x bin width
    :param histogram2D: flag that specifies if the coordinates in the output are
        -bin centers (such as for contour) for False, or
        -bin edges (such as for pcolor) for True.
    Returns x,y,z 2D arrays
    '''
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


def _getDataUnevenFlag(workspace,**kwargs):
    '''
    Helper function that allows :meth:`matplotlib.axes.Axes.pcolor`,
    :meth:`matplotlib.axes.Axes.pcolorfast`, and :meth:`matplotlib.axes.Axes.pcolormesh`
    to plot rectangles parallel to the axes even if the data is not
    on a regular grid.
    :param workspace: a workspace2d
    if AxisAligned keyword is available and True or if the workspace does
    not have a constant number of bins, it will return true, otherwise false
    '''
    aligned = kwargs.pop('AxisAligned', False)
    try:
        _=workspace.blocksize()
    except RuntimeError:
        aligned=True
    return(aligned,kwargs)


def _getUnevenData(workspace, distribution):
    '''
    Function to get data for uneven workspace2Ds, such as
    that pcolor, pcolorfast, and pcolormesh will plot axis aligned rectangles
    :param workspace: a workspace2d
    :param distribution: if False, and the workspace contains histogram data,
        the intensity will be divided by the x bin width
    Returns three lists. Each element in the x list is an array of boundaries
    for a spectra. Each element in the y list is a 2 element array with the extents
    of a particular spectra. The z list contains arrays of intensities at bin centers
    '''
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


def _dim2array(d):
    '''
    Create a numpy array containing bin centers along the dimension d
    :param d: an :class:`mantid.geometry.IMDDimension` object

    returns: bin boundaries for dimension d
    '''
    dmin=d.getMinimum()
    dmax=d.getMaximum()
    return numpy.linspace(dmin,dmax,d.getNBins()+1)


def boundaries_from_points(input_array):
    '''
    The function tries to guess bin boundaries from bin centers
    :param input_array: a :class:`numpy.ndarray` of bin centers
    '''
    assert isinstance(input_array,numpy.ndarray),'Not a numpy array'
    if len(input_array)==0:
        raise ValueError('could not extend array with no elements')
    if len(input_array)==1:
        return numpy.array([input_array[0]-0.5,input_array[0]+0.5])
    return numpy.concatenate(([(3*input_array[0]-input_array[1])*0.5],
                           (input_array[1:]+input_array[:-1])*0.5,
                           [(3*input_array[-1]-input_array[-2])*0.5]))


def points_from_boundaries(input_array):
    '''
    The function returns bin centers from bin boundaries
    :param input_array: a :class:`numpy.ndarray` of bin boundaries
    '''
    assert isinstance(input_array,numpy.ndarray),'Not a numpy array'
    if len(input_array)<2:
        raise ValueError('could not get centers from less than two boundaries')
    return (.5*(input_array[0:-1]+input_array[1:]))


def _getMDData(workspace,normalization,withError=False):
    '''
    generic function to extract data from an MDHisto workspace
    :param workspace: :class:`mantid.dataobjects.MDHistoWorkspace` containing data
    :param normalization: if :class:`mantid.api.MDNormalization.NumEventsNormalization`
        it will divide intensity by the number of corresponding MDEvents
    returns a tuple containing bin boundaries for each dimension, the (maybe normalized)
    signal and error arrays
    '''
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
    data=data.squeeze().T
    if err is not None:
        err=err.squeeze().T
    return (dimarrays,data,err)


def _getMDData1D(workspace,normalization):
    '''
    Function to transform data in an MDHisto workspace with exactly
    one non-integrated dimension into arrays of bin centers, data,
    and error, to be used in 1D plots (plot, scatter, errorbar)
    '''
    coordinate,data,err=_getMDData(workspace,normalization,withError=True)
    assert len(coordinate)==1, 'The workspace is not 1D'
    coordinate=points_from_boundaries(coordinate[0])
    return (coordinate,data,err)


def _getMDData2D_bin_bounds(workspace,normalization):
    '''
    Function to transform data in an MDHisto workspace with exactly
    two non-integrated dimension into arrays of bin boundaries in each
    dimension, and data. To be used in 2D plots (pcolor, pcolorfast, pcolormesh)
    Note return coordinates are 1d vectors. Use numpy.meshgrid to generate 2d versions
    '''
    coordinate,data,_=_getMDData(workspace,normalization,withError=False)
    assert len(coordinate)==2, 'The workspace is not 2D'
    return (coordinate[0],coordinate[1],data)


def _getMDData2D_bin_centers(workspace,normalization):
    '''
    Function to transform data in an MDHisto workspace with exactly
    two non-integrated dimension into arrays of bin centers in each
    dimension, and data. To be used in 2D plots (contour, contourf,
    tricontour, tricontourf, tripcolor)
    Note return coordinates are 1d vectors. Use numpy.meshgrid to generate 2d versions
    '''
    x,y,data=_getMDData2D_bin_bounds(workspace,normalization,withError=False)
    x=points_from_boundaries(x)
    y=points_from_boundaries(y)
    return (x,y,data)


def _setLabels1D(axes, workspace):
    '''
    helper function to automatically set axes labels for 1D plots
    '''
    labels = getAxesLabels(workspace)
    axes.set_xlabel(labels[1])
    axes.set_ylabel(labels[0])


def _setLabels2D(axes, workspace):
    '''
    helper function to automatically set axes labels for 2D plots
    '''
    labels = getAxesLabels(workspace)
    axes.set_xlabel(labels[1])
    axes.set_ylabel(labels[2])


def plot(axes, workspace, *args, **kwargs):
    '''
    Unpack mantid workspace and render it with matplotlib. ``args`` and
    ``kwargs`` are passed to :py:meth:`matplotlib.axes.Axes.plot` after special
    keyword arguments are removed. This will automatically label the
    line according to the spectrum number unless specified otherwise.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param specNum:   spectrum number to plot if MatrixWorkspace
    :param wkspIndex: workspace index to plot if MatrixWorkspace
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a MatrixWorkspace histogram.

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
    if isinstance(workspace,mantid.dataobjects.MDHistoWorkspace):
        (normalization,kwargs)=_getNormalization(workspace, **kwargs)
        (x,y,dy)=_getMDData1D(workspace,normalization)
        dx=None
    else:
        (wkspIndex, distribution, kwargs) = _getWkspIndexDistAndLabel(workspace, **kwargs)
        (x, y, dy, dx) = _getSpectrum(workspace, wkspIndex, distribution, withDy=True, withDx=True)
    _setLabels1D(axes, workspace)
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
    if isinstance(workspace,mantid.dataobjects.MDHistoWorkspace):
        (normalization,kwargs)=_getNormalization(workspace, **kwargs)
        (x,y,_)=_getMDData1D(workspace,normalization)
    else:
        (wkspIndex, distribution, kwargs) = _getWkspIndexDistAndLabel(workspace, **kwargs)
        (x, y, _, _) = _getSpectrum(workspace, wkspIndex, distribution)
    _setLabels1D(axes, workspace)
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
    if isinstance(workspace,mantid.dataobjects.MDHistoWorkspace):
        (normalization,kwargs)=_getNormalization(workspace, **kwargs)
        x,y,z=_getMDData2D_bin_centers(workspace,normalization)
    else:
        (distribution, kwargs) = _getDistribution(workspace, **kwargs)
        (x,y,z) = _getMatrix2DData(workspace, distribution,histogram2D=False)
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
    if isinstance(workspace,mantid.dataobjects.MDHistoWorkspace):
        (normalization,kwargs)=_getNormalization(workspace, **kwargs)
        x,y,z=_getMDData2D_bin_centers(workspace,normalization)
    else:
        (distribution, kwargs) = _getDistribution(workspace, **kwargs)
        (x,y,z) = _getMatrix2DData(workspace, distribution,histogram2D=False)
    _setLabels2D(axes, workspace)
    return axes.contourf(x, y, z, *args, **kwargs)

def _pcolorpieces(axes, workspace, distribution, *args,**kwargs):
    (x,y,z)=_getUnevenData(workspace, distribution)
    pcolortype=kwargs.pop('pcolortype','')
    mini=numpy.min([numpy.min(i) for i in z])
    maxi=numpy.max([numpy.max(i) for i in z])
    if 'vmin' in kwargs:
        mini=kwargs['vmin']
    if 'vmax' in kwargs:
        maxi=kwargs['vmax']
    if 'norm' not in kwargs:
        kwargs['norm']=matplotlib.colors.Normalize(vmin=mini, vmax=maxi)
    else:
        if kwargs['norm'].vmin==None:
            kwargs['norm'].vmin=mini
        if kwargs['norm'].vmax==None:
            kwargs['norm'].vmax=maxi    
    for xi,yi,zi in zip(x,y,z):
        XX,YY=numpy.meshgrid(xi,yi,indexing='ij')
        if 'mesh' in pcolortype.lower():
            cm=axes.pcolormesh(XX,YY,zi.reshape(-1,1),**kwargs)
        elif 'fast' in pcolortype.lower():
            cm=axes.pcolorfast(XX,YY,zi.reshape(-1,1),**kwargs)
        else:
            cm=axes.pcolor(XX,YY,zi.reshape(-1,1),**kwargs)
    return cm

    

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
    _setLabels2D(axes, workspace)
    if isinstance(workspace,mantid.dataobjects.MDHistoWorkspace):
        (normalization,kwargs)=_getNormalization(workspace, **kwargs)
        x,y,z=_getMDData2D_bin_bounds(workspace,normalization)
    else:
        (aligned, kwargs)=_getDataUnevenFlag(workspace,**kwargs)
        (distribution, kwargs) = _getDistribution(workspace, **kwargs)
        if aligned:
            kwargs['pcolortype']=''
            return _pcolorpieces(axes, workspace, distribution, *args,**kwargs)
        else:
            (x,y,z) = _getMatrix2DData(workspace, distribution,histogram2D=True)
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
    _setLabels2D(axes, workspace)
    if isinstance(workspace,mantid.dataobjects.MDHistoWorkspace):
        (normalization,kwargs)=_getNormalization(workspace, **kwargs)
        x,y,z=_getMDData2D_bin_bounds(workspace,normalization)
    else:
        (aligned, kwargs)=_getDataUnevenFlag(workspace,**kwargs)
        (distribution, kwargs) = _getDistribution(workspace, **kwargs)
        if aligned:
            kwargs['pcolortype']='fast'
            return _pcolorpieces(axes, workspace, distribution, *args,**kwargs)
        else:
            (x,y,z) = _getMatrix2DData(workspace, distribution,histogram2D=True)
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
    _setLabels2D(axes, workspace)
    if isinstance(workspace,mantid.dataobjects.MDHistoWorkspace):
        (normalization,kwargs)=_getNormalization(workspace, **kwargs)
        x,y,z=_getMDData2D_bin_bounds(workspace,normalization)
    else:
        (aligned, kwargs)=_getDataUnevenFlag(workspace,**kwargs)
        (distribution, kwargs) = _getDistribution(workspace, **kwargs)
        if aligned:
            kwargs['pcolortype']='mesh'
            return _pcolorpieces(axes, workspace, distribution, *args,**kwargs)
        else:
            (x,y,z) = _getMatrix2DData(workspace, distribution,histogram2D=True)

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
    if isinstance(workspace,mantid.dataobjects.MDHistoWorkspace):
        (normalization,kwargs)=_getNormalization(workspace, **kwargs)
        xtemp,ytemp,z=_getMDData2D_bin_centers(workspace,normalization)
        x,y=numpy.meshgrid(xtemp,ytemp)
    else:
        (distribution, kwargs) = _getDistribution(workspace, **kwargs)
        (x,y,z) = _getMatrix2DData(workspace, distribution,histogram2D=False)
    _setLabels2D(axes, workspace)

    return axes.tripcolor(x.ravel(), y.ravel(), z.ravel(), *args, **kwargs)

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
    if isinstance(workspace,mantid.dataobjects.MDHistoWorkspace):
        (normalization,kwargs)=_getNormalization(workspace, **kwargs)
        xtemp,ytemp,z=_getMDData2D_bin_centers(workspace,normalization)
        x,y=numpy.meshgrid(xtemp,ytemp)
    else:
        (distribution, kwargs) = _getDistribution(workspace, **kwargs)
        (x,y,z) = _getMatrix2DData(workspace, distribution,histogram2D=False)
    _setLabels2D(axes, workspace)
    #tricontour segfaults if many z values are not finite
    #https://github.com/matplotlib/matplotlib/issues/10167
    x=x.ravel()
    y=y.ravel()
    z=z.ravel()
    condition=numpy.isfinite(z)
    x=x[condition]
    y=y[condition]
    z=z[condition]
    return axes.tricontour(x, y, z, *args, **kwargs)


def tricontourf(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`mantid.plots.contourf`, but works
    for non-uniform grids. Currently this only works with workspaces
    that have a constant number of bins between spectra or with
    MDHistoWorkspaces.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a matrix workspace histogram.

    See :meth:`matplotlib.axes.Axes.tricontourf` for more information.
    '''
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = _getNormalization(workspace, **kwargs)
        (xtemp, ytemp, z) = _getMDData2D_bin_centers(workspace, normalization)
        (x, y) = numpy.meshgrid(xtemp, ytemp)
    else:
        (distribution, kwargs) = _getDistribution(workspace, **kwargs)
        (x, y, z) = _getMatrix2DData(workspace, distribution,histogram2D=False)
    _setLabels2D(axes, workspace)
    #tricontourf segfaults if many z values are not finite
    #https://github.com/matplotlib/matplotlib/issues/10167
    x = x.ravel()
    y = y.ravel()
    z = z.ravel()
    condition = numpy.isfinite(z)
    x = x[condition]
    y = y[condition]
    z = z[condition]
    return axes.tricontourf(x, y, z, *args, **kwargs)
