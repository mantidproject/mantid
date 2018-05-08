from __future__ import (absolute_import, division, print_function)

import collections
from mantid import mtd
from mantid.simpleapi import DeleteWorkspace, LineProfile, OneMinusExponentialCor, Transpose
import matplotlib
import matplotlib.colors
from matplotlib import pyplot
import numpy
from scipy import constants
import time


def _applyIfTimeSeries(logValue, function):
    """Apply function to logValue, if it is time series log, otherwise return logValue as is."""
    if isinstance(logValue, collections.Iterable):
        return function(logValue)
    return logValue


def _chooseMarker(markers, index):
    """Pick a marker from markers and return next cyclic index."""
    if index >= len(markers):
        index = 0
    marker = markers[index]
    index += 1
    return marker, index


def _clearlatex(s):
    """Return string s with special LaTeX characters removed."""
    for c in ['%', '_', '$', '&', '\\', '^', '{', '}',]:
        s = s.replace(c, '')
    return s


def _configurematplotlib(params):
    """Set matplotlib rc parameters from the params dictionary."""
    matplotlib.rcParams.update(params)


def _finalizeprofileE(axes):
    """Set axes for const E axes."""
    axes.set_xlim(xmin=0.)
    axes.set_xlabel('$Q$ (\\AA$^{-1}$)')
    axes.set_ylim(0.)
    xMin, xMax = axes.get_xlim()
    print('Auto Q-range: {}...{} \xc5-1'.format(xMin, xMax))


def _finalizeprofileQ(workspaces, axes):
    """Set axes for const Q axes."""
    workspaces = _normwslist(workspaces)
    axes.set_xlim(xmin=-10.)
    axes.set_xlabel('Energy (meV)')
    cMax = 0.
    for ws in workspaces:
        c = numpy.nanmax(ws.readY(0))
        if c > cMax:
            cMax = c
    axes.set_ylim(ymin=0., ymax=cMax / 100.)
    xMin, xMax = axes.get_xlim()
    print('Auto E-range: {}...{} meV'.format(xMin, xMax))


def _globalnanminmax(workspaces):
    """Return global minimum and maximum of the given workspaces."""
    workspaces = _normwslist(workspaces)
    globalMin = numpy.inf
    globalMax = -numpy.inf
    for ws in workspaces:
        cMin, cMax = nanminmax(ws)
        if cMin < globalMin:
            globalMin = cMin
        if cMax > globalMax:
            globalMax = cMax
    return globalMin, globalMax


def _label(ws, cut, width, singleWS, singleCut, singleWidth, quantity, units):
    """Return a line label for a line profile."""
    ws = _normws(ws)
    wsLabel = ''
    if not singleWS:
        logs = SampleLogs(ws)
        T = _applyIfTimeSeries(logs.sample.temperature, numpy.mean)
        wsLabel = '\\#{:06d} $T$ = {:0.1f} K $E_i$ = {:0.2f} meV'.format(logs.run_number, T, logs.Ei)
    cutLabel = ''
    if not singleCut or not singleWidth:
        cutLabel = quantity + ' = {:0.2f} $\pm$ {:1.2f}'.format(cut, width) + ' ' + units
    return wsLabel + ' ' + cutLabel


def _normws(workspace):
    """Retrieve workspace from mtd if it is a string, otherwise return as-is."""
    name = str(workspace)
    if name:
        workspace = mtd[name]
    return workspace


def _normwslist(workspaces):
    """Retrieve workspaces from mtd if they are string, otherwise return as-is."""
    if not isinstance(workspaces, collections.Iterable) or isinstance(workspaces, str):
        workspaces = [workspaces]
    unnormWss = workspaces
    return [_normws(ws) for ws in unnormWss]


def _plottingtime():
    """Return a string presenting the plotting time."""
    return time.strftime('%d.%m.%Y %H:%M:%S')


def _binCentres(edges):
    """Return bin centers."""
    return (edges[:-1] + edges[1:]) / 2


def _mantidsubplotsetup():
    """Return the Mantid projection setup."""
    return {'projection': 'mantid'}


