def plot(axes, workspace, *args, **kwargs): #specNum=None, wkspIndex=None, distribution=None, *args, **kwargs):
    '''Unpack mantid workspace and render it with matplotlib. ``args`` and
    ``kwargs`` are passed to :class:matplotlib.axes.Axes after special
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
    # get the special arguments out of kwargs
    specNum = kwargs.get('specNum', None)
    if 'specNum' in kwargs:
        del kwargs['specNum']
    wkspIndex = kwargs.get('wkspIndex', None)
    if 'wkspIndex' in kwargs:
        del kwargs['wkspIndex']
    distribution = kwargs.get('distribution', None)
    if 'distribution' in kwargs:
        del kwargs['distribution']

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

    if 'label' not in kwargs:
        kwargs['label'] = 'spec {0}'.format(specNum)

    y = workspace.readY(wkspIndex)
    if workspace.isHistogramData():
        x = .5*(workspace.readX(wkspIndex)[0:-1]+workspace.readX(wkspIndex)[1:])
        if distribution is None:
            distribution = workspace.isDistribution()
        else:
            distribution = bool(distribution)

        if not distribution:
            y = y / (workspace.readX(wkspIndex)[1:] - workspace.readX(wkspIndex)[0:-1])
    else:
        x = workspace.readX(wkspIndex)

    axes.plot(x, y, *args, **kwargs)
