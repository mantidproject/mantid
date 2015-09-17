#pylint: disable=no-init,invalid-name
#from mantid.api import AlgorithmFactory
#from mantid.simpleapi import PythonAlgorithm, WorkspaceProperty
# from mantid.kernel import Direction
from mantid.api import *
from mantid.kernel import *
import mantid.simpleapi

def try_int(s):
    "Convert to integer if possible."
    try: return int(s)
    except: return s

def natsort_key(s):
    "Used internally to get a tuple by which s is sorted."
    import re
    return map(try_int, re.findall(r'(\d+|\D+)', s))

def natcmp(a, b):
    "Natural string comparison, case sensitive."
    return cmp(natsort_key(a), natsort_key(b))

def natcasecmp(a, b):
    "Natural string comparison, ignores case."
    return natcmp(a.lower(), b.lower())

def natsort(seq, cmp=natcmp):
    "In-place natural string sort."
    seq.sort(cmp)

def natsorted(seq, cmp=natcmp):
    "Returns a copy of seq, sorted by natural string sort."
    import copy
    temp = copy.copy(seq)
    natsort(temp, cmp)
    return temp

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

        # Let's make sure we have a unique and naturally sorted list
        banks_list = banks.split(",")
        banks = ",".join(natsorted(list(set(banks_list))))
        
        self.getLogger().information('Loading data from banks:' + banks.replace("bank", ""))

        wksp_name = "__tmp"
        ws = mantid.simpleapi.LoadEventNexus(Filename=filename, BankName=banks, OutputWorkspace=wksp_name)

        self.setProperty("OutputWorkspace", ws)
        mantid.simpleapi.DeleteWorkspace(wksp_name)

# Register
AlgorithmFactory.subscribe(LoadVisionInelastic)
