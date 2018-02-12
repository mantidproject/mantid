from __future__ import (absolute_import, division, print_function)

import collections
from mantid import mtd
from mantid.simpleapi import DeleteWorkspace, LineProfile
import matplotlib
from matplotlib import pyplot
import numpy
import time


def _globalnanminmax(workspaces):
    """Return a suitable maximum intensity for y limits."""
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


def _normws(workspace):
    """Return a reference to workspace."""
    name = str(workspace)
    if name:
        workspace = mtd[name]
    return workspace


def _normwslist(workspaces):
    """Return a list of references to workspaces."""
    ws = list()
    if not isinstance(workspaces, collections.Iterable) or isinstance(workspaces, str):
        workspaces = [workspaces]
    unnormWss = workspaces
    return [_normws(ws) for ws in unnormWss]


def _profilelegend(workspaces, scan, units, cuts, widths, axes):
    """Set line labels and add a legend to line profile axes."""
    workspaces = _normwslist(workspaces)
    if not isinstance(cuts, collections.Iterable):
        cuts = [cuts]
    if not isinstance(widths, collections.Iterable):
        widths = [widths] * len(cuts)
    wsLabels = list()
    if len(workspaces) > 1:
        for ws in workspaces:
            logs = SampleLogs(ws)
            T = numpy.mean(logs.sample.temperature)
            wsLabels.append('\\#{:06d} T = {:0.1f} K Ei = {:0.2f} meV'.format(logs.run_number, T, logs.Ei))
    cutLabels = list()
    if len(cuts) > 1:
        for cut, width in zip(cuts, widths):
            cutLabels.append(scan + ' = {:0.2f} +- {:1.2f}'.format(cut, width) + units)
    labels = list()
    if len(workspaces) == 1:
        labels = cutLabels
    elif len(cuts) == 1:
        labels = wsLabels
    else:
        for ws in wsLabels:
            for cut in cutLabels:
                labels.append(ws + ' ' + cut)
    lines = axes.get_lines()
    for line, label in zip(lines, labels):
        line.set_label(label)
    axes.legend()


def _profiletitle(workspaces, scan, units, cuts, widths, figure):
    """Add title to line profile figure."""
    workspaces = _normwslist(workspaces)
    if not isinstance(cuts, collections.Iterable):
        cuts = [cuts]
    if not isinstance(widths, collections.Iterable):
        widths = [widths] * len(cuts)
    ws = workspaces[0]
    logs = SampleLogs(ws)
    if len(cuts) == 1:
        title = (logs.instrument.name + ' ' + time.strftime('%x') + ' ' + time.strftime('%X') + '\n'
                 + scan + ' = {:0.2f} +- {:0.2f}'.format(cuts[0], widths[0]) + ' ' + units)
    elif len(workspaces) == 1:
        T = numpy.mean(logs.sample.temperature)
        title = (str(ws) + ' ' + logs.instrument.name + ' \\#{:06d}'.format(logs.run_number) + '\n'
                 + time.strftime('%x') + ' ' + time.strftime('%X') + '\n'
                 + 'T = {:0.1f} K Ei = {:0.2f} meV'.format(T, logs.Ei))
    else:
        title = logs.instrument.name + ' ' + time.strftime('%x') + ' ' + time.strftime('%X')
    figure.suptitle(title)


def _profileytitle(axes):
    """Set the correct y label for profile axes."""
    axes.set_ylabel('$S(Q,E)$')


def _points(edges):
    """Convert bin edges to points."""
    return (edges[:-1] + edges[1:]) / 2
    

def box2D(xs, vertAxis, horMin=-numpy.inf, horMax=numpy.inf, vertMin=-numpy.inf, vertMax=numpy.inf):
    """Return slicing for a numpy array limited by given min and max values."""
    if len(vertAxis) > xs.shape[0]:
        vertAxis = _points(vertAxis)
    horBegin = numpy.argwhere(xs[0, :] >= horMin)[0][0]
    horEnd = numpy.argwhere(xs[0, :] < horMax)[-1][0] + 1
    vertBegin = numpy.argwhere(vertAxis >= vertMin)[0][0]
    vertEnd = numpy.argwhere(vertAxis < vertMax)[-1][0] + 1
    return slice(vertBegin, vertEnd), slice(horBegin, horEnd)


def mantid_subplot_setup():
    """Return a dict for the matplotlib.pyplot.subplots()."""
    return {'projection': 'mantid'}


def nanminmax(workspace, horMin=-numpy.inf, horMax=numpy.inf, vertMin=-numpy.inf, vertMax=numpy.inf):
    """Return global min and max intensities of workspaces."""
    workspace = _normws(workspace)
    xs = workspace.extractX()
    ys = workspace.extractY()
    if xs.shape[1] > ys.shape[1]:
        xs = _points(xs)
    vertAxis = workspace.getAxis(1).extractValues()
    box = box2D(xs, vertAxis, horMin, horMax, vertMin, vertMax)
    ys = ys[box]
    cMin = numpy.nanmin(ys)
    cMax = numpy.nanmax(ys)
    return cMin, cMax


