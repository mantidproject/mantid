from mantid.kernel import *
from mantid.api import *
from mantid import simpleapi
# from sansdata import SANSdata


class LoadSANSLegacy(PythonAlgorithm):

    def category(self):
        return "Examples"

    def name(self):
        return "LoadSANSLegacy"

    def summary(self):
        return "Load the SANS Legacy data file to the mantid workspace."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "",
                                          FileAction.Load, ['.001']),
                             "Name of SANS experimental data file.")

        self.declareProperty(WorkspaceProperty("OutputWorkspace",
                                               "", direction=Direction.Output),
                             doc="Name of the workspace to store the experimental data.")

    def PyExec(self):
        # filename = self.getPropertyValue("Filename")
        out_ws_name = self.getPropertyValue("OutputWorkspace")

        simpleapi.CreateWorkspace(OutputWorkspace=out_ws_name, DataX=[1,2,3], DataY=[1,2,3])
        out_ws = simpleapi.AnalysisDataService.retrieve(out_ws_name)
        simpleapi.LoadInstrument(out_ws, InstrumentName='SANS', RewriteSpectraMap=True)
        self.setProperty("OutputWorkspace", out_ws)


# Register algorithm with mantid
AlgorithmFactory.subscribe(LoadSANSLegacy)
