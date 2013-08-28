"""*WIKI* 


*WIKI*"""

from MantidFramework import *
from mantidsimple import *
import os

class LoadMultipleGSS(PythonAlgorithm):
    def category(self):
        return "DataHandling;PythonAlgorithms"

    def name(self):
        return "LoadMultipleGSS"

    def __load(self, directory, instr, run, loader, exts, wksp):
        for ext in exts:
            filename = "%s_%s%s" % (instr, str(run), ext)
            if len(directory) > 0:
                filename = os.path.join(directory, filename)
                if not os.path.exists(filename):
                    continue
            try:
                self.log().information("Trying to load '%s'" % filename)
                loader(filename, wksp)
                return
            except Exception, e:
                pass
        raise RuntimeError("Failed to load run %s" % str(run))              

    def PyInit(self):
        self.declareListProperty("RunNumbers",[0], Validator=ArrayBoundedValidator(Lower=0))
        self.declareFileProperty("Directory", "", FileAction.OptionalDirectory)

    def PyExec(self):
        # generic stuff for running
        #wksp = self.getPropertyValue("OutputWorkspace")
        runs = self.getProperty("RunNumbers")
        instr = mtd.getSettings().facility().instrument().shortName()
        directory = self.getPropertyValue("Directory").strip()

        # change here if you want something other than gsas files
        exts = ['.txt', '.gsa']
        loader = LoadGSS

        # load things and conjoin them
        first = True
        for run in runs:
            run = str(run)
            self.__load(directory, instr, run, loader, exts, run)
	    ConvertUnits(InputWorkspace=run, OutputWorkspace=run, Target="dSpacing")

mtd.registerPyAlgorithm(LoadMultipleGSS())