def _profiletitle(workspaces, scan, units, cuts, widths, figure):
    """Add title to line profile figure."""
    workspaces = _normwslist(workspaces)
    if not isinstance(cuts, collections.Iterable):
        cuts = [cuts]
    if not isinstance(widths, collections.Iterable):
        widths = [widths] * len(cuts)
    if len(workspaces) == 1:
        title = _singledatatitle(workspaces[0])
    else:
        logs = SampleLogs(workspaces[0])
        title = logs.instrument.name + ' ' + _plottingtime()
    if len(cuts) == 1 and len(widths) == 1:
        title = title + '\n' + scan + ' = {:0.2f} $\pm$ {:0.2f}'.format(cuts[0], widths[0]) + ' ' + units
    figure.suptitle(title)


def _profileytitle(workspace, axes):
    """Set the correct y label for profile axes."""
    if workspace.YUnit() == 'Dynamic susceptibility':
        axes.set_ylabel("$\\chi''(Q,E)$")
    else:
        axes.set_ylabel('$S(Q,E)$')


def _removesingularity(ws, epsilon):
    """Find the bin nearest to X = 0, and if -epsilon <= bin centre < epsilon, set Y and E to zero."""
    for i in range(ws.getNumberHistograms()):
        xs = ws.readX(i)
        ys = ws.dataY(i)
        es = ws.dataE(i)
        if len(xs) != len(ys):
            xs = _binCentres(xs)
        binIndex = numpy.argmin(numpy.abs(xs))
        if xs[binIndex] >= -epsilon and xs[binIndex] < epsilon:
            ys[binIndex] = 0.
            es[binIndex] = 0.


def _sanitizeforlatex(s):
    """Return a string with LaTeX special characters escaped."""
    s = s.replace('_', '\\_')
    s = s.replace('#', '\\#')
    s = s.replace('@', '\\@')
    s = s.replace('&', '\\@')
    s = s.replace('$', '\\$')
    return s


def _singledatatitle(workspace):
    """Return title for a single data dataset."""
    workspace = _normws(workspace)
    logs = SampleLogs(workspace)
    T = _applyIfTimeSeries(logs.sample.temperature, numpy.mean)
    wsName = _sanitizeforlatex(str(workspace))
    title = (wsName + ' ' + logs.instrument.name + ' \\#{:06d}'.format(logs.run_number) + '\n'
             + _plottingtime() + '\n'
             + '$T$ = {:0.1f} K $E_i$ = {:0.2f} meV'.format(T, logs.Ei))
    return title


def _SofQWtitle(workspace, figure):
    """Add title to SofQW figure."""
    workspace = _normws(workspace)
    title = _singledatatitle(workspace)
    figure.suptitle(title)


def box2D(xs, vertAxis, horMin=-numpy.inf, horMax=numpy.inf, vertMin=-numpy.inf, vertMax=numpy.inf):
    """Return slicing for a 2D numpy array limited by given min and max values.

    :param xs: the 2D X data of a workspace from :func:`mantid.api.MatrixWorkspace.extractX`
    :type xs: a 2D :class:`numpy.ndarray`
    :param vertAxis: the vertical axis values of a workspace
    :type vertAxis: a 1D :class:`numpy.ndarray`
    :param horMin: the left edge of the box
    :type horMin: float
    :param horMax: the right edge of the box
    :type horMax: float
    :param vertMin: the bottom edge of the box
    :type vertMin: float
    :param vertMax: the top edge of the box
    :type vertMax: float
    :returns: a tuple of two :class:`slice` objects, the first one for vertical dimension, the second for horizontal.
    """
    if len(vertAxis) > xs.shape[0]:
        vertAxis = _binCentres(vertAxis)
    horBegin = numpy.argwhere(xs[0, :] >= horMin)[0][0]
    horEnd = numpy.argwhere(xs[0, :] < horMax)[-1][0] + 1
    vertBegin = numpy.argwhere(vertAxis >= vertMin)[0][0]
    vertEnd = numpy.argwhere(vertAxis < vertMax)[-1][0] + 1
    return slice(vertBegin, vertEnd), slice(horBegin, horEnd)


def defaultrcParams():
    """Return a dictionary of directtools default matplotlib rc parameters.

    :returns: a :class:`dict` of default :mod:`matplotlib` rc parameters needed by :mod:`directtools`
    """
    params = {
        'legend.numpoints': 1,
        'text.usetex': True,
    }
    return params


