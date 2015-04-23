#pylint: disable=no-init,invalid-name
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *

class PoldiDataAnalysis(PythonAlgorithm):
    """
    This workflow algorithm uses all of the POLDI specific algorithms to perform a complete data analysis,
    starting from the correlation method and preliminary 1D-fits, proceeding with either one or two passses
    of 2D-fitting.

    All resulting workspaces are grouped together at the end so that they are all in one place.
    """

    def category(self):
        return "Workflow"

    def name(self):
        return "PoldiDataAnalysis"

    def summary(self):
        return "Run all necessary steps for a complete analysis of POLDI data."

    def PyInit(self):
        pass

    def PyExec(self):
        pass

AlgorithmFactory.subscribe(PoldiDataAnalysis())