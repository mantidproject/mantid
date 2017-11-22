import numpy

def _getWkspIndexDistAndLabel(workspace, kwargs):
    # get the special arguments out of kwargs
    specNum = kwargs.get('specNum', None)
    if 'specNum' in kwargs:
        del kwargs['specNum']
    wkspIndex = kwargs.get('wkspIndex', None)
    if 'wkspIndex' in kwargs:
        del kwargs['wkspIndex']

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

def plot(axes, workspace, *args, **kwargs):
    '''Unpack mantid workspace and render it with matplotlib. ``args`` and
    ``kwargs`` are passed to :meth:matplotlib.axes.Axes.plot after special
    keyword arguments are removed. This will automatically label the
    line according to the spectrum number unless specified otherwise.

    @param workspace :: :class:`mantid.api.MatrixWorkspace` to extract the data from
    @param specNum :: spectrum number to plot
    @param wkspIndex :: workspace index to plot
    @param axes :: :class:`matplotlib.axes.Axes` object that will do the plotting
    @param distribution :: None (default) asks the workspace. False
    means divide by bin width. True means do not divide by bin width.
    Applies only when the the workspace is a histogram.

    Either ``specNum`` or ``wkspIndex`` needs to be specified. Giving
    both will generate a :class:`exceptions.RuntimeError`.
    '''
    (wkspIndex, distribution, kwargs) = _getWkspIndexDistAndLabel(workspace, kwargs)

    x = workspace.readX(wkspIndex)
    y = workspace.readY(wkspIndex)
    if workspace.isHistogramData():
        if not distribution:
            y = y / (x[1:] - x[0:-1])

        x = .5*(x[0:-1]+x[1:])

    axes.plot(x, y, *args, **kwargs)

def errorbar(axes, workspace, *args, **kwargs):
    '''Unpack mantid workspace and render it with matplotlib. ``args`` and
    ``kwargs`` are passed to :meth:matplotlib.axes.Axes.errorbar after special
    keyword arguments are removed. This will automatically label the
    line according to the spectrum number unless specified otherwise.

    @param workspace :: :class:`mantid.api.MatrixWorkspace` to extract the data from
    @param specNum :: spectrum number to plot
    @param wkspIndex :: workspace index to plot
    @param axes :: :class:`matplotlib.axes.Axes` object that will do the plotting
    @param distribution :: None (default) asks the workspace. False
    means divide by bin width. True means do not divide by bin width.
    Applies only when the the workspace is a histogram.

    Either ``specNum`` or ``wkspIndex`` needs to be specified. Giving
    both will generate a :class:`exceptions.RuntimeError`.
    '''
    (wkspIndex, distribution, kwargs) = _getWkspIndexDistAndLabel(workspace, kwargs)

    x = workspace.readX(wkspIndex)
    y = workspace.readY(wkspIndex)
    # TODO should extract dx but EventWorkspace does the wrong thing
    dy = workspace.readE(wkspIndex)
    if workspace.isHistogramData():
        if not distribution:
            y = y / (x[1:] - x[0:-1])
            dy = dy / (x[1:] - x[0:-1])
        x = .5*(x[0:-1]+x[1:])

    axes.errorbar(x, y, dy, *args, **kwargs)


def contour(axes, workspace, *args, **kwargs):
    '''Essentially the same as :meth:`matplotlib.axes.Axes.contour`
    but calculates the countour levels. Currently this only works with
    workspaces that have a constant number of bins between spectra.

    @param workspace :: :class:`mantid.api.MatrixWorkspace` to extract the data from
    @param axes :: :class:`matplotlib.axes.Axes` object that will do the plotting
    @param distribution :: None (default) asks the workspace. False
    means divide by bin width. True means do not divide by bin width.
    Applies only when the the workspace is a histogram.
    '''
    (distribution, kwargs) = _getDistribution(workspace, kwargs)

    x = workspace.extractX()
    z = workspace.extractY()

    if workspace.isHistogramData():
        if not distribution:
            z = z / (x[:,1:] - x[:,0:-1])
        x = .5*(x[:,0:-1]+x[:,1:])

    # y axis is spectrum number
    y = numpy.zeros(workspace.getNumberHistograms(), dtype=float)
    for index in range(workspace.getNumberHistograms()):
        specNum = workspace.getSpectrum(index).getSpectrumNo()
        y[index] = float(specNum)

    y = numpy.tile(y, (x.shape[1], 1)).transpose()

    axes.contour(x, y, z, *args, **kwargs)

def contourf(axes, workspace, *args, **kwargs):
    '''Essentially the same as :meth:`matplotlib.axes.Axes.contourf`
    but calculates the countour levels. Currently this only works with
    workspaces that have a constant number of bins between spectra.

    @param workspace :: :class:`mantid.api.MatrixWorkspace` to extract the data from
    @param axes :: :class:`matplotlib.axes.Axes` object that will do the plotting
    @param distribution :: None (default) asks the workspace. False
    means divide by bin width. True means do not divide by bin width.
    Applies only when the the workspace is a histogram.
    '''
    (distribution, kwargs) = _getDistribution(workspace, kwargs)

    x = workspace.extractX()
    z = workspace.extractY()

    if workspace.isHistogramData():
        if not distribution:
            z = z / (x[:,1:] - x[:,0:-1])
        x = .5*(x[:,0:-1]+x[:,1:])

    # y axis is spectrum number
    y = numpy.zeros(workspace.getNumberHistograms(), dtype=float)
    for index in range(workspace.getNumberHistograms()):
        specNum = workspace.getSpectrum(index).getSpectrumNo()
        y[index] = float(specNum)

    y = numpy.tile(y, (x.shape[1], 1)).transpose()

    axes.contourf(x, y, z, *args, **kwargs)
