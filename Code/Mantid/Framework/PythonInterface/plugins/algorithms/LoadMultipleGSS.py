#pylint: disable=no-init,invalid-name
from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import *
import os

class LoadMultipleGSS(PythonAlgorithm):
    def category(self):
        return "DataHandling;PythonAlgorithms"

    def name(self):
        return "LoadMultipleGSS"

    def summary(self):
        return "This algorithm loads multiple gsas files from a single directory into mantid."

    def __load(self, directory, prefix):
        for ext in self.__exts:
            filename = "%s%s" % (prefix, ext)
            if len(directory) > 0:
                filename = os.path.join(directory, filename)
                if not os.path.exists(filename):
                    continue
            try:
                self.log().information("Trying to load '%s'" % filename)
                self.__loader(Filename=filename, OutputWorkspace=prefix, UseBankIDasSpectrumNumber=True)
                return
            except Exception, e:
                pass
        raise RuntimeError("Failed to load run %s" % prefix)

    def PyInit(self):
        self.declareProperty("FilePrefix","")
        intArrayValidator = IntArrayBoundedValidator()
        intArrayValidator.setLower(0)
        self.declareProperty(IntArrayProperty("RunNumbers",[0], validator=intArrayValidator))
        self.declareProperty(FileProperty("Directory", "", action=FileAction.OptionalDirectory))

    def PyExec(self):
        # generic stuff for running
        prefix = self.getProperty("FilePrefix").value
        runs = self.getProperty("RunNumbers").value
        directory = self.getProperty("Directory").value.strip()

        # change here if you want something other than gsas files
        self.__exts = ['.txt', '.gsa']
        self.__loader = LoadGSS

        # load things and conjoin them
        first = True
        for run in runs:
            wksp = "%s_%d" % (prefix,run)
            self.__load(directory, wksp)
            ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="dSpacing")

AlgorithmFactory.subscribe(LoadMultipleGSS)
