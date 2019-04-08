# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import collections
from directtools import _validate
from mantid import logger, mtd
from mantid.simpleapi import DeleteWorkspace, LineProfile, OneMinusExponentialCor, Transpose
import matplotlib
import matplotlib.colors
from matplotlib import pyplot
import numpy
from scipy import constants
import time


def _applyiftimeseries(logValue, function):
    """Apply function to logValue, if it is time series log, otherwise return logValue as is."""
    if isinstance(logValue, collections.Iterable):
        return function(logValue)
    return logValue


def _bincentres(edges):
    """Return bin centers."""
    return (edges[:-1] + edges[1:]) / 2


def _choosemarker(markers, index):
    """Pick a marker from markers and return next cyclic index."""
    if index >= len(markers):
        index = 0
    marker = markers[index]
    index += 1
    return marker, index


def _clearmath(s):
    """Return string s with special math characters removed."""
    for c in ['%', '_', '$', '&',  '\\', '^', '{', '}',]:
        s = s.replace(c, '')
    s = s.replace(u'\u00c5', 'A')
    return s


def _configurematplotlib(params):
    """Set matplotlib rc parameters from the params dictionary."""
    matplotlib.rcParams.update(params)


def _chooseylabel(workspace, axes):
    """Set the correct y label for profile axes."""
    yUnitLabel = workspace.YUnitLabel()
    if yUnitLabel == "X''(Q,E)":
        axes.set_ylabel(r"$\chi''(Q,E)$")
    elif yUnitLabel == 'g^{neutron}(E) (arb. units)':
        axes.set_ylabel(r'$g(E)$')
    elif yUnitLabel == 'g(E) (states/cm^-1)':
        axes.set_ylabel('$g(E)$ (states/cm$^{-1}$)')
    elif yUnitLabel == 'g(E) (states/meV)':
        axes.set_ylabel('$g(E)$ (states/meV)')
    else:
        axes.set_ylabel('$S(Q,E)$')


def _cutcentreandwidth(line):
    """Return cut centre and width as tuple."""
    axis = line.getAxis(1)
    aMin = axis.getMin()
    aMax = axis.getMax()
    centre = (aMin + aMax) / 2.
    width = (aMax - aMin) / 2.
    return centre, width


def _denormalizeline(line):
    """Multiplies line workspace by the line width."""
    axis = line.getAxis(1)
    height = axis.getMax() - axis.getMin()
    Ys = line.dataY(0)
    Ys *= height
    Es = line.dataE(0)
    Es *= height


def _dostitle(workspaces, axes):
    """Add title to density-of-states axes."""
    workspaces = _normwslist(workspaces)
    if len(workspaces) == 1:
        title = _singledatatitle(workspaces[0])
    else:
        title = _plottingtime()
        logs = workspaces[0].run()
        instrument = _instrumentname(logs)
        if instrument is not None:
            title = instrument + ' ' + title
    axes.set_title(title)


def _energylimits(workspaces):
    """Find suitable xmin and xmax for energy transfer plots."""
    workspaces = _normwslist(workspaces)
    eMax = 0.
    for ws in workspaces:
        axisIndex = 0 if ws.getAxis(0).getUnit().name() == 'Energy transfer' else 1
        Xs = ws.getAxis(axisIndex).extractValues()
        eMax = max(eMax, Xs[-1])
    eMin = -eMax
    logs = workspaces[0].run()
    ei = _incidentenergy(logs)
    T = _applyiftimeseries(_sampletemperature(logs), numpy.mean)
    if ei is not None:
        if T is not None:
            eMin = min(-T / 6., -0.2 * ei)
        else:
            eMin = -0.2 * ei
    return eMin, eMax


def _finalizeprofileE(axes):
    """Set axes for const E axes."""
    if axes.get_xscale() == 'linear':
        axes.set_xlim(xmin=0.)
    axes.set_xlabel(u'$Q$ (\u00c5$^{-1}$)')
    if axes.get_yscale() == 'linear':
        axes.set_ylim(0.)


def _finalizeprofileQ(workspaces, axes):
    """Set axes for const Q axes."""
    workspaces = _normwslist(workspaces)
    eMin, eMax = _energylimits(workspaces)
    axes.set_xlim(xmax=eMax)
    if axes.get_xscale() == 'linear':
        axes.set_xlim(xmin=eMin)
    axes.set_xlabel('Energy (meV)')
    unusedMin, cMax = _globalnanminmax(workspaces)
    if axes.get_yscale() == 'linear':
        axes.set_ylim(ymin=0.)
    axes.set_ylim(ymax=cMax / 100.)


