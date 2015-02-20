#pylint: disable=no-init,invalid-name
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import Direction,IntArrayProperty, FloatArrayProperty
import  mantid,math,numpy


class SortDetectors(PythonAlgorithm):
    """ Sort detectors by distance
    """
    def category(self):
        """ Return category
        """
        return "PythonAlgorithms;Utility"

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
        self.declareProperty(mantid.api.WorkspaceProperty("Workspace", "",
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
            upinds=zip(*upstream)[0]
            updist=zip(*upstream)[1]
        if len(downstream)>0:
            downstream.sort(key=lambda x: x[1])
            downinds=zip(*downstream)[0]
            downdist=zip(*downstream)[1]

        self.setProperty("UpstreamSpectra", numpy.array(upinds))
        self.setProperty("UpstreamDetectorDistances", numpy.array(updist))
        self.setProperty("DownstreamSpectra", numpy.array(downinds))
        self.setProperty("DownstreamDetectorDistances", numpy.array(downdist))

AlgorithmFactory.subscribe(SortDetectors)