def plotprofiles(direction, workspaces, cuts, widths):
    """Plot line profile from given workspaces and cuts."""
    workspaces = _normwslist(workspaces)
    if not isinstance(cuts, collections.Iterable):
        cuts = [cuts]
    if not isinstance(widths, collections.Iterable):
        widths = [widths] * len(cuts)
    figure, axes = subplots()
    for ws in workspaces:
        for cut, width in zip(cuts, widths):
            # TODO: Use StoreInADS=False after issue #21731 has been fixed.
            wsName = '__line_profile'
            line = LineProfile(ws, cut, width, Direction=direction,
                               OutputWorkspace=wsName, EnableLogging=False)
            axes.errorbar(line, specNum=0)
            DeleteWorkspace(line)
    _profileytitle(axes)
    return figure, axes


def plotconstE(workspaces, E, dE):
    """Plot line profiles at constant energy."""
    figure, axes = plotprofiles('Horizontal', workspaces, E, dE)
    _profiletitle(workspaces, 'E', 'meV', E, dE, figure)
    _profilelegend(workspaces, 'E', 'meV', E, dE, axes)
    cMin, cMax = _globalnanminmax(workspaces)
    axes.set_ylim(ymin=0., ymax=cMax / 100.)
    xMin, xMax = axes.get_xlim()
    print('Auto Q-range: {}...{} \xc5-1'.format(xMin, xMax))
    return figure, axes


def plotconstQ(workspaces, Q, dQ):
    """Plot line profiles at constant momentum transfer."""
    figure, axes = plotprofiles('Vertical', workspaces, Q, dQ)
    _profiletitle(workspaces, 'Q', '\\AA', Q, dQ, figure)
    _profilelegend(workspaces, 'Q', '\\AA', Q, dQ, axes)
    axes.set_xlim(xmin=0.)
    xMin, xMax = axes.get_xlim()
    print('Auto E-range: {}...{} meV'.format(xMin, xMax))
    return figure, axes


def plotSofQW(workspace, horMin=-numpy.inf, horMax=numpy.inf, vertMin=-numpy.inf, vertMax=numpy.inf, cMin=-numpy.inf, cMax=numpy.inf):
    """Plot a 2D plot with given axis limits and return the plotting layer."""
    # Accept both workspace names and actual workspaces.
    workspace = _normws(workspace)
    figure, axes = subplots()
    cMin = cMin if cMin != -numpy.inf else None
    cMax = cMax if cMax != numpy.inf else None
    contours = axes.pcolor(workspace, vmin = cMin, vmax = cMax)
    figure.colorbar(contours)
    if horMin != -numpy.inf:
        axes.set_xlim(left=horMin)
    if horMax != numpy.inf:
        axes.set_xlim(right=horMax)
    if vertMin != -numpy.inf:
        axes.set_ylim(bottom=vertMin)
    if vertMax != numpy.inf:
        axes.set_ylim(top=vertMax)
    return figure, axes


def wsreport(workspace):
    """Print relevant information from sample logs."""
    workspace = _normws(workspace)
    print(str(workspace))
    logs = SampleLogs(workspace)
    print('Instrument: ' + logs.instrument.name)
    print('Run Number: {:06d}'.format(logs.run_number))
    print('Start Time: {}'.format(logs.start_time))
    print('Ei = {:0.2f} meV    lambda = {:0.2f} \xc5'.format(logs.Ei, logs.wavelength))
    meanT = numpy.mean(logs.sample.temperature)
    stdT = numpy.std(logs.sample.temperature)
    print('T = {:0.2f} +- {:4.2f} K'.format(meanT, stdT))
    minT = numpy.amin(logs.sample.temperature)
    maxT = numpy.amax(logs.sample.temperature)
    print('T in [{:0.2f},{:0.2f}]'.format(minT, maxT))
    fermiHertz = logs.FC.rotation_speed / 60.
    print('Fermi = {:0.0f} rpm = {:0.1f} Hz'.format(logs.FC.rotation_speed, fermiHertz))


def subplots(**kwargs):
    """Return matplotlib figure and axes."""
    return pyplot.subplots(subplot_kw=mantid_subplot_setup(), **kwargs)


def validQ(workspace, energyTransfer=0.0):
    """Return a q range at given energy transfer where S(q,w) is defined."""
    workspace = _normws(workspace)
    vertBins = workspace.getAxis(1).extractValues()
    if len(vertBins) > workspace.getNumberHistograms:
        vertBins = _points(vertBins)
    elasticIndex = numpy.argmin(numpy.abs(vertBins - energyTransfer))
    ys = workspace.readY(elasticIndex)
    validIndices = numpy.argwhere(numpy.logical_not(numpy.isnan(ys)))
    xs = workspace.readX(elasticIndex)
    lower = xs[numpy.amin(validIndices)]
    upperIndex = numpy.amax(validIndices)
    if len(xs) > len(ys):
        upperIndex = upperIndex + 1
    upper = xs[upperIndex]
    return lower, upper


class SampleLogs:
    def __init__(self, workspace):
        """Transform sample log entries from workspace into attributes of this object."""
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


matplotlib.rc('text', usetex=True)
