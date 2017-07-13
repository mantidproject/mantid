from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *

class NewMCAbsorption(DataProcessorAlgorithm):

    def category(self):
        return "Workflow\\Inelastic;CorrectionFunctions\\AbsorptionCorrections;Workflow\\MIDAS"

    def summary(self):
        return "Calculates absorption corrections for a given sample shape."


# Register algorithm with Mantid
AlgorithmFactory.subscribe(NewMCAbsorption)