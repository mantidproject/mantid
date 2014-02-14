"""*WIKI*

== Assumption ==
1. All logs specified by user should be synchronized. 

== File format ==
* Column 0:
* Column 1: 
* Column 2 to 2 + n:

*WIKI*"""

import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
import os


class ExportVulcanSampleLogs(PythonAlgorithm):
    """ Python algorithm to export sample logs to spread sheet file 
    for VULCAN
    """
    def category(self):
	""" Category
	"""
        return "Utilities;PythonAlgorithms"

    def name(self):
	""" Algorithm name
	"""
        return "ExportVulcanSampleLogs"

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
	timezones = ["America/New_York"]
        self.declareProperty("TimeZone", "America/New_York", StringListValidator(timezones))

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
	self._writeLogFile(logtimesdict, logvaluedict, loglength, localtimediff)

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
	    localtimediff = getLocalTimeShiftInSecond(time0, self._timezone)
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
	    wbuf += "%.6f\t %.6f\t " % (abstime, reltime) 
	    # Write each log value 
            for samplelog in self._sampleloglist:
		if logvaluedict[samplelog] is not None:
		    logvalue = logvaluedict[samplelog][i]
		else:
		    logvalue = 0.
		wbuf += "%.6f\t " % (logvalue)
	    wbuf += "\n"
	# ENDFOR

	try:
    	    ofile = open(self._outputfilename, "w")
    	    ofile.write(wbuf)
    	    ofile.close()
    	except IOError as err:
    	    print err
	    raise NotImplementedError("Unable to write file %s. Check permission." % (self._outputfilename))

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
	line0 = "Test date: %s" % (str(testdatetime)) 
	line1 = "Test description: %s" % (description) 
	line2 = self._headercontent

	# Write file
    	wbuf = line0 + "\n" + line1 + "\n" + line2 + "\n"
    	headerfilename = self._outputfilename.split(".")[0] + "_header.txt" 
	self.log().information("Writing header file %s ... " % (headerfilename))
   
	try: 
	    ofile = open(headerfilename, "w") 
	    ofile.write(wbuf) 
	    ofile.close()
	except OSError as err:
	    self.log().error(str(err))

        return
   


def getLocalTimeShiftInSecond(utctime, localtimezone):
    """ Calculate the difference between UTC time and local time of given 
    DataAndTime
    """
    from datetime import datetime
    from dateutil import tz

    print "Input UTC time = %s" % (str(utctime))

    from_zone = tz.gettz('UTC')
    to_zone = tz.gettz(localtimezone)

    t1str = (str(utctime)).split('.')[0].strip()
    print "About to convert time string: ", t1str
    try: 
        utc = datetime.strptime(t1str, '%Y-%m-%dT%H:%M:%S')
    except ValueError as err:
        print "Unable to convert time string %s. Error message: %s" % (t1str, str(err))
        raise err

    utc = utc.replace(tzinfo=from_zone)
    sns = utc.astimezone(to_zone)

    shift_in_hr = utc.hour - sns.hour

    shift_in_sec = shift_in_hr * 3600.

    return shift_in_sec


def convertToLocalTime(utctimestr, localtimezone):
    """ Convert UTC time in string to local time
    """
    from datetime import datetime
    from dateutil import tz

    print "Input UTC time = %s" % (utctimestr)

    from_zone = tz.gettz('UTC')
    to_zone = tz.gettz(localtimezone)

    t1str = (utctimestr).split('.')[0]
    utc = datetime.strptime(t1str, '%Y-%m-%dT%H:%M:%S')

    utc = utc.replace(tzinfo=from_zone)
    sns = utc.astimezone(to_zone)

    return str(sns)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ExportVulcanSampleLogs)
