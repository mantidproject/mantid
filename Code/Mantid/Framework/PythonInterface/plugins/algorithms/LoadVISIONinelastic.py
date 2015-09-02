#pylint: disable=no-init,invalid-name
from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import *
import os

class LoadVISIONinelastic(PythonAlgorithm):

    __forward = ""
    __backward = ""

    def category(self):
        return "DataHandling;PythonAlgorithms"

    def name(self):
        return "LoadVISIONinelastic"

    def summary(self):
        return "This algorithm loads only the inelastic detectors on VISION."

    def PyInit(self):
        self.declareProperty("Filename", "")
        self.declareProperty("Banks", "All")

    def PyExec(self):
        filename = self.getProperty("Filename").value
        banks = self.getProperty("Banks").value