def _globalnanminmax(workspaces):
    """Return global minimum and maximum of the given workspaces."""
    workspaces = _normwslist(workspaces)
    globalMin = numpy.inf
    globalMax = -numpy.inf
    for ws in workspaces:
        cMin, cMax = nanminmax(ws)
        globalMin = min(globalMin, cMin)
        globalMax = max(globalMax, cMax)
    return globalMin, globalMax


def _horizontallineatzero(axes):
    """Add a horizontal line at Y = 0."""
    spine = axes.spines['bottom']
    axes.axhline(linestyle=spine.get_linestyle(), color=spine.get_edgecolor(), linewidth=spine.get_linewidth())


def _incidentenergy(logs):
    """Return the incident energy value from the logs or None."""
    if logs.hasProperty('Ei'):
        return logs.getProperty('Ei').value
    else:
        return None


def _instrumentname(logs):
    """Return the instrument name from the logs or None."""
    if logs.hasProperty('instrument.name'):
        return logs.getProperty('instrument.name').value
    else:
        return None


def _label(ws, cut, width, singleWS, singleCut, singleWidth, quantity, units):
    """Return a line label for a line profile."""
    ws = _normws(ws)
    wsLabel = ''
    if not singleWS:
        wsLabel = _workspacelabel(ws)
    cutLabel = ''
    if not singleCut or not singleWidth:
        cutLabel = quantity + r' = {:0.2f} $\pm$ {:1.2f}'.format(cut, width) + ' ' + units
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


def _plotsinglehistogram(workspaces, labels, style, xscale, yscale):
    """Plot single histogram workspaces."""
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
            markerStyle, markerIndex = _choosemarker(markers, markerIndex)
        axes.errorbar(ws, wkspIndex=0, linestyle=lineStyle, marker=markerStyle, label=label, distribution=True)
    axes.set_xscale(xscale)
    axes.set_yscale(yscale)
    if axes.get_yscale() == 'linear':
        _horizontallineatzero(axes)
    _chooseylabel(workspaces[0], axes)
    return figure, axes


def _plottingtime():
    """Return a string presenting the plotting time."""
    return time.strftime('%d.%m.%Y %H:%M:%S')


def _mantidsubplotsetup():
    """Return the Mantid projection setup."""
    return {'projection': 'mantid'}


def _profiletitle(workspaces, cuts, scan, units, axes):
    """Add title to line profile axes."""
    workspaces = _normwslist(workspaces)
    cuts = _normwslist(cuts)
    if len(workspaces) == 1:
        title = _singledatatitle(workspaces[0])
        centre, width = _cutcentreandwidth(cuts[0])
        if len(cuts) == 1:
            title = title + '\n' + scan + r' = {:0.2f} $\pm$ {:0.2f}'.format(centre, width) + ' ' + units
    else:
        title = _plottingtime()
        logs = workspaces[0].run()
        instrument = _instrumentname(logs)
        if instrument is not None:
            title = instrument + ' ' + title
    axes.set_title(title)


def _removesingularity(ws, epsilon):
    """Find the bin nearest to X = 0, and if -epsilon <= bin centre < epsilon, set Y and E to zero."""
    for i in range(ws.getNumberHistograms()):
        xs = ws.readX(i)
        ys = ws.dataY(i)
        es = ws.dataE(i)
        if len(xs) != len(ys):
            xs = _bincentres(xs)
        binIndex = numpy.argmin(numpy.abs(xs))
        if xs[binIndex] >= -epsilon and xs[binIndex] < epsilon:
            ys[binIndex] = 0.
            es[binIndex] = 0.


def _runnumber(logs):
    """Return run number from the logs or None."""
    if logs.hasProperty('run_number'):
        return logs.getProperty('run_number').value
    else:
        return None


def _sampletemperature(logs):
    """Return the instrument specific sample temperature from the logs or None."""
    instrument = _instrumentname(logs)
    if instrument in ['IN4', 'IN5', 'IN6']:
        return logs.getProperty('sample.temperature').value
    else:
        return None


def _sanitize(s):
    """Return a string with special characters escaped."""
    s = s.replace('$', '\\$')
    return s


def _singledatatitle(workspace):
    """Return title for a single data dataset."""
    workspace = _normws(workspace)
    wsName = _sanitize(str(workspace))
    title = wsName
    logs = workspace.run()
    instrument= _instrumentname(logs)
    if instrument is not None:
        title = title + ' ' + instrument
    run = _runnumber(logs)
    if run is not None:
        title = title + ' #{:06d}'.format(run)
    title = title + '\n' + _plottingtime() + '\n'
    ei = _incidentenergy(logs)
    if ei is not None:
        title = title + r' $E_i$ = {:0.2f} meV'.format(ei)
    T = _sampletemperature(logs)
    if T is not None:
        T = _applyiftimeseries(T, numpy.mean)
        title = title + r' $T$ = {:0.1f} K'.format(T)
    return title


