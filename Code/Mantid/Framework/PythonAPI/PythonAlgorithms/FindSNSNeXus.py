from MantidFramework import *
from mantidsimple import *

class FindSNSNeXus(PythonAlgorithm):
    def category(self):
        return "ICat"

    def name(self):
        return "FindSNSNeXus"

    def PyInit(self):
        try:
            from findnexus_lib import sns_inst_util
            instruments = []
            banned = ["DAS"]
            for instr in sns_inst_util.INSTRUMENTS:
                if instr.facility == "SNS" and not str(instr) in banned:
                    instruments.append(str(instr))
            self.declareProperty("Instrument", "",
                                 Validator=ListValidator(instruments))
        except ImportError, e:
            self.declareProperty("Instrument", "")
        self.declareProperty("RunNumber", 0)
        extensions = [".nxs", "_event.nxs", "_neutron_event.dat",
                      "_neutron0_event.dat", "_neutron1_event.dat",
                      "_cvinfo.xml", "_runinfo.xml"]
        self.declareProperty("Extension", "",
                             Validator=ListValidator(extensions))
        self.declareProperty("UseWebService", True)
        self.declareProperty("ResultPath", "", Direction=Direction.Output)

    def findNeXus(self, runnumber):
        # find the nexus file
        result = self.finder.findNeXus(runnumber)
        if len(result) <= 0:
            raise RuntimeError("Failed to find \"" + self.nxsfile + "\"")
        result = result[0]
        if result.endswith(self.realfile):
            return result

        # if looking for something else try that next
        import os
        result = os.path.join(os.path.split(result)[0], self.realfile)
        if not os.path.exists(result):
            raise RuntimeError("File \"" + result + "\" does not exist")
        return result

    def findPreNeXus(self, runnumber):
        # find the prenexus directory
        result = self.finder.findPreNeXus(runnumber)
        if len(result) <= 0:
            raise RuntimeError("Failed to find \"" + self.realfile + "\"")
        result = result[0]

        # go to the actual filename
        import os
        result = os.path.join(result, self.realfile)
        if not os.path.exists(result):
            raise RuntimeError("File \"" + result + "\" does not exist")
        return result

    def PyExec(self):
        try:
            from findnexus_lib import Finder, ArchiveInfo
        except ImportError:
            raise RuntimeError("'findnexus_lib' is not installed")

        # get the properties so the nexus file will be found
        instrument = self.getProperty("Instrument").upper()
        runnumber = self.getProperty("RunNumber")
        extension = self.getProperty("Extension")
        usews = self.getProperty("Use WebService")

        self.nxsfile = instrument + "_" + str(runnumber) + ".nxs"
        self.realfile = instrument + "_" + str(runnumber) + extension

        info = ArchiveInfo(instrument=instrument)
        self.finder = Finder(info, usews=usews)
        if extension.endswith(".nxs"):
            result = self.findNeXus(runnumber)
        else:
            result = self.findPreNeXus(runnumber)
        self.log().information(result)
        self.setProperty("ResultPath", result)

mtd.registerPyAlgorithm(FindSNSNeXus())
