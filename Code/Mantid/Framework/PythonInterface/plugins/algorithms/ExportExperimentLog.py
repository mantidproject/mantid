import mantid.simpleapi as api
import mantid
from mantid.api import *
from mantid.kernel import *

import os
import datetime

class ExportExperimentLog(PythonAlgorithm):

    """ Algorithm to export experiment log
    """

    def summmary(self):
        return "Exports experimental log."

    def PyInit(self):
        """ Declaration of properties
        """
        # wsprop = mantid.api.MatrixWorkspaceProperty("InputWorkspace", "", mantid.kernel.Direction.Input)
        wsprop = MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input)
        self.declareProperty(wsprop, "Input workspace containing the sample log information. ")

        self.declareProperty(FileProperty("OutputFilename","", FileAction.Save, ['.txt, .csv']),
            "Output file of the experiment log.")

        filemodes = ["append", "fastappend", "new"]
        self.declareProperty("FileMode", "append", mantid.kernel.StringListValidator(filemodes),
            "Optional to create a new file or append to an existing file.")

        lognameprop = StringArrayProperty("SampleLogNames", values=[], direction=Direction.Input)
        self.declareProperty(lognameprop, "Sample log names.")

        logtitleprop = StringArrayProperty("SampleLogTitles", values=[], direction=Direction.Input)
        self.declareProperty(logtitleprop, "Log sample titles as file header.")

        logoperprop = StringArrayProperty("SampleLogOperation", values=[], direction=Direction.Input)
        self.declareProperty(logoperprop, "Operation on each log, including None (as no operation), min, max, average, sum and \"0\". ")

        fileformates = ["tab", "comma (csv)"]
        des = "Output file format.  'tab' format will insert a tab between 2 adjacent values; 'comma' will put a , instead. " + \
                "With this option, the posfix of the output file is .csv automatically. "
        self.declareProperty("FileFormat", "tab", mantid.kernel.StringListValidator(fileformates), des)

        # Time zone
        timezones = ["UTC", "America/New_York"]
        self.declareProperty("TimeZone", "America/New_York", StringListValidator(timezones))


        return


    def PyExec(self):
        """ Main execution body
        """
        # Process inputs
        self._processInputs()

        # Get sample log's value
        valuedict = self._getSampleLogsValue()
        if valuedict is None:
            return

        # Load input
        if self._filemode == "append":
            same = self._examineLogFile()
            if same is False:
                if self._reorderOld is True:
                    self._reorderExistingFile()
                else:
                    self._startNewFile()
                # ENDIFELSE
            # ENDIF (same)
        # ENDIF (filemode)

        # Write file
        if self._filemode == "new":
            # Create log file and write header
            self._createLogFile()

        # Append the new experiment log
        self._appendExpLog(valuedict)

        return

    def _processInputs(self):
        """ Process input properties
        """
        import os
        import os.path

        self._wksp = self.getProperty("InputWorkspace").value

        self._logfilename = self.getProperty("OutputFilename").value

        # Field and keys
        self._headerTitles = self.getProperty("SampleLogTitles").value
        self._sampleLogNames = self.getProperty("SampleLogNames").value
        ops = self.getProperty("SampleLogOperation").value

        if len(self._sampleLogNames) != len(ops):
            raise NotImplementedError("Size of sample log names and sample operations are unequal!")
        self._sampleLogOperations = {}
        for i in xrange(len(self._sampleLogNames)):
            key = self._sampleLogNames[i]
            value = ops[i]
            self._sampleLogOperations[key] = value
        # ENDFOR

        if len(self._headerTitles) > 0 and len(self._headerTitles) != len(self._sampleLogNames):
            raise NotImplementedError("Input header titles have a different length to sample log names")

        # Determine file mode
        if os.path.exists(self._logfilename) is False:
            self._filemode = "new"
            if len(self._headerTitles) == 0:
                raise NotImplementedError("Without specifying header title, unable to new a file.")
        else:
            self._filemode = self.getProperty("FileMode").value

        # Examine the file mode
        if self._filemode == "new" or self._filemode == "append":
            if len(self._headerTitles) != len(self._sampleLogNames):
                raise NotImplementedError("In mode new or append, there must be same number of sample titles and names")

        self.log().notice("File mode is %s. " % (self._filemode))

        # This is left for a feature that might be needed in future.
        self._reorderOld = False

        # Output file format
        self._fileformat = self.getProperty("FileFormat").value
        if self._fileformat == "tab":
            self._valuesep = "\t"
        else:
            self._valuesep = ","

        # Output file's postfix
        if self._fileformat == "comma (csv)":
            fileName, fileExtension = os.path.splitext(self._logfilename)
            if fileExtension != ".csv":
                # Force the extension of the output file to be .csv
                self._logfilename = "%s.csv" % (fileName)

        self._timezone = self.getProperty("TimeZone").value

        return

    def _createLogFile(self):
        """ Create a log file
        """
        if len(self._headerTitles) == 0:
            raise NotImplementedError("No header title specified. Unable to write a new file.")

        wbuf = ""
        for ititle in xrange(len(self._headerTitles)):
            title = self._headerTitles[ititle]
            wbuf += "%s" % (title)
            if ititle < len(self._headerTitles)-1:
                wbuf += self._valuesep

        try:
            ofile = open(self._logfilename, "w")
            ofile.write(wbuf)
            ofile.close()
        except OSError as err:
            raise NotImplementedError("Unable to write file %s. Check permission. Error message %s." % (
                self._logfilename, str(err)))

        return

    def _examineLogFile(self):
        """ Examine existing log file.
        If the titles in the existing log file are different from the inputs,
        then the old file will be saved
        """
        # Parse old file
        try:
            logfile = open(self._logfilename, "r")
            lines = logfile.readlines()
            logfile.close()
        except OSError as err:
            raise NotImplementedError("Unable to read existing log file %s. Error: %s." % (
                self._logfilename, str(err)))

        # Find the title line: first none-empty line
        for line in lines:
            titleline = line.strip()
            if len(titleline) > 0:
                break

        # Examine
        titles = titleline.split()

        same = True
        if len(titles) != len(self._headerTitles):
            same = False
        for ititle in xrange(len(titles)):
            title1 = titles[ititle]
            title2 = self._headerTitles[ititle]
            if title1 != title2:
                same = False
                break
            # ENDIF
        # ENDFOR

        return same


    def _startNewFile(self):
        """ Start a new file is user wants and save the older one to a different name
        """
        import datetime
        import time
        import os

        # Rename old file and reset the file mode

        # Rename the old one: split path from file, new name, and rename
        fileName, fileExtension = os.path.splitext(self._logfilename)

        now = datetime.datetime.now()
        nowstr = time.strftime("%Y_%B_%d_%H_%M")

        newfilename = fileName + "_" + nowstr + fileExtension
        os.rename(self._logfilename, newfilename)

        # Reset the file mode
        self._filemode = "new"

        return


    def _appendExpLog(self, logvaluedict):
        """ Append experiment log values to log file
        """
        self.log().information("Appending is called once.")

        # Write to a buffer
        wbuf = ""

        for logname in self._sampleLogNames:
            value = logvaluedict[logname]
            wbuf += "%s%s" % (str(value), self._valuesep)
        wbuf = wbuf[0:-1]

        # Append to file
        lfile = open(self._logfilename, "a")
        lfile.write("\n%s" %(wbuf))
        lfile.close()

        return

    def _reorderExistingFile(self):
        """ Re-order the columns of the existing experimental log file
        """
        raise NotImplementedError("Too complicated")

        return

    def _getSampleLogsValue(self):
        """ From the workspace to get the value
        """
        import numpy as np

        valuedict = {}
        run = self._wksp.getRun()
        if run is None:
            self.log().error("There is no Run object associated with workspace %s. " % (
                str(self._wksp)))
            return None

        for logname in self._sampleLogNames:
            isexist = run.hasProperty(logname)

            # check whether this property does exist.
            if isexist is False:
                # If not exist, use 0.00 as default
                self.log().warning("Sample log %s does not exist in given workspace %s. " % (logname, str(self._wksp)))
                valuedict[logname] = 0.0
                continue

            logproperty = run.getProperty(logname)
            logclass = logproperty.__class__.__name__

            # Get log value according to type
            if logclass == "StringPropertyWithValue":
                propertyvalue = logproperty.value
                operationtype = self._sampleLogOperations[logname]
                if operationtype.lower() == "localtime":
                    propertyvalue = self._convertLocalTimeString(propertyvalue)
            elif logclass == "FloatPropertyWithValue":
                propertyvalue = logproperty.value
            elif logclass == "FloatTimeSeriesProperty":
                operationtype = self._sampleLogOperations[logname]
                if operationtype.lower() == "min":
                    propertyvalue = min(logproperty.value)
                elif operationtype.lower() == "max":
                    propertyvalue = min(logproperty.value)
                elif operationtype.lower() == "average":
                    propertyvalue = np.average(logproperty.value)
                elif operationtype.lower() == "sum":
                    propertyvalue = np.sum(logproperty.value)
                elif operationtype.lower() == "0":
                    propertyvalue = logproperty.value[0]
                else:
                    raise NotImplementedError("Operation %s for FloatTimeSeriesProperty %s is not supported." % (operationtype, logname))
            else:
                raise NotImplementedError("Class type %d is not supported." % (logclass))

            valuedict[logname] = propertyvalue
        # ENDFOR

        return valuedict


    def _convertLocalTimeString(self, utctimestr):
        """ Convert a UTC time in string to the local time in string
        """
        from datetime import datetime
        from dateutil import tz

        utctimestr = str(utctimestr)

        self.log().information("Input UTC time = %s" % (utctimestr))

        from_zone = tz.gettz('UTC')
        to_zone = tz.gettz(self._timezone)

        try:
            if utctimestr.count(".") == 1:
                tail = utctimestr.split(".")[1]
                extralen = len(tail)-6
                extra = utctimestr[-extralen:]
                utctimestr = utctimestr[0:-extralen]
            utctime = datetime.strptime(utctimestr, '%Y-%m-%dT%H:%M:%S.%f')
        except ValueError as err:
            self.log().error("Unable to convert time string %s. Error message: %s" % (utctimestr, str(err)))
            raise err

        utctime = utctime.replace(tzinfo=from_zone)
        localtime = utctime.astimezone(to_zone)

        localtimestr = localtime.strftime("%Y-%m-%d %H:%M:%S.%f") + extra

        return localtimestr

# Register algorithm with Mantid
AlgorithmFactory.subscribe(ExportExperimentLog)