def dynamicsusceptibility(workspace, temperature, outputName=None, zeroEnergyEpsilon=1e-6):
    """Convert :math:`S(Q,E)` to susceptibility :math:`\chi''(Q,E)`.

    #. If the X units are not in DeltaE, the workspace is transposed
    #. The Y data in *workspace* is multiplied by :math:`1 - e^{\Delta E / (kT)}`
    #. Y data in the bin closest to 0 meV and within -*zeroEnergyEpsilon* < :math:`\Delta E` < *zeroEnergyEpsilon* is set to 0
    #. If the input was transposed, transpose the output as well

    :param workspace: a :math:`S(Q,E)` workspace to convert
    :type workspace: :class:`mantid.api.MatrixWorkspace`
    :param temperature: temperature in Kelvin
    :type temperature: float
    :param outputName: name of the output workspace. If :class:`None`, the output will be given some generated name.
    :type outputName: str or None
    :param zeroEnergyEpsilon: if a bin center is within this value from 0, the bin's value is set to zero.
    :type zeroEnergyEpsilon: float
    :returns: a :class:`mantid.api.MatrixWorkspace` containing :math:`\chi''(Q,E)`
    """
    workspace = _normws(workspace)
    horAxis = workspace.getAxis(0)
    horUnit = horAxis.getUnit().unitID()
    doTranspose = horUnit != 'DeltaE'
    if outputName is None:
        outputName = 'CHIofQW_{}'.format(str(workspace))
    if doTranspose:
        workspace = Transpose(workspace, OutputWorkspace='__transposed_SofQW_', EnableLogging=False)
    c = 1e-3 * constants.e / constants.k / temperature
    outWS = OneMinusExponentialCor(workspace, OutputWorkspace=outputName, C=c, Operation='Multiply', EnableLogging=False)
    _removesingularity(outWS, zeroEnergyEpsilon)
    if doTranspose:
        outWS = Transpose(outWS, OutputWorkspace=outputName, EnableLogging=False)
        DeleteWorkspace('__transposed_SofQW_', EnableLogging=False)
    outWS.setYUnit("Dynamic susceptibility")
    return outWS


def nanminmax(workspace, horMin=-numpy.inf, horMax=numpy.inf, vertMin=-numpy.inf, vertMax=numpy.inf):
    """Return min and max intensities of a workspace ignoring NaNs.

    The search region can be limited by *horMin*, *horMax*, *vertMin* and *vertMax*.

    :param workspace: a workspace
    :type workspace: :class:`mantid.api.MatrixWorkspace`
    :param horMin: the left edge of the search region
    :type horMin: float
    :param horMax: the right edge of the search region
    :type horMax: float
    :param vertMin: the bottom edge of the search region
    :type vertMin: float
    :param vertMax: the top edge of the search region
    :type vertMax: float

    :returns: a tuple containing the minimum and maximum
    """
    workspace = _normws(workspace)
    xs = workspace.extractX()
    ys = workspace.extractY()
    if xs.shape[1] > ys.shape[1]:
        xs = _binCentres(xs)
    vertAxis = workspace.getAxis(1).extractValues()
    box = box2D(xs, vertAxis, horMin, horMax, vertMin, vertMax)
    ys = ys[box]
    cMin = numpy.nanmin(ys)
    cMax = numpy.nanmax(ys)
    return cMin, cMax


