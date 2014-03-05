"""*WIKI*

*WIKI*"""

import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *

# this should match those in LoadPDCharacterizations
COL_NAMES = [
    "frequency",   # double
    "wavelength",  # double
    "bank",        # integer
    "vanadium",    # integer
    "container",   # integer
    "empty",       # integer
    "d_min",       # string
    "d_max",       # string
    "tof_min",     # double
    "tof_max"      # double
    ]

class PDDetermineCharacterizations(PythonAlgorithm):
    def category(self):
        return "Workflow/Diffraction"

    def name(self):
        return "PDDetermineCharacterizations"

    def PyInit(self):
        # input parameters
        self.declareProperty(WorkspaceProperty("InputWorkspace", "",
                                               Direction.Input),
                             "Workspace with logs to help identify frequency and wavelength")

        self.declareProperty(ITableWorkspaceProperty("Characterizations", "",
                                                     Direction.Input),
                             "Table of characterization information")

        # output parameters
        defaultMsg = " run to use. 0 for if it couldn't be determined."
        self.declareProperty("BackRun", 0, direction=Direction.Output,
                             doc="The background" + defaultMsg)
        self.declareProperty("NormRun", 0, direction=Direction.Output,
                             doc="The background" + defaultMsg)
        self.declareProperty("NormBackRun", 0, direction=Direction.Output,
                             doc="The background" + defaultMsg)

    def validateInputs(self):
        # correct workspace type
        char = self.getProperty("Characterizations").value
        if char.id() != "TableWorkspace":
            msg = "Characterizations should be a TableWorkspace"
            return {"Characterizations":msg}

        # correct number of columns
        colNames = char.getColumnNames()
        if len(colNames) != len(COL_NAMES):
            msg = "Encountered invalid number of columns in " \
                + "Characterizations. Found %d, expected 10" % len(a)
            return {"Characterizations":msg}

        # correct column names
        for (left, right) in zip(colNames, COL_NAMES):
            if left != right:
                msg = "Encountered column \"%s\" when expected \"%s\"" \
                    % (left, right)
                return {"Characterizations":msg}

        # everything is fine
        return {}

    def PyExec(self):
        char = self.getProperty("Characterizations").value
        if char.rowCount() <= 0:
            return
        print char.rowCount()
        wksp = self.getProperty("InputWorkspace").value

# Register algorthm with Mantid.
AlgorithmFactory.subscribe(PDDetermineCharacterizations)
