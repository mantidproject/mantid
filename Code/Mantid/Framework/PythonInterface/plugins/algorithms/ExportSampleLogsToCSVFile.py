#pylint: disable=no-init,invalid-name,too-many-instance-attributes
from mantid.api import *
from mantid.kernel import *
import os

class ExportSampleLogsToCSVFile(PythonAlgorithm):
    """ Python algorithm to export sample logs to spread sheet file
    for VULCAN
    """

    _wksp = None
    _outputfilename = None
    _sampleloglist = None
    _headerconten = None
    _writeheader = None
    _headercontent = None
    _timezone = None
    _timeTolerance = None
    _maxtimestamp = None
    _maxtime = None
    _starttime = None
    _localtimediff = None

    def category(self):
        """ Category
        """
        return "Utility;PythonAlgorithms"

    def name(self):
        """ Algorithm name
        """
        return "ExportSampleLogsToCSVFile"

    def summary(self):
        return "Exports sample logs to spreadsheet file."

    def PyInit(self):
        """ Declare properties
        """
        # Input workspace
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),
                             "Name of data workspace containing sample logs to be exported. ")

        # Output file name
        self.declareProperty(FileProperty("OutputFilename", "", FileAction.Save, [".txt"]),
                             "Name of the output sample environment log file name.")

        # Sample log names
        self.declareProperty(StringArrayProperty("SampleLogNames", values=[], direction=Direction.Input),
                             "Names of sample logs to be exported in a same file.")

        # Header
        self.declareProperty("WriteHeaderFile", False, "Flag to generate a sample log header file.")

        self.declareProperty("Header", "", "String in the header file.")

        # Time zone
        timezones = ["UTC", "America/New_York", "Asia/Shanghai", "Australia/Sydney", "Europe/London", "GMT+0",\
                "Europe/Paris", "Europe/Copenhagen"]

        description = "Sample logs recorded in NeXus files (in SNS) are in UTC time.  TimeZone " + \
            "can allow the algorithm to output the log with local time."
        self.declareProperty("TimeZone", "America/New_York", StringListValidator(timezones), description)

        # Log time tolerance
        self.declareProperty("TimeTolerance", 0.01,
                             "If any 2 log entries with log times within the time tolerance, " + \
                             "they will be recorded in one line. Unit is second. ")

        return


    def PyExec(self):
        """ Main executor
        """
        # Read inputs
        self._getProperties()

        # Read in logs
        logtimesdict, logvaluedict, loglength = self._readSampleLogs()

        # Local time difference
        localtimediff = self._calLocalTimeDiff(logtimesdict, loglength)

        # Write log file
        # self._writeLogFile(logtimesdict, logvaluedict, loglength, localtimediff)
        logtimeslist = []
        logvaluelist = []
        for logname in self._sampleloglist:
            logtimeslist.append(logtimesdict[logname])
            logvaluelist.append(logvaluedict[logname])
        self._writeAscynLogFile(logtimeslist, logvaluelist, localtimediff, self._timeTolerance)

        # Write header file
        if self._writeheader is True:
            testdatetime = self._wksp.getRun().getProperty("run_start").value
            description = "Type your description here"
            self._writeHeaderFile(testdatetime, description)

        return


    def _getProperties(self):
        """ Get and process properties
        """
        self._wksp = self.getProperty("InputWorkspace").value

        self._outputfilename = self.getProperty("OutputFilename").value
        filedir = os.path.split(self._outputfilename)[0]
        if os.path.exists(filedir) is False:
            raise NotImplementedError("Directory %s does not exist.  File cannot be written." % (filedir))

        self._sampleloglist = self.getProperty("SampleLogNames").value
        if len(self._sampleloglist) == 0:
            raise NotImplementedError("Sample logs names cannot be empty.")

        self._writeheader = self.getProperty("WriteHeaderFile").value
        self._headercontent = self.getProperty("Header").value
        if self._writeheader is True and len(self._headercontent.strip()) == 0:
            self.log().warning("Header is empty. Thus WriteHeaderFile is forced to be False.")
            self._writeheader = False

        self._timezone = self.getProperty("TimeZone").value

        self._timeTolerance = self.getProperty("TimeTolerance").value
        if (self._timeTolerance) <= 0.:
            raise NotImplementedError("TimeTolerance must be larger than zero.")

        return


    def _calLocalTimeDiff(self, logtimesdict, loglength):
        """ Calcualte the time difference between local time and UTC in seconds
        """
        # Find out local time
        if loglength > 0:
            # Locate time0
            for key in logtimesdict.keys():
                times = logtimesdict[key]
                if times is not None:
                    time0 = logtimesdict[key][0]
                    break
            # Local time difference
            localtimediff = getLocalTimeShiftInSecond(time0, self._timezone, self.log())
        else:
            localtimediff = 0

        return localtimediff


    def _writeLogFile(self, logtimesdict, logvaluedict, loglength, localtimediff):
        """ Write the logs to file
        """
        wbuf = ""

        # Init time
        if loglength > 0:
            for log in logtimesdict.keys():
                if logtimesdict[log] is not None:
                    time0 = logtimesdict[log][0]
                    abstime_init = time0.totalNanoseconds() * 1.E-9 - localtimediff
                    times = logtimesdict[log]
                    break

        # Loop
        for i in xrange(loglength):
            abstime = times[i].totalNanoseconds() * 1.E-9 - localtimediff
            reltime = abstime - abstime_init
            # Write absoute time and relative time
            wbuf += "%.6f\t%.6f\t" % (abstime, reltime)
            # Write each log value
            for samplelog in self._sampleloglist:
                if logvaluedict[samplelog] is not None:
                    logvalue = logvaluedict[samplelog][i]
                else:
                    logvalue = 0.
                wbuf += "%.6f\t" % (logvalue)
            wbuf += "\n"
        # ENDFOR

        try:
            ofile = open(self._outputfilename, "w")
            ofile.write(wbuf)
            ofile.close()
        except IOError:
            raise NotImplementedError("Unable to write file %s. Check permission." % (self._outputfilename))

        return

    def _getLogsInfo(self, logtimeslist):
        """ Get maximum number of lines, staring time and ending time in the output log file
        """
        maxnumlines = 0
        first = True
        for logtimes in logtimeslist:
            # skip NONE
            if logtimes is None:
                continue

            # count on lines
            tmplines = len(logtimes)
            maxnumlines +=  tmplines

            # start and max time
            tmpstarttime = logtimes[0]
            tmpmaxtime = logtimes[-1]
            if first is True:
                starttime = tmpstarttime
                maxtime = tmpmaxtime
                first = False
            else:
                if tmpmaxtime > maxtime:
                    maxtime = tmpmaxtime
                if tmpstarttime < starttime:
                    starttime = tmpstarttime
            # ENDIFELSE

        return maxnumlines, starttime, maxtime


    def _writeAscynLogFile(self, logtimeslist, logvaluelist, localtimediff, timetol):
        """ Logs are recorded upon the change of the data
        time tolerance : two log entries within time tolerance will be recorded as one
        Arguments
        - timetol  : tolerance of time (in second)
        """
        # Check input
        if logtimeslist.__class__.__name__ != "list":
            raise NotImplementedError("Input log times is not list")
        if logvaluelist.__class__.__name__ != "list":
            raise NotImplementedError("Input log value is not list")

        wbuf = ""
        currtimeindexes = []
        for dummy_i in xrange(len(logtimeslist)):
            currtimeindexes.append(0)
        nextlogindexes = []

        continuewrite = True
        linecount = 0
        maxcount, mintime, maxtime = self._getLogsInfo(logtimeslist)
        self._maxtimestamp = maxcount
        self._maxtime = maxtime
        self._starttime = mintime

        self._localtimediff = localtimediff
        while continuewrite:
            self._findNextTimeStamps(logtimeslist, currtimeindexes, timetol, nextlogindexes)
            self.log().debug("Next time stamp log indexes: %s" % (str(nextlogindexes)))
            if len(nextlogindexes) == 0:
                # No new indexes that can be found
                continuewrite = False
            else:
                #
                templine = self._writeNewLine(logtimeslist, logvaluelist, currtimeindexes, nextlogindexes)
                self.log().debug("Write new line %d: %s" % (linecount, templine))
                self._progressTimeIndexes(currtimeindexes, nextlogindexes)
                wbuf += templine + "\n"
                linecount += 1
            # ENDIF

            if linecount > maxcount:
                raise NotImplementedError("Logic error.")
        # ENDWHILE

        # Remove last "\n"
        if wbuf[-1] == "\n":
            wbuf = wbuf[:-1]

        try:
            ofile = open(self._outputfilename, "w")
            ofile.write(wbuf)
            ofile.close()
        except IOError:
            raise NotImplementedError("Unable to write file %s. Check permission." % (self._outputfilename))

        return

    def _findNextTimeStamps(self, logtimeslist, currtimeindexes, timetol, nexttimelogindexes):
        """
        Arguments:
        - nexttimelogindexes : (output) indexes of logs for next time stamp
        """
        # clear output
        nexttimelogindexes[:] = []

        # Initialize
        nexttime = self._maxtime

        for i in xrange(0, len(logtimeslist)):
            # skip the None log
            if logtimeslist[i] is None:
                continue

            timeindex = currtimeindexes[i]
            if timeindex >= len(logtimeslist[i]):
                # skip as out of boundary of log
                continue
            tmptime = logtimeslist[i][timeindex]
            self.log().debug("tmptime type = %s " % ( type(tmptime)))

            # difftime = calTimeDiff(tmptime, nexttime)
            difftime = (tmptime.totalNanoseconds() - nexttime.totalNanoseconds())*1.0E-9

            if abs(difftime) < timetol:
                # same ...
                nexttimelogindexes.append(i)
            elif difftime < 0:
                # new smaller time
                nexttime = tmptime
                nexttimelogindexes[:] = []
                nexttimelogindexes.append(i)
            # ENDIF
        # ENDIF

        return

    def _writeNewLine(self, logtimeslist, logvaluelist, currtimeindexes, nexttimelogindexes):
        """ Write a new line
        """
        # Check
        if len(nexttimelogindexes) == 0:
            raise NotImplementedError("Logic error")

        # Log time
        # self.log().information("logtimelist of type %s." % (type(logtimeslist)))
        #logtime = logtimeslist[currtimeindexes[nexttimelogindexes[0]]]
        logindex = nexttimelogindexes[0]
        logtimes = logtimeslist[logindex]
        thislogtime = logtimes[currtimeindexes[logindex]]
        # FIXME : refactor the following to increase efficiency
        abstime = thislogtime.totalNanoseconds() * 1.E-9 - self._localtimediff
        reltime = thislogtime.totalNanoseconds() * 1.E-9 - self._starttime.totalNanoseconds() * 1.0E-9
        wbuf = "%.6f\t%.6f\t" % (abstime, reltime)

        # Log valuess
        for i in xrange(len(logvaluelist)):
            timeindex = currtimeindexes[i]
            if not i in nexttimelogindexes:
                timeindex -= 1
                if timeindex < 0:
                    timeindex = 0
            if logvaluelist[i] is None:
                logvalue = 0.
            else:
                logvalue = logvaluelist[i][timeindex]

            # FIXME - This case is not considered yet
            # if logvaluedict[samplelog] is not None:
            #     logvalue = logvaluedict[samplelog][i]
            # else:
            #     logvalue = 0.
            wbuf += "%.6f\t" % (logvalue)
        # ENDFOR

        return wbuf


    def _progressTimeIndexes(self, currtimeindexes, nexttimelogindexes):
        """ Progress index
        """
        for i in xrange(len(currtimeindexes)):
            if i in nexttimelogindexes:
                currtimeindexes[i] += 1

        return


    def _readSampleLogs(self):
        """ Read sample logs
        """
        import sys

        # Get all properties' times and value and check whether all the logs are in workspaces
        samplerun = self._wksp.getRun()

        logtimesdict = {}
        logvaluedict = {}
        for samplename in self._sampleloglist:
            # Check existence
            logexist = samplerun.hasProperty(samplename)

            if logexist is True:
                # Get hold of sample values
                p = samplerun.getProperty(samplename)
                logtimesdict[samplename] = p.times
                logvaluedict[samplename] = p.value

            else:
                # Add None
                self.log().warning("Sample log %s does not exist. " % (samplename))
                logtimesdict[samplename] = None
                logvaluedict[samplename] = None

            # ENDIF
        # ENDFOR

        # Check properties' size
        loglength = sys.maxint
        for i in xrange(len(self._sampleloglist)):
            if logtimesdict[self._sampleloglist[i]] is not None:
                tmplength = len(logtimesdict[self._sampleloglist[i]])
                if loglength != tmplength:
                    if loglength != sys.maxint:
                        self.log().warning("Log %s has different length from previous ones. " % (self._sampleloglist[i]))
                    loglength = min(loglength, tmplength)
                # ENDIF
            # ENDIF
        # ENDFOR

        if loglength == sys.maxint:
            self.log().warning("None of given log names is found in workspace. ")
            loglength = 0
        else:
            self.log().information("Final Log length = %d" % (loglength))

        return (logtimesdict, logvaluedict, loglength)


    def _writeHeaderFile(self, testdatetime, description):
        """ Write the header file for a LoadFrame
        """
        # Construct 3 lines of the header file
        testdatetime_mk = DateAndTime(testdatetime)
        line0 = "Test date: %s (%.6f) Time Zone: %s" % (str(testdatetime), float(testdatetime_mk.totalNanoseconds())/1.0E9, self._timezone)
        line1 = "Test description: %s" % (description)
        line2 = self._headercontent

        # Write file
        wbuf = line0 + "\n" + line1 + "\n" + line2 + "\n"
        # headerfilename = self._outputfilename.split(".")[0] + "_header.txt"
        filepath = os.path.dirname(self._outputfilename)
        basename = os.path.basename(self._outputfilename)
        baseheadername = basename.split(".txt")[0] + "_header.txt"
        headerfilename = os.path.join(filepath, baseheadername)

        self.log().information("Writing header file %s ... " % (headerfilename))

        try:
            ofile = open(headerfilename, "w")
            ofile.write(wbuf)
            ofile.close()
        except OSError as err:
            self.log().error(str(err))

        return

def getLocalTimeShiftInSecond(utctime, localtimezone, logger = None):
    """ Calculate the difference between UTC time and local time of given
    DataAndTime
    """
    from datetime import datetime
    from dateutil import tz

    if logger:
        logger.information("Input UTC time = %s" % (str(utctime)))

    # Return early if local time zone is UTC
    if localtimezone == "UTC":
        return 0

    # Find out difference in time zone
    from_zone = tz.gettz('UTC')
    to_zone = tz.gettz(localtimezone)

    t1str = (str(utctime)).split('.')[0].strip()
    if logger:
        logger.information("About to convert time string: %s" % t1str)
    try:
        if t1str.count("T") == 1:
            utc = datetime.strptime(t1str, '%Y-%m-%dT%H:%M:%S')
        else:
            utc = datetime.strptime(t1str, '%Y-%m-%d %H:%M:%S')
    except ValueError as err:
        raise err

    utc = utc.replace(tzinfo=from_zone)
    sns = utc.astimezone(to_zone)

    newsns = sns.replace(tzinfo=None)
    newutc = utc.replace(tzinfo=None)

    shift = newutc-newsns
    shift_in_sec = shift.seconds

    return shift_in_sec


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ExportSampleLogsToCSVFile)
