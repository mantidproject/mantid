#pylint: disable=no-init,invalid-name
from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *
import os

class ConjoinFiles(PythonAlgorithm):
    def category(self):
        return "DataHandling;PythonAlgorithms"

    def name(self):
        return "ConjoinFiles"

    def summary(self):
        return "Conjoin two file-based workspaces."

    def __load(self, directory, instr, run, loader, exts, wksp):
        filename = None
        for ext in exts:
            filename = "%s_%s%s" % (instr, str(run), ext)
            if len(directory) > 0:
                filename = os.path.join(directory, filename)
                if not os.path.exists(filename):
                    continue
            try:
                self.log().information("Trying to load '%s'" % filename)
                loader(Filename=filename, OutputWorkspace=wksp)
                return
            except Exception, e:
                logger.information(str(e))
        raise RuntimeError("Failed to load run %s from file %s" % (str(run), filename))

    def PyInit(self):
        greaterThanZero = IntArrayBoundedValidator()
        greaterThanZero.setLower(0)
        self.declareProperty(IntArrayProperty("RunNumbers",values=[0],
                             validator=greaterThanZero), doc="Run numbers")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output))
        self.declareProperty(FileProperty("Directory", "", FileAction.OptionalDirectory))

    def PyExec(self):
        # generic stuff for running
        wksp = self.getPropertyValue("OutputWorkspace")
        runs = self.getProperty("RunNumbers")
        instr = config.getInstrument().shortName()
        directory = self.getPropertyValue("Directory").strip()

        # change here if you want something other than gsas files
        exts = ['.txt', '.gsa']
        loader = LoadGSS

        # load things and conjoin them
        first = True
        for run in runs.value:
            run = str(run)
            if first:
                self.__load(directory, instr, run, loader, exts, wksp)
                first = False
            else:
                self.__load(directory, instr, run, loader, exts, run)
                ConjoinWorkspaces(InputWorkspace1=wksp, InputWorkspace2=run, CheckOverlapping=False)
                if mtd.doesExist(run):
                    DeleteWorkspace(run)

        self.setProperty("OutputWorkspace", mtd[wksp])

AlgorithmFactory.subscribe(ConjoinFiles)

