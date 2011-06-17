from MantidFramework import *
from mantidsimple import *

class ConjoinFiles(PythonAlgorithm):
    def category(self):
        return "DataHandling"

    def name(self):
        return "ConjoinFiles"

    def __load(self, instr, run, loader, exts, wksp):
        for ext in exts:
            filename = "%s_%s%s" % (instr, str(run), ext)
            try:
                loader(filename, wksp)
                return
            except:
                pass                

    def PyInit(self):
        self.declareListProperty("RunNumbers",[0], Validator=ArrayBoundedValidator(Lower=0))
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction=Direction.Output)

    def PyExec(self):
        # generic stuff for running
        wksp = self.getPropertyValue("OutputWorkspace")
        runs = self.getProperty("RunNumbers")
        instr = mtd.getSettings().facility().instrument().shortName()

        # change here if you want something other than gsas files
        exts = ['.gsa', '.txt']
        loader = LoadGSS

        # load things and conjoin them
        first = True
        for run in runs:
            run = str(run)
            if first:
                self.__load(instr, run, loader, exts, wksp)
                first = False
            else:
                self.__load(instr, run, loader, exts, run)
                ConjoinWorkspaces(wksp, run, CheckOverlapping=False)
                mtd.deleteWorkspace(run)

        self.setProperty("OutputWorkspace", mtd[wksp])

mtd.registerPyAlgorithm(ConjoinFiles())