def plotconstE(workspaces, E, dE, style='l', keepCutWorkspaces=True, xscale='linear', yscale='linear'):
    """Plot line profiles at constant energy transfer from :math:`S(Q,E)` workspace.

    Creates cut workspaces using :ref:`algm-LineProfile`, then plots the cuts. A list of workspaces,
    constant energy transfers, or cut widths, or any combination thereof can be given as parameters.

    The last entry in the returned tuple is a list of cut workspace names. This will be an empty list
    is *keeCutWorkspaces* is set to `False` as the workspaces will not appear in the ADS.

    :param workspaces: a single :math:`S(Q,E)` workspace or list of workspaces to cut
    :type workspaces: str, :class:`mantid.api.MatrixWorkspace` or a list thereof
    :param E: a constant energy transfer or a :class:`list` thereof
    :type E: float or :class:`list` of floats
    :param dE: width of the cut or a list of widths
    :type dE: float or :class:`list` of floats
    :param style: plot style: 'l' for lines, 'm' for markers, 'lm' for both
    :type style: str
    :param keepCutWorkspaces: whether or not keep the cut workspaces in the ADS
    :type keepCutWorkspaces: bool
    :param xscale: horizontal axis scaling: 'linear', 'log', 'symlog', 'logit'
    :type xscale: str
    :param yscale: vertical axis scaling: 'linear', 'log', 'symlog', 'logit'
    :type yscale: str
    :returns: A tuple of (:class:`matplotlib.Figure`, :class:`matplotlib.Axes`, a :class:`list` of names)
    """
    figure, axes, cutWSList = plotcuts('Horizontal', workspaces, E, dE, '$E$', 'meV', style, keepCutWorkspaces,
                                       xscale, yscale)
    _profiletitle(workspaces, '$E$', 'meV', E, dE, figure)
    axes.legend()
    axes.set_xlim(xmin=0.)
    axes.set_xlabel('$Q$ (\\AA$^{-1}$)')
    axes.set_ylim(0.)
    xMin, xMax = axes.get_xlim()
    print('Auto Q-range: {}...{} \xc5-1'.format(xMin, xMax))
    return figure, axes, cutWSList


def plotconstQ(workspaces, Q, dQ, style='l', keepCutWorkspaces=True, xscale='linear', yscale='linear'):
    """Plot line profiles at constant momentum transfer from :math:`S(Q,E)` workspace.

    Creates cut workspaces using :ref:`algm-LineProfile`, then plots the cuts. A list of workspaces,
    constant momentum transfers, or cut widths, or any combination thereof can be given as parameters.

    The last entry in the returned tuple is a list of cut workspace names. This will be an empty list
    is *keeCutWorkspaces* is set to `False` as the workspaces will not appear in the ADS.

    :param workspaces: a single :math:`S(Q,E)` workspace or list of workspaces to cut
    :type workspaces: str, :class:`mantid.api.MatrixWorkspace` or a list thereof
    :param Q: a constant momentum transfer or a :class:`list` thereof
    :type Q: float or :class:`list` of floats
    :param dQ: width of the cut or a list of widths
    :type dQ: float or :class:`list` of floats
    :param style: plot style: 'l' for lines, 'm' for markers, 'lm' for both
    :type style: str
    :param keepCutWorkspaces: whether or not keep the cut workspaces in the ADS
    :type keepCutWorkspaces: bool
    :param xscale: horizontal axis scaling: 'linear', 'log', 'symlog', 'logit'
    :type xscale: str
    :param yscale: vertical axis scaling: 'linear', 'log', 'symlog', 'logit'
    :type yscale: str
    :returns: A tuple of (:class:`matplotlib.Figure`, :class:`matplotlib.Axes`, a :class:`list` of names)
    """
    figure, axes, cutWSList = plotcuts('Vertical', workspaces, Q, dQ, '$Q$', '\\AA$^{-1}$', style, keepCutWorkspaces,
                                       xscale, yscale)
    _profiletitle(workspaces, '$Q$', '\\AA$^{-1}$', Q, dQ, figure)
    axes.legend()
    axes.set_xlim(xmin=-10.)
    axes.set_xlabel('Energy (meV)')
    cMin, cMax = _globalnanminmax(workspaces)
    axes.set_ylim(ymin=0., ymax=cMax / 100.)
    xMin, xMax = axes.get_xlim()
    print('Auto E-range: {}...{} meV'.format(xMin, xMax))
    return figure, axes, cutWSList


