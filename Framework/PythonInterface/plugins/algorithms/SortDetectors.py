# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import Direction,IntArrayProperty, FloatArrayProperty
import  mantid
import math
import numpy


class SortDetectors(PythonAlgorithm):
    """ Sort detectors by distance
    """

    def category(self):
        """ Return category
        """
        return "Utility\\Sorting"

    def seeAlso(self):
        return [ "SortByQVectors","SortXAxis" ]

    def name(self):
        """ Return name
        """
        return "SortDetectors"

    def summary(self):
        """ Return summary
        """
        return "Algorithm to sort detectors by distance."

    def PyInit(self):
        """ Declare properties
        """
        self.declareProperty(mantid.api.WorkspaceProperty(  "Workspace", "",
                                                            direction=mantid.kernel.Direction.Input,
                                                            validator=mantid.api.InstrumentValidator()),
                             "Input workspace")

        self.declareProperty(IntArrayProperty("UpstreamSpectra", Direction.Output))
        self.declareProperty(FloatArrayProperty("UpstreamDetectorDistances", Direction.Output))
        self.declareProperty(IntArrayProperty("DownstreamSpectra", Direction.Output))
        self.declareProperty(FloatArrayProperty("DownstreamDetectorDistances", Direction.Output))

    def PyExec(self):
        """ Main execution body
        """
        workspace = self.getProperty("Workspace").value
        samplePos = workspace.getInstrument().getSample().getPos()
        moderatorPos = workspace.getInstrument().getSource().getPos()
        incident = samplePos - moderatorPos

        upstream=[]
        upinds=[]
        updist=[]
        downstream=[]
        downinds=[]
        downdist=[]
        for i in range(workspace.getNumberHistograms()):
            detPos=workspace.getDetector(i).getPos()
            scattered=detPos-samplePos
            if abs(scattered.angle(incident))>0.999*math.pi:
                upstream.append((i,scattered.norm()))
            else:
                downstream.append((i,scattered.norm()))

        if len(upstream)>0:
            upstream.sort(key=lambda x: x[1])
            zipped_upstream = list(zip(*upstream))
            upinds=zipped_upstream[0]
            updist=zipped_upstream[1]
        if len(downstream)>0:
            downstream.sort(key=lambda x: x[1])
            zipped_downstream = list(zip(*downstream))
            downinds=zipped_downstream[0]
            downdist=zipped_downstream[1]

        self.setProperty("UpstreamSpectra", numpy.array(upinds))
        self.setProperty("UpstreamDetectorDistances", numpy.array(updist))
        self.setProperty("DownstreamSpectra", numpy.array(downinds))
        self.setProperty("DownstreamDetectorDistances", numpy.array(downdist))


AlgorithmFactory.subscribe(SortDetectors)
