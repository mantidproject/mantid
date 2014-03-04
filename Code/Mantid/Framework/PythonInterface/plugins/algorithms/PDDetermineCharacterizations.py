"""*WIKI*

*WIKI*"""

import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *

class PDDetermineCharacterizations(PythonAlgorithm):
    def category(self):
        return "Workflow/Diffraction"

    def name(self):
        return "PDDetermineCharacterizations"

    def PyInit(self):
        pass

    def PyExec(self):
        pass

# Register algorthm with Mantid.
AlgorithmFactory.subscribe(PDDetermineCharacterizations)