def plotcuts(direction, workspaces, cuts, widths, quantity, unit, style='l', keepCutWorkspaces=True, xscale='linear', yscale='linear'):
    """Cut and plot multiple line profiles.

    Creates cut workspaces using :ref:`algm-LineProfile`, then plots the cuts. A list of workspaces,
    cut centres, or cut widths, or any combination thereof can be given as parameters.

    The last entry in the returned tuple is a list of cut workspace names. This will be an empty list
    is *keeCutWorkspaces* is set to `False` as the workspaces will not appear in the ADS.

    :param direction: Cut direction. Only ``'Horizontal'`` and ``'Vertical'`` are accepted
    :type direction: str
    :param workspaces: a single workspace or a list thereof
    :type workspaces: str, :class:`mantid.api.MatrixWorkspace` or a :class:`list` thereof
    :param cuts: the center of the cut or a list of centers
    :type cuts: float or a :class:`list` thereof
    :param widths: the width of the cut or a list of widths
    :type widths: float or a :class:`list` thereof
    :param quantity: name of the physical quantity along which the cut is made, used for legend label
    :type quantity: str
    :param unit: unit of *quantity*
    :type unit: str
    :param style: plot style: 'l' for lines, 'm' for markers, 'lm' for both
    :type style: str
    :param keepCutWorkspaces: whether or not keep the cut workspaces in the ADS
    :type keepCutWorkspaces: bool
    :param xscale: horizontal axis scaling: 'linear', 'log', 'symlog', 'logit'
    :type xscale: str
    :param yscale: vertical axis scaling: 'linear', 'log', 'symlog', 'logit'
    :type yscale: str
    :returns: A tuple of (:class:`matplotlib.Figure`, :class:`matplotlib.Axes`, a :class:`list` of names)
    """
    workspaces = _normwslist(workspaces)
    if not isinstance(cuts, collections.Iterable):
        cuts = [cuts]
    if not isinstance(widths, collections.Iterable):
        widths = [widths]
    lineStyle = 'solid' if 'l' in style else 'None'
    figure, axes = subplots()
    markers = matplotlib.markers.MarkerStyle.filled_markers
    markerIndex = 0
    markerStyle = 'None'
    wsCount = 0
    cutWSList = list()
    for ws in workspaces:
        wsCount += 1
        for cut in cuts:
            for width in widths:
                wsStr = str(ws)
                if wsStr == '':
                    wsStr = str(wsCount)
                quantityStr = _clearlatex(quantity)
                unitStr = _clearlatex(unit)
                wsName = 'cut_{}_{}={}+-{}{}'.format(wsStr, quantityStr, cut, width, unitStr)
                if keepCutWorkspaces:
                    cutWSList.append(wsName)
                line = LineProfile(ws, cut, width, Direction=direction,
                                   OutputWorkspace=wsName, StoreInADS=keepCutWorkspaces, EnableLogging=False)
                if 'm' in style:
                    markerStyle, markerIndex = _chooseMarker(markers, markerIndex)
                label = _label(ws, cut, width, len(workspaces) == 1, len(cuts) == 1, len(widths) == 1, quantity, unit)
                axes.errorbar(line, specNum=0, linestyle=lineStyle, marker=markerStyle, label=label, distribution=True)
    axes.set_xscale(xscale)
    axes.set_yscale(yscale)
    _profileytitle(workspaces[0], axes)
    return figure, axes, cutWSList


def plotprofiles(workspaces, labels=None, style='l', xscale='linear', yscale='linear'):
    """Plot line profile workspaces.

    Plots the first histograms from given workspaces.

    :param workspaces: a single workspace or a list thereof
    :type workspaces: str, :class:`mantid.api.MatrixWorkspace` or a :class:`list` thereof
    :param labels: a list of cut labels for the plot legend
    :type labels: str, a :class:`list` of strings or None
    :param style: plot style: 'l' for lines, 'm' for markers, 'lm' for both
    :type style: str
    :param xscale: horizontal axis scaling: 'linear', 'log', 'symlog', 'logit'
    :type xscale: str
    :param yscale: vertical axis scaling: 'linear', 'log', 'symlog', 'logit'
    :type yscale: str
    :returns: a tuple of (:mod:`matplotlib.Figure`, :mod:`matplotlib.Axes`)
    """
    workspaces = _normwslist(workspaces)
    if not isinstance(labels, collections.Iterable) or isinstance(labels, str):
        if labels is None:
            labels = len(workspaces) * ['']
        else:
            labels = [labels]
    if len(workspaces) != len(labels):
        raise ValueError('workspaces and labels list lengths do not match.')
    lineStyle = 'solid' if 'l' in style else 'None'
    figure, axes = subplots()
    markers = matplotlib.markers.MarkerStyle.filled_markers
    markerStyle = 'None'
    markerIndex = 0
    for ws, label in zip(workspaces, labels):
        if 'm' in style:
            markerStyle, markerIndex = _chooseMarker(markers, markerIndex)
        axes.errorbar(ws, specNum=0, linestyle=lineStyle, marker=markerStyle, label=label, distribution=True)
    axes.set_xscale(xscale)
    axes.set_yscale(yscale)
    _profileytitle(workspaces[0], axes)
    xUnit = workspaces[0].getAxis(0).getUnit().unitID()
    if xUnit == 'DeltaE':
        _finalizeprofileQ(workspaces, axes)
    elif xUnit == 'MomentumTransfer':
        _finalizeprofileE(axes)
    return figure, axes


