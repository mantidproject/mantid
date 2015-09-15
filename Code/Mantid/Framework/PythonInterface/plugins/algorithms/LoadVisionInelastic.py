#pylint: disable=no-init,invalid-name
#from mantid.api import AlgorithmFactory
#from mantid.simpleapi import PythonAlgorithm, WorkspaceProperty
# from mantid.kernel import Direction
from mantid.api import *
from mantid.kernel import *
import mantid.simpleapi


class LoadVisionInelastic(PythonAlgorithm):

    __forward = "bank1,bank2,bank3,bank4,bank5,bank6,bank7"
    __backward = "bank8,bank9,bank10,bank11,bank12,bank13,bank14"

    def category(self):
        return "DataHandling;PythonAlgorithms"

    def name(self):
        return "LoadVisionInelastic"

    def summary(self):
        return "This algorithm loads only the inelastic detectors on VISION."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "", action=FileAction.Load, extensions=["*.nxs.h5"]))
        self.declareProperty("Banks", "all")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output))

    def PyExec(self):
        filename = self.getProperty("Filename").value
        banks = self.getProperty("Banks").value

        # First lets replace 'All' with 'forward,backward'
        banks = banks.lower().replace("all", "forward,backward")
        banks = banks.lower().replace("forward", self.__forward)
        banks = banks.lower().replace("backward", self.__backward)
        
        # Let's make sure we have a unique list
        banks_list = banks.split(",")
        banks = ",".join(set(banks_list))

        self.getLogger().information('Loading data from banks:' + banks.replace("bank", ""))

        wksp_name = "__tmp"
        ws = mantid.simpleapi.LoadEventNexus(Filename=filename, BankName=banks, OutputWorkspace=wksp_name)

        self.setProperty("OutputWorkspace", ws)
        mantid.simpleapi.DeleteWorkspace(wksp_name)

# Register
AlgorithmFactory.subscribe(LoadVisionInelastic)
