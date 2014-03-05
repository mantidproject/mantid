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
        wksp = self.getProperty("InputWorkspace").value
        frequency = self.getFrequency(wksp.getRun())
        wavelength = self.getWavelength(wksp.getRun())
        self.log().information("Determined frequency: " + str(frequency) \
                                   + " Hz, center wavelength:" \
                                   + str(wavelength) + " Angstrom")

    def getFrequency(self, logs):
        for name in ["SpeedRequest1", "Speed1", "frequency"]:
            if name in logs.keys():
                frequency = logs[name]
                if frequency.units != "Hz":
                    msg = "When looking at %s log encountered unknown units" \
                        + " for frequency. Only know how to deal with " \
                        + "frequency in Hz, not %s" % (name, frequency.units)
                    self.log().information(msg)
                else:
                    frequency = frequency.getStatistics().mean
                    if frequency == 0.: 
                        self.log().information("'%s' mean value is zero" % name)
                    else:
                        self.log().information("Found frequency in %s log" \
                                                   % name)
                        return frequency
        self.log().warning("Failed to determine frequency in \"%s\"" \
                               % str(wksp))
        return None

    def getWavelength(self, logs):
        name = "LambdaRequest"
        if name in logs.keys():
            wavelength = logs[name]
            if wavelength.units != "Angstrom":
                msg = "Only know how to deal with LambdaRequest in "\
                    "Angstrom, not $s" % wavelength
                self.log().information(msg)
            else:
                wavelength = wavelength.getStatistics().mean
                if wavelength == 0.:
                    self.log().information("'%s' mean value is zero" % name)
                else:
                    return wavelength

        self.log().warning("Failed to determine wavelength in \"%s\"" \
                               % str(wksp))
        return None

# Register algorthm with Mantid.
AlgorithmFactory.subscribe(PDDetermineCharacterizations)