def plotSofQW(workspace, QMin=0., QMax=None, EMin=None, EMax=None, VMin=0., VMax=None, colormap='jet', colorscale='linear'):
    """Plot a 2D :math:`S(Q,E)` workspace.

    :param workspace: a workspace to plot
    :type workspace: str or :class:`mantid.api.MatrixWorkspace`
    :param QMin: minimum :math:`Q` to include in the plot
    :type QMin: float or None
    :param QMax: maximum :math:`Q` to include in the plot
    :type QMax: float or None
    :param EMin: minimum energy transfer to include in the plot
    :type EMin: float or None
    :param EMax: maximum energy transfer to include in the plot
    :type EMax: float or None
    :param VMin: minimum intensity to show on the color bar
    :type VMin: float or None
    :param VMax: maximum intensity to show on the color bar
    :type VMax: float or None
    :param colormap: name of the colormap
    :type colormap: str
    :param colorscale: color map scaling: 'linear', 'log'
    :type colorscale: str
    :returns: a tuple of (:mod:`matplotlib.Figure`, :mod:`matplotlib.Axes`)
    """
    # Accept both workspace names and actual workspaces.
    workspace = _normws(workspace)
    isSusceptibility = workspace.YUnit() == 'Dynamic susceptibility'
    figure, axes = subplots()
    if QMin is None:
        QMin = 0.
    if QMax is None:
        dummy, QMax = validQ(workspace)
    if EMin is None:
        if isSusceptibility:
            EMin = 0.
        else:
            EMin = -10.
    if EMax is None:
        EAxis = workspace.getAxis(1).extractValues()
        EMax = numpy.amax(EAxis)
    if VMax is None:
        vertMax = EMax if EMax is not None else numpy.inf
        dummy, VMax = nanminmax(workspace, horMin=QMin, horMax=QMax, vertMin=EMin, vertMax=vertMax)
        VMax /= 100.
    if VMin is None:
        VMin = 0.
    colorNormalization = None
    if colorscale == 'linear':
        colorNormalization = matplotlib.colors.Normalize()
    elif colorscale == 'log':
        if VMin <= 0.:
            if VMax > 0.:
                VMin = VMax / 1000.
            else:
                raise RuntimeError('Cannot plot negative range in log scale.')
        colorNormalization = matplotlib.colors.LogNorm()
    else:
        raise RuntimeError('Unknown colorscale: ' + colorscale)
    print('Plotting intensity range: {}...{}'.format(VMin, VMax))
    contours = axes.pcolor(workspace, vmin=VMin, vmax=VMax, distribution=True, cmap=colormap, norm=colorNormalization)
    colorbar = figure.colorbar(contours)
    if isSusceptibility:
        colorbar.set_label("$\\chi''(Q,E)$ (arb. units)")
    else:
        colorbar.set_label('$S(Q,E)$ (arb. units)')
    axes.set_xlim(left=QMin)
    axes.set_xlim(right=QMax)
    axes.set_ylim(bottom=EMin)
    if EMax is not None:
        axes.set_ylim(top=EMax)
    axes.set_xlabel('$Q$ (\\AA$^{-1}$)')
    axes.set_ylabel('Energy (meV)')
    _SofQWtitle(workspace, figure)
    return figure, axes


def subplots(**kwargs):
    """Return matplotlib figure and axes with Mantid projection.

    The returned figure and axes have the proper projection to plot Mantid workspaces directly.

    :param kwargs: keyword arguments that are directly passed to :func:`matplotlib.pyplot.subplots`.
    :type kwargs: dict
    :returns: a tuple of (:class:`matplotlib.Figure`, :class:`matplotlib.Axes`)
    """
    return pyplot.subplots(subplot_kw=_mantidsubplotsetup(), **kwargs)


