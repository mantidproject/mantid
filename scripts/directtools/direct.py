# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
import mantidplot
import numpy


def intensityInterval(workspace):
    """Return an Interval holding the min and max intensities."""
    # Accept both workspace names and actual workspaces.
    name = str(workspace)
    if name:
        workspace = mtd[name]
    data = workspace.extractY()
    cMin = numpy.nanmin(data)
    cMax = numpy.nanmax(data)
    return (cMin, cMax)


def plotSofQW(workspace, horMin=None, horMax=None, vertMin=None, vertMax=None, cMin=None, cMax=None):
    """Plot a 2D plot with given axis limits and return the plotting layer."""
    # Accept both workspace names and actual workspaces.
    name = str(workspace)
    if name:
        workspace = mtd[name]
    layer = mantidplot.plot2D(workspace).activeLayer()
    if vertMin is not None and vertMax is not None:
        layer.setScale(0, vertMin, vertMax)
    if cMin is not None and cMax is not None:
        layer.setScale(1, cMin, cMax)
    if horMin is not None and horMax is not None:
        layer.setScale(2, horMin, horMax)
    return layer


def trimNaNs(workspace, trimmedWSName = None):
    """Return a workspace with NaNs removed from beginning and end."""
    from mantid.simpleapi import CreateWorkspace
    # Accept both workspace names and actual workspaces.
    name = str(workspace)
    if name:
        workspace = mtd[name]
    if workspace.getNumberHistograms() != 1:
        raise RuntimeError('trinNaNs supports single histogram workspaces only.')
    ys = workspace.readY(0)
    nans = numpy.isnan(ys)
    nonNaNIndices = numpy.nonzero(nans == False)
    begin = nonNaNIndices[0][0]
    end = nonNaNIndices[0][-1] + 1
    trimmedYs = ys[begin:end]
    trimmedEs = workspace.readE(0)[begin:end]
    xs = workspace.readX(0)
    trimmedXs = xs[begin:end] if len(xs) == len(ys) else xs[begin:end+1]
    if trimmedWSName is None:
        trimmedWSName = str(workspace) + '_NaNtrimmed'
    # TODO Use the original vertical axis from workspace when the
    # CreateWorkspace bug fix has been merged to Mantid.
    vertAxis = workspace.getAxis(1).extractValues()
    if len(vertAxis) == 2:
        vertAxis = numpy.array([(vertAxis[0] + vertAxis[1]) / 2.0])
    outWS = CreateWorkspace(DataX=trimmedXs, DataY=trimmedYs, DataE=trimmedEs,
        UnitX=workspace.getAxis(0).getUnit().unitID(),
        VerticalAxisUnit=workspace.getAxis(1).getUnit().unitID(),
        VerticalAxisValues=vertAxis,
        OutputWorkspace=trimmedWSName, ParentWorkspace=workspace)
    return outWS


def validQRange(workspace, energyTransfer=0.0):
    """Return a q range at given energy transfer where S(q,w) is defined."""
    # Accept both workspace names and actual workspaces.
    name = str(workspace)
    if name:
        workspace = mtd[name]
    # Get the vertical (energy) bin edges.
    vertBins = workspace.getAxis(1).extractValues()
    # Find index of zero energy transfer.
    elasticIndex = numpy.argmin(numpy.abs(vertBins))
    # Get the elastic intensities.
    Ys = workspace.readY(elasticIndex)
    # Find indices to valid data.
    validIndices = numpy.argwhere(numpy.isnan(Ys) == False)
    # Get horizontal (q) bin edges.
    Xs = workspace.readX(elasticIndex)
    # Return the range as a tuple.
    lower = Xs[numpy.min(validIndices)]
    upper = Xs[numpy.max(validIndices)]
    return (lower, upper)
