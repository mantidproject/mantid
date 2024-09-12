# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import *
import os


class LoadMultipleGSS(PythonAlgorithm):
    __exts = None
    __loader = None

    def category(self):
        return "DataHandling\\Text"

    def seeAlso(self):
        return ["LoadGSS"]

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
            except (RuntimeError, ValueError, IOError):
                pass
        raise RuntimeError("Failed to load run %s" % prefix)

    def PyInit(self):
        self.declareProperty("FilePrefix", "")
        intArrayValidator = IntArrayBoundedValidator(lower=0)
        self.declareProperty(IntArrayProperty("RunNumbers", [0], validator=intArrayValidator))
        self.declareProperty(FileProperty("Directory", "", action=FileAction.OptionalDirectory))

    def PyExec(self):
        # generic stuff for running
        prefix = self.getProperty("FilePrefix").value
        runs = self.getProperty("RunNumbers").value
        directory = self.getProperty("Directory").value.strip()

        # change here if you want something other than gsas files
        self.__exts = [".txt", ".gsa"]
        self.__loader = LoadGSS

        # load things and conjoin them
        for run in runs:
            wksp = "%s_%d" % (prefix, run)
            self.__load(directory, wksp)
            ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="dSpacing")


AlgorithmFactory.subscribe(LoadMultipleGSS)