def validQ(workspace, E=0.0):
    """Return a :math:`Q` range at given energy transfer where :math:`S(Q,E)` is defined.

    :math:`S(Q,E)` is undefined when Y = NaN

    :param workspace: A :math:`S(Q,E)` workspace to investigate
    :type workspace: str or :class:`mantid.api.MatrixWorkspace`
    :param E: energy transfer at which to evaluate the range
    :type E: float
    :returns: a tuple of (:math:`Q_{min}`, :math:`Q_{max}`)
    """
    workspace = _normws(workspace)
    vertBins = workspace.getAxis(1).extractValues()
    if len(vertBins) > workspace.getNumberHistograms():
        vertBins = _binCentres(vertBins)
    elasticIndex = numpy.argmin(numpy.abs(vertBins - E))
    ys = workspace.readY(int(elasticIndex))
    validIndices = numpy.argwhere(numpy.logical_not(numpy.isnan(ys)))
    xs = workspace.readX(int(elasticIndex))
    lower = xs[numpy.amin(validIndices)]
    upperIndex = numpy.amax(validIndices)
    if len(xs) > len(ys):
        upperIndex = upperIndex + 1
    upper = xs[upperIndex]
    return lower, upper


def wsreport(workspace):
    """Print some useful information from sample logs.

    The logs are expected to contain some ILL specific fields.

    :param workspace: a workspace from which to extract the logs
    :type workspace: str or :class:`mantid.api.MatrixWorkspace`
    :returns: None
    """
    workspace = _normws(workspace)
    print(str(workspace))
    logs = SampleLogs(workspace)
    print('Instrument: ' + logs.instrument.name)
    print('Run Number: {:06d}'.format(logs.run_number))
    print('Start Time: {}'.format(logs.start_time))
    print('Ei = {:0.2f} meV    lambda = {:0.2f} \xc5'.format(logs.Ei, logs.wavelength))
    meanT = _applyIfTimeSeries(logs.sample.temperature, numpy.mean)
    stdT = _applyIfTimeSeries(logs.sample.temperature, numpy.std)
    print('T = {:0.2f} +- {:4.2f} K'.format(meanT, stdT))
    minT = _applyIfTimeSeries(logs.sample.temperature, numpy.amin)
    maxT = _applyIfTimeSeries(logs.sample.temperature, numpy.amax)
    print('T in [{:0.2f},{:0.2f}]'.format(minT, maxT))
    fermiHertz = logs.FC.rotation_speed / 60.
    print('Fermi = {:0.0f} rpm = {:0.1f} Hz'.format(logs.FC.rotation_speed, fermiHertz))


class SampleLogs:
    """A convenience class to access the sample logs of :class:`mantid.api.MatrixWorkspace`.

    Upon initialization, this class adds the sample logs as data attributes to itself. The
    attributes get their names from the logs. Log names containing dots result in nested
    log objects. Thus, if a workspace contains logs ``'a'`` and ``'b.c'``:

    .. code::

        logs = SampleLogs(workspace)
        # This is equivalent of calling workspace.run().getProperty('a').value
        logs.a
        # This is equivalent of calling workspace.run().getProperty('b.c').value
        logs.b.c
    """
    def __init__(self, workspace):
        """Initialize a `SampleLogs` object.
        Transform sample log entries from workspace into attributes of this object.

        :param workspace: the workspace from which to extract the sample logs
        :type workspace: :class:`mantid.api.MatrixWorkspace`
        """
        class Log:
            pass
        workspace = _normws(workspace)
        properties = workspace.run().getProperties()
        for p in properties:
            name = p.name
            components = name.split('.')
            obj = self
            while len(components) > 1:
                c = components.pop(0)
                if not hasattr(obj, c):
                    subdir = Log()
                    setattr(obj, c, subdir)
                    obj = subdir
                else:
                    obj = getattr(obj, c)
            setattr(obj, components[0], p.value)


# Set default matplotlib rc parameters.
_configurematplotlib(defaultrcParams())