def _SofQWtitle(workspace, axes):
    """Add title to SofQW axes."""
    workspace = _normws(workspace)
    title = _singledatatitle(workspace)
    axes.set_title(title)


def _wavelength(logs):
    """Return the wavelength from the logs or None."""
    if logs.hasProperty('wavelength'):
        return logs.getProperty('wavelength').value
    else:
        return None


def _workspacelabel(workspace):
    """Return workspace information useful for plot labels."""
    workspace = _normws(workspace)
    label = ''
    logs = workspace.run()
    run = _runnumber(logs)
    if run is not None:
        label = label + r'#{:06d}'.format(run)
    ei = _incidentenergy(logs)
    if ei is not None:
        label =  label + r' $E_i$ = {:0.2f} meV'.format(ei)
    T = _sampletemperature(logs)
    if T is not None:
        T = _applyiftimeseries(T, numpy.mean)
        label = label + r' $T$ = {:0.1f} K'.format(T)
    return label


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
        vertAxis = _bincentres(vertAxis)
    horBegin = numpy.argwhere(xs[0, :] >= horMin)[0][0]
    horEnd = numpy.argwhere(xs[0, :] < horMax)[-1][0] + 1
    vertBegin = numpy.argwhere(vertAxis >= vertMin)[0][0]
    vertEnd = numpy.argwhere(vertAxis < vertMax)[-1][0] + 1
    return slice(vertBegin, vertEnd), slice(horBegin, horEnd)


