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


def _getWkspIndexDistAndLabel(workspace, kwargs):
    # get the special arguments out of kwargs
    specNum = kwargs.get('specNum', None)
    if 'specNum' in kwargs:
        del kwargs['specNum']
    wkspIndex = kwargs.get('wkspIndex', None)
    if 'wkspIndex' in kwargs:
        del kwargs['wkspIndex']

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

    (distribution, kwargs) = _getDistribution(workspace, kwargs)

    return (wkspIndex, distribution, kwargs)


def _getDistribution(workspace, kwargs):
    '''Determine whether or not the data is a distribution. The value in
    the kwargs wins.'''
    distribution = kwargs.get('distribution', None)
    if 'distribution' in kwargs:
        del kwargs['distribution']

    # fix up the distribution flag
    if workspace.isHistogramData():
        if distribution is None:
            distribution = workspace.isDistribution()
        else:
            distribution = bool(distribution)

    return (distribution, kwargs)


def getAxesLabels(workspace):
    '''Return a tuple of ``(YUnit, <other units>)``'''
    axes = [workspace.YUnit()]
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
    # TODO should make extracting dx optional
    if withDx and workspace.getSpectrum(wkspIndex).hasDx():
        dy = workspace.readDx(wkspIndex)

    if workspace.isHistogramData():
        if not distribution:
            y = y / (x[1:] - x[0:-1])
            if dy is not None:
                dy = dy / (x[1:] - x[0:-1])
        x = .5*(x[0:-1]+x[1:])

    return (x,y,dy,dx)


def plot(axes, workspace, *args, **kwargs):
    '''
    Unpack mantid workspace and render it with matplotlib. ``args`` and
    ``kwargs`` are passed to :py:meth:`matplotlib.axes.Axes.plot` after special
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
    (distribution, kwargs) = _getDistribution(workspace, kwargs)
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
    (distribution, kwargs) = _getDistribution(workspace, kwargs)
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
    (distribution, kwargs) = _getDistribution(workspace, kwargs)
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
    (distribution, kwargs) = _getDistribution(workspace, kwargs)
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
    (distribution, kwargs) = _getDistribution(workspace, kwargs)
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
    (distribution, kwargs) = _getDistribution(workspace, kwargs)
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
    (distribution, kwargs) = _getDistribution(workspace, kwargs)
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
    (distribution, kwargs) = _getDistribution(workspace, kwargs)
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
    (distribution, kwargs) = _getDistribution(workspace, kwargs)
    (x,y,z) = _getContour(workspace, distribution)
    _setLabels2D(axes, workspace)

    return axes.tricontourf(x.ravel(), y.ravel(), z.ravel(), *args, **kwargs)
