#pylint: disable=no-init,invalid-name
#from mantid.api import AlgorithmFactory
#from mantid.simpleapi import PythonAlgorithm, WorkspaceProperty
# from mantid.kernel import Direction
from mantid.api import *
from mantid.kernel import *
import mantid.simpleapi


class LoadVisionElasticBackScattering(PythonAlgorithm):

    __backscattering = ""
    #__equitorial = ""

    def category(self):
        return "DataHandling;PythonAlgorithms"

    def name(self):
        return "LoadVisionElasticBackScattering"

    def summary(self):
        return "This algorithm loads only the backscattering elastic detectors on VISION."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "", action=FileAction.Load, extensions=["*.nxs.h5"]))
        self.declareProperty("Banks", "all")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output))

    def PyExec(self):
        filename = self.getProperty("Filename").value
        banks = self.getProperty("Banks").value

        banks = banks.lower().replace("all", "backscattering")
        banks = banks.lower().replace("backscattering", self.__backscattering)
        #banks = banks.lower().replace("equitorial", self.__equitorial)

        # Let's make sure we have a unique list
        banks_list = banks.split(",")
        banks = ",".join(set(banks_list))

        self.getLogger().information('Loading data from banks:' + banks.replace("bank", ""))

        wksp_name = "__tmp"

        # TODO: How do we work out if the file has histrogram based or not.
        try:
            ws = mantid.simpleapi.LoadEventNexus(Filename=filename, BankName=banks, OutputWorkspace=wksp_name)
        except:
            # TODO: load histrogram


        self.setProperty("OutputWorkspace", ws)
        mantid.simpleapi.DeleteWorkspace(wksp_name)

# Register
AlgorithmFactory.subscribe(LoadVisionElasticBackScattering)