def defaultrcparams():
    """Return a dictionary of directtools default matplotlib rc parameters.

    :returns: a :class:`dict` of default :mod:`matplotlib` rc parameters needed by :mod:`directtools`
    """
    params = {
        'legend.numpoints': 1
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
    if not _validate._isSofQW(workspace):
        raise RuntimeError('Failed to calculate dynamic susceptibility. '
                           + "The workspace '{}' does not look like a S(Q,E).".format(str(workspace)))
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
    outWS.setYUnitLabel("Dynamic susceptibility")
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
        xs = numpy.apply_along_axis(_bincentres, 1, xs)
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
    _validate._styleordie(style)
    workspaces = _normwslist(workspaces)
    eID = 'DeltaE'
    if workspaces[0].getAxis(1).getUnit().unitID() == eID:
        axisIndex = 1
        direction = 'Horizontal'
    else:
        axisIndex = 0
        direction = 'Vertical'
    for ws in workspaces:
        if ws.getAxis(axisIndex).getUnit().unitID() != eID:
            raise RuntimeError("Cannot cut in const E. The workspace '{}' is not in units of energy transfer.".format(str(ws)))
    figure, axes, cutWSList = plotcuts(direction, workspaces, E, dE, r'$E$', 'meV', style, keepCutWorkspaces,
                                       xscale, yscale)
    _profiletitle(workspaces, cutWSList, r'$E$', 'meV', axes)
    if len(cutWSList) > 1:
        axes.legend()
    _finalizeprofileE(axes)
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
    _validate._styleordie(style)
    workspaces = _normwslist(workspaces)
    qID = 'MomentumTransfer'
    if workspaces[0].getAxis(0).getUnit().unitID() == qID:
        axisIndex = 0
        direction = 'Vertical'
    else:
        axisIndex = 1
        direction = 'Horizontal'
    for ws in workspaces:
        if ws.getAxis(axisIndex).getUnit().unitID() != qID:
            raise RuntimeError("Cannot cut in const Q. The workspace '{}' is not in units of momentum transfer.".format(str(ws)))
    figure, axes, cutWSList = plotcuts(direction, workspaces, Q, dQ, r'$Q$', u'\u00c5$^{-1}$', style, keepCutWorkspaces,
                                       xscale, yscale)
    _profiletitle(workspaces, cutWSList, r'$Q$', u'\u00c5$^{-1}$', axes)
    if len(cutWSList) > 1:
        axes.legend()
    _finalizeprofileQ(workspaces, axes)
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
    _validate._styleordie(style)
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
                quantityStr = _clearmath(quantity)
                unitStr = _clearmath(unit)
                wsName = 'cut_{}_{}={}+-{}{}'.format(wsStr, quantityStr, cut, width, unitStr)
                if keepCutWorkspaces:
                    cutWSList.append(wsName)
                line = LineProfile(ws, cut, width, Direction=direction,
                                   OutputWorkspace=wsName, StoreInADS=keepCutWorkspaces, EnableLogging=False)
                if ws.isDistribution() and direction == 'Vertical':
                    _denormalizeline(line)
                if 'm' in style:
                    markerStyle, markerIndex = _choosemarker(markers, markerIndex)
                realCutCentre, realCutWidth = _cutcentreandwidth(line)
                label = _label(ws, realCutCentre, realCutWidth, len(workspaces) == 1, len(cuts) == 1, len(widths) == 1, quantity, unit)
                axes.errorbar(line, wkspIndex=0, linestyle=lineStyle, marker=markerStyle, label=label, distribution=True)
    axes.set_xscale(xscale)
    axes.set_yscale(yscale)
    if axes.get_yscale() == 'linear':
        _horizontallineatzero(axes)
    _chooseylabel(workspaces[0], axes)
    return figure, axes, cutWSList


def plotDOS(workspaces, labels=None, style='l', xscale='linear', yscale='linear'):
    """Plot density of state workspaces.

    Plots the given DOS workspaces.

    :param workspaces: a single workspace or a list thereof
    :type workspaces: str, :class:`mantid.api.MatrixWorkspace` or a :class:`list` thereof
    :param labels: a list of labels for the plot legend
    :type labels: str, a :class:`list` of strings or None
    :param style: plot style: 'l' for lines, 'm' for markers, 'lm' for both
    :type style: str
    :param xscale: horizontal axis scaling: 'linear', 'log', 'symlog', 'logit'
    :type xscale: str
    :param yscale: vertical axis scaling: 'linear', 'log', 'symlog', 'logit'
    :type yscale: str
    :returns: a tuple of (:mod:`matplotlib.Figure`, :mod:`matplotlib.Axes`)
    """
    _validate._styleordie(style)
    workspaces = _normwslist(workspaces)
    for ws in workspaces:
        _validate._singlehistogramordie(ws)
        if not _validate._isDOS(ws):
            logger.warning("The workspace '{}' does not look like proper DOS data. Trying to plot nonetheless.".format(ws))
    if labels is None:
        labels = [_workspacelabel(ws) for ws in workspaces]
    figure, axes = _plotsinglehistogram(workspaces, labels, style, xscale, yscale)
    _dostitle(workspaces, axes)
    if len(workspaces) > 1:
        axes.legend()
    return figure, axes


def plotprofiles(workspaces, labels=None, style='l', xscale='linear', yscale='linear'):
    """Plot line profile workspaces.

    Plots the given single histogram cut workspaces.

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
    _validate._styleordie(style)
    workspaces = _normwslist(workspaces)
    for ws in workspaces:
        _validate._singlehistogramordie(ws)
    figure, axes = _plotsinglehistogram(workspaces, labels, style, xscale, yscale)
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
    workspace = _normws(workspace)
    if not _validate._isSofQW(workspace):
        logger.warning("The workspace '{}' does not look like proper S(Q,W) data. Trying to plot nonetheless.".format(str(workspace)))
    qHorizontal = workspace.getAxis(0).getUnit().name() == 'q'
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
            EMin, unusedEMax = _energylimits(workspace)
    if EMax is None:
        EAxisIndex = 1 if qHorizontal else 0
        EAxis = workspace.getAxis(EAxisIndex).extractValues()
        EMax = EAxis[-1]
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
                raise RuntimeError('Cannot plot nonpositive range in log scale.')
        colorNormalization = matplotlib.colors.LogNorm()
    else:
        raise RuntimeError('Unknown colorscale: ' + colorscale)
    contours = axes.pcolor(workspace, vmin=VMin, vmax=VMax, distribution=True, cmap=colormap, norm=colorNormalization)
    colorbar = figure.colorbar(contours)
    if isSusceptibility:
        colorbar.set_label(r"$\chi''(Q,E)$ (arb. units)")
    else:
        colorbar.set_label(r'$S(Q,E)$ (arb. units)')
    if qHorizontal:
        xLimits = {'left': QMin, 'right': QMax}
        yLimits = {'bottom': EMin}
        if EMax is not None:
            yLimits['top'] = EMax
        xLabel = u'$Q$ (\u00c5$^{-1}$)'
        yLabel = 'Energy (meV)'
    else:
        xLimits = {'left': EMin}
        if EMax is not None:
            xLimits['right'] = EMax
        yLimits = {'bottom': QMin, 'top': QMax}
        xLabel = 'Energy (meV)'
        yLabel = u'$Q$ (\u00c5$^{-1}$)'
    axes.set_xlim(**xLimits)
    axes.set_ylim(**yLimits)
    axes.set_xlabel(xLabel)
    axes.set_ylabel(yLabel)
    _SofQWtitle(workspace, axes)
    return figure, axes


def subplots(**kwargs):
    """Return matplotlib figure and axes with Mantid projection.

    The returned figure and axes have the proper projection to plot Mantid workspaces directly.

    :param kwargs: keyword arguments that are directly passed to :func:`matplotlib.pyplot.subplots`.
    :type kwargs: dict
    :returns: a tuple of (:class:`matplotlib.Figure`, :class:`matplotlib.Axes`)
    """
    figure, axes = pyplot.subplots(subplot_kw=_mantidsubplotsetup(), **kwargs)
    figure.set_tight_layout(True)
    return figure, axes


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
    if workspace.getAxis(0).getUnit().name() == 'q':
        vertPoints = workspace.getAxis(1).extractValues()
        if len(vertPoints) > workspace.getNumberHistograms():
            vertPoints = _bincentres(vertPoints)
        elasticIndex = int(numpy.argmin(numpy.abs(vertPoints - E)))
        ys = workspace.readY(elasticIndex)
        validIndices = numpy.argwhere(numpy.logical_not(numpy.isnan(ys)))
        xs = workspace.readX(elasticIndex)
        lower = xs[numpy.amin(validIndices)]
        upperIndex = numpy.amax(validIndices)
        if len(xs) > len(ys):
            upperIndex = upperIndex + 1
        upper = xs[upperIndex]
        return lower, upper
    else:
        horPoints = workspace.readX(0)
        nPoints = len(workspace.readY(0))
        if len(horPoints) > nPoints:
            horPoints = _bincentres(horPoints)
        elasticIndex = int(numpy.argmin(numpy.abs(horPoints - E)))
        vertPoints = workspace.getAxis(1).extractValues()
        if len(vertPoints) > workspace.getNumberHistograms():
            vertPoints = _bincentres(vertPoints)
        lower = numpy.inf
        upper = -numpy.inf
        for i in range(workspace.getNumberHistograms()):
            y = workspace.readY(i)[elasticIndex]
            if not numpy.isnan(y):
                q = vertPoints[i]
                if q < lower:
                    lower = q
                if q > upper:
                    upper = q
        return lower, upper


def wsreport(workspace):
    """Print some useful information from sample logs.

    The logs are expected to contain some ILL specific fields.

    :param workspace: a workspace from which to extract the logs
    :type workspace: str or :class:`mantid.api.MatrixWorkspace`
    :returns: None
    """
    workspace = _normws(workspace)
    logs = workspace.run()
    print(str(workspace))
    instrument = _instrumentname(logs)
    if instrument is not None:
        print('Instrument: ' + instrument)
    if logs.hasProperty('run_number'):
        print('Run Number: {:06d}'.format(logs.getProperty('run_number').value))
    if logs.hasProperty('start_time'):
        print('Start Time: {}'.format(logs.getProperty('start_time').value.replace('T', ' ')))
    ei = _incidentenergy(logs)
    wavelength = _wavelength(logs)
    if ei is not None and wavelength is not None:
        print('Ei = {:0.2f} meV    lambda = {:0.2f} A'.format(ei, wavelength))
    T = _sampletemperature(logs)
    if T is not None:
        if isinstance(T, collections.Iterable):
            meanT = _applyiftimeseries(T, numpy.mean)
            stdT = _applyiftimeseries(T, numpy.std)
            print('T = {:0.2f} +- {:4.2f} K'.format(meanT, stdT))
            minT = _applyiftimeseries(T, numpy.amin)
            maxT = _applyiftimeseries(T, numpy.amax)
            print('T in [{:0.2f},{:0.2f}]'.format(minT, maxT))
        else:
            print('T = {:0.2f} K'.format(T))
    # Instrument specific additional information
    if instrument == 'IN4':
        rpm = logs.getProperty('FC.rotation_speed').value
        hertz = rpm / 60.
        print('Fermi = {:0.0f} rpm = {:0.1f} Hz'.format(rpm, hertz))
    elif instrument == 'IN6':
        rpm = logs.getProperty('Fermi.rotation_speed').value
        hertz = rpm / 60.
        print('Fermi = {:0.0f} rpm = {:0.1f} Hz'.format(rpm, hertz))


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
_configurematplotlib(defaultrcparams())
