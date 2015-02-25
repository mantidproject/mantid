#pylint: disable=no-init
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
DEF_INFO = {
    "frequency":0.,
    "wavelength":0.,
    "bank":1,
    "vanadium":0,
    "container":0,
    "empty":0,
    "d_min":"",
    "d_max":"",
    "tof_min":0.,
    "tof_max":0.
    }

class PDDetermineCharacterizations(PythonAlgorithm):
    def category(self):
        return "Workflow\\Diffraction\\UsesPropertyManager"

    def name(self):
        return "PDDetermineCharacterizations"

    def summary(self):
        return "Determines the characterizations of a workspace."

    def PyInit(self):
        # input parameters
        self.declareProperty(WorkspaceProperty("InputWorkspace", "",
                                               Direction.Input),
                             "Workspace with logs to help identify frequency and wavelength")

        self.declareProperty(ITableWorkspaceProperty("Characterizations", "",
                                                     Direction.Input),
                             "Table of characterization information")

        self.declareProperty("ReductionProperties",
                             "__pd_reduction_properties",
                             validator=StringMandatoryValidator(),
                             doc="Property manager name for the reduction")

        defaultMsg = " run to use. 0 to use value in table, -1 to not use."
        self.declareProperty("BackRun", 0,
                             doc="The background" + defaultMsg)
        self.declareProperty("NormRun", 0,
                             doc="The background" + defaultMsg)
        self.declareProperty("NormBackRun", 0,
                             doc="The background" + defaultMsg)

        self.declareProperty(StringArrayProperty("FrequencyLogNames", ["SpeedRequest1", "Speed1", "frequency"],\
            direction=Direction.Input),\
            "Possible log names for frequency.")

        self.declareProperty(StringArrayProperty("WaveLengthLogNames", ["LambdaRequest", "lambda"],\
            direction=Direction.Input),\
            "Candidate log names for wave length.")

        return

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
        # setup property manager to return
        manager_name = self.getProperty("ReductionProperties").value
        if PropertyManagerDataService.doesExist(manager_name):
            manager = PropertyManagerDataService.retrieve(manager_name)
        else:
            manager = PropertyManager()

        # empty characterizations table means return the default values
        char = self.getProperty("Characterizations").value
        if char.rowCount() <= 0:
            for key in COL_NAMES:
                if not manager.existsProperty(key):
                    manager[key] = DEF_INFO[key]
            PropertyManagerDataService.addOrReplace(manager_name, manager)
            return
        wksp = self.getProperty("InputWorkspace").value

        # determine wavelength and frequency
        frequency = self.getFrequency(wksp.getRun(), str(wksp))
        wavelength = self.getWavelength(wksp.getRun(), str(wksp))
        self.log().information("Determined frequency: " + str(frequency) \
                                   + " Hz, center wavelength:" \
                                   + str(wavelength) + " Angstrom")

        # get a row of the table
        info = self.getLine(char, frequency, wavelength)

        # update the characterization runs as necessary
        propNames = ("BackRun",   "NormRun",  "NormBackRun")
        dictNames = ("container", "vanadium", "empty")
        for (propName, dictName) in zip(propNames, dictNames):
            runNum = self.getProperty(propName).value
            if runNum < 0: # reset value
                info[dictName] = 0
            elif runNum > 0: # override value
                info[dictName] = runNum

        # convert to a property manager
        self.processInformation(manager, info)
        PropertyManagerDataService.addOrReplace(manager_name, manager)

    def processInformation(self, prop_man, info_dict):
        for key in COL_NAMES:
            val = info_dict[key]
            # Convert comma-delimited list to array, else return the original
            # value.
            if type("") == type(val):
                if (len(val)==0) and  (key in DEF_INFO.keys()):
                    val = DEF_INFO[key]
                else:
                    try:
                        val = [float(x) for x in val.split(',')]
                    except ValueError, err:
                        self.log().error("Error to parse key: '%s' value = '%s'. " % (str(key), str(val)))
                        raise NotImplementedError(str(err))

            try:
                prop_man[key] = val
            except TypeError:
                # Converter error, so remove old value first
                del prop_man[key]
                prop_man[key] = val

    def closeEnough(self, left, right):
        left = float(left)
        right = float(right)
        if abs(left-right) == 0.:
            return True
        if 100. * abs(left-right)/left < 5.:
            return True
        return False

    def getLine(self, char, frequency, wavelength):
        """ Get line in the characterization file with given frequency and wavelength
        """
        # empty dictionary if things are wrong
        if frequency is None or wavelength is None:
            return dict(DEF_INFO)

        # go through every row looking for a match
        result = dict(DEF_INFO)
        icount = 0
        for i in xrange(char.rowCount()):
            row = char.row(i)
            if not self.closeEnough(frequency, row['frequency']):
                continue
            if not self.closeEnough(wavelength, row['wavelength']):
                continue
            result = dict(row)
            icount += 1

        self.log().information("Total %d rows are parsed for frequency = %f, wavelength = %f" % (icount, frequency, wavelength))
        return result

    def getFrequency(self, logs, wkspName):
        """ Get frequency from log
        """
        frequencynames = self.getProperty("FrequencyLogNames").value
        self.log().notice("Type of frequency names is %s" % (str(type(frequencynames))))
        for name in frequencynames:
            if name in logs.keys():
                frequency = logs[name]
                if frequency.units != "Hz":
                    msg = "When looking at " + name \
                        + " log encountered unknown units for frequency. " \
                        + "Only know how to deal with " \
                        + "frequency in Hz, not " + frequency.units
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
                               % wkspName)
        return None

    def getWavelength(self, logs, wkspName):
        """ Get wave length
        Wavelength can be given by 2 sample logs, either LambdaRequest or lambda.
        And its unit can be either Angstrom or A.
        """
        wavelengthnames = self.getProperty("WaveLengthLogNames").value

        for name in wavelengthnames:
            # skip if not exists
            if name not in logs.keys():
                continue

            # get value
            wavelength = logs[name]

            # unit
            if name == "LambdaRequest":
                if wavelength.units != "Angstrom":
                    msg = "Only know how to deal with LambdaRequest in Angstrom, not %s" % (wavelength.units)
                    self.log().warning(msg)
                    break

            elif name == "lambda":
                if wavelength.units != "A":
                    msg = "Only know how to deal with lambda in A, not %s" % (wavelength.units)
                    self.log().warning(msg)
                    break

            else:
                if wavelength.units != "Angstrom" and wavelength.units != "A":
                    msg = "Only know how to deal with %s in Angstrom (A) but not %s" % (name,\
                            wavelength.units)
                    self.log().warning(msg)
                    break

            # return
            wavelength = wavelength.getStatistics().mean
            if wavelength == 0.:
                self.log().warning("'%s' mean value is zero" % name)
                break
            else:
                return wavelength

        # ENDFOR

        return None

# Register algorthm with Mantid.
AlgorithmFactory.subscribe(PDDetermineCharacterizations)
