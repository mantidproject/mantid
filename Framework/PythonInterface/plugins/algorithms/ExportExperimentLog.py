# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
import numpy as np
import mantid
from mantid.api import *
from mantid.kernel import *
import datetime
import time
import os
from dateutil import tz
from six.moves import range

#pylint: disable=too-many-instance-attributes


class ExportExperimentLog(PythonAlgorithm):

    """ Algorithm to export experiment log
    """

    _wksp = None
    _sampleLogNames = None
    _sampleLogOperations = None
    _fileformat = None
    _csv_separator = None
    _logfilename = None
    _reorderOld = None
    _timezone = None
    _titleToOrder = None
    _orderRecord = None
    titleToOrder = None
    _removeDupRecord = None
    _headerTitles = None
    _filemode = None

    def summmary(self):
        return "Exports experimental log."

    def category(self):
        """ Defines the category the algorithm will be put in the algorithm browser
        """
        return 'DataHandling\\Logs'

    def seeAlso(self):
        return ["ExportSampleLogsToCSVFile"]

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

        self.declareProperty("OrderByTitle", "", "Log file will be ordered by the value of this title from low to high.")
        self.declareProperty("RemoveDuplicateRecord", False, "Coupled with OrderByTitle, duplicated record will be removed.")

        overrideprop = StringArrayProperty("OverrideLogValue", values=[], direction=Direction.Input)
        self.declareProperty(overrideprop, "List of paired strings as log title and value to override values from workspace.")

        # Time zone
        timezones = ["UTC", "America/New_York", "Asia/Shanghai", "Australia/Sydney", "Europe/London", "GMT+0",
                     "Europe/Paris", "Europe/Copenhagen"]
        self.declareProperty("TimeZone", "America/New_York", StringListValidator(timezones))

        return

    def PyExec(self):
        """ Main execution body
        """
        # Process inputs
        self._process_inputs()

        # Get sample log's value
        sample_log_dict = self._get_sample_logs_value()
        if len(sample_log_dict) == 0:
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
        self._append_experiment_log(sample_log_dict)

        # Order the record file
        if self._orderRecord is True:
            self._orderRecordFile()

        return

    #pylint: disable=too-many-branches
    def _process_inputs(self):
        """ Process input properties
        """
        self._wksp = self.getProperty("InputWorkspace").value

        self._logfilename = self.getProperty("OutputFilename").value

        # Field and keys
        self._headerTitles = self.getProperty("SampleLogTitles").value
        self._sampleLogNames = self.getProperty("SampleLogNames").value

        # operations
        ops = self.getProperty("SampleLogOperation").value
        if len(self._sampleLogNames) != len(ops):
            raise RuntimeError("Size of sample log names and sample operations are unequal!")
        self._sampleLogOperations = []
        for i in range(len(self._sampleLogNames)):
            value = ops[i]
            self._sampleLogOperations.append(value)
        # ENDFOR

        if len(self._headerTitles) > 0 and len(self._headerTitles) != len(self._sampleLogNames):
            raise RuntimeError("Input header titles have a different length to sample log names")

        # Output file format
        self._fileformat = self.getProperty("FileFormat").value
        if self._fileformat == "tab":
            self._csv_separator = "\t"
        else:
            self._csv_separator = ","

        # Output file's postfix
        if self._fileformat == "comma (csv)":
            fileName, fileExtension = os.path.splitext(self._logfilename)
            if fileExtension != ".csv":
                # Force the extension of the output file to be .csv
                self._logfilename = "%s.csv" % (fileName)

        # Determine file mode
        if os.path.exists(self._logfilename) is False:
            self._filemode = "new"
            if len(self._headerTitles) == 0:
                raise RuntimeError("Without specifying header title, unable to new a file.")
            self.log().debug("Log file %s does not exist. So file mode is NEW." % (self._logfilename))
        else:
            self._filemode = self.getProperty("FileMode").value
            self.log().debug("FileMode is from user specified value.")

        # Examine the file mode
        if self._filemode in ["new", "append"]:
            # For new and append mode, header title must be given and correspond to log names
            if len(self._headerTitles) != len(self._sampleLogNames):
                raise RuntimeError("In mode new or append, Header titles (now {}) must be given and"
                                   " must be same number of sample titles and names".format(self._headerTitles))
        else:
            # Fast append mode: header title might be overridden by sample log locations
            if len(self._headerTitles) != len(self._sampleLogNames):
                self._headerTitles = self._sampleLogNames[:]
        # END-IF-ELSE

        self.log().information("File mode: {}".format(self._filemode))

        # This is left for a feature that might be needed in future.
        self._reorderOld = False

        self._timezone = self.getProperty("TimeZone").value

        # Determine whether output log-record file should be ordered by value of some log
        self._orderRecord = False
        self._titleToOrder = None
        if self._filemode != "new":
            ordertitle = self.getProperty("OrderByTitle").value
            if ordertitle in self._headerTitles:
                self._orderRecord = True
                self._removeDupRecord = self.getProperty("RemoveDuplicateRecord").value
                self.titleToOrder = ordertitle
            else:
                self.log().warning("Specified title to order by (%s) is not in given log titles." % (ordertitle))

        if self._orderRecord is False:
            self._removeDupRecord = False

        return

    def _createLogFile(self):
        """ Create a log file
        """
        if len(self._headerTitles) == 0:
            raise RuntimeError("No header title specified. Unable to write a new file.")

        wbuf = ""
        for ititle in range(len(self._headerTitles)):
            title = self._headerTitles[ititle]
            wbuf += "%s" % (title)
            if ititle < len(self._headerTitles)-1:
                wbuf += self._csv_separator

        try:
            ofile = open(self._logfilename, "w")
            ofile.write(wbuf)
            ofile.close()
        except OSError as err:
            raise RuntimeError("Unable to write file %s. Check permission. Error message %s." % (
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
            raise RuntimeError("Unable to read existing log file %s. Error: %s." % (
                self._logfilename, str(err)))

        # Find the title line: first none-empty line
        for line in lines:
            titleline = line.strip()
            if len(titleline) > 0:
                break

        # Examine
        titles = titleline.split()
        self.log().debug("Examine finds titles: %s" % (titles))

        same = True
        if len(titles) != len(self._headerTitles):
            if len(self._headerTitles) == 0:
                self._headerTitles = titles[:]
            else:
                same = False
        for ititle in range(len(titles)):
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

        # Rename old file and reset the file mode

        # Rename the old one: split path from file, new name, and rename
        fileName, fileExtension = os.path.splitext(self._logfilename)

        #now = datetime.datetime.now()
        nowstr = time.strftime("%Y_%B_%d_%H_%M")

        newfilename = fileName + "_" + nowstr + fileExtension
        os.rename(self._logfilename, newfilename)

        # Reset the file mode
        self._filemode = "new"

        return

    def _append_experiment_log(self, log_value_dict):
        """ Append experiment log values to log file
        """
        self.log().information("Appending is called once.")

        # Write to a buffer
        wbuf = ''

        self.log().debug("Samlpe Log Names: {}".format(self._sampleLogNames))
        self.log().debug("Title      Names: {}".format(self._headerTitles))

        for il in range(len(self._headerTitles)):
            # from log value dictionary (including from overridden and workspace)
            log_title_i = self._headerTitles[il]
            operation_type = self._sampleLogOperations[il]

            # form key
            log_key = '{}'.format(log_title_i)
            if operation_type is not None:
                log_key = '{}-{}'.format(log_key, operation_type)

            # locate value
            if log_key in log_value_dict:
                value = log_value_dict[log_key]
            elif log_title_i in log_value_dict:
                value = log_value_dict[log_title_i]
            else:
                raise RuntimeError('Log {} with operation {} cannot be found in log value dict.  Available keys are {}'
                                   ''.format(log_title_i, operation_type, log_value_dict.keys()))
            # END-IF-ELSE

            # append line
            wbuf += "{}{}".format(value, self._csv_separator)
        # END-FOR

        # remove last separator
        wbuf = wbuf[0:-1]

        # Append to file
        lfile = open(self._logfilename, "a")
        lfile.write("\n%s" %(wbuf))
        lfile.close()

        return

    #pylint: disable=too-many-branches
    def _orderRecordFile(self):
        """ Check and order (if necessary) record file
        by value of specified log by title
        """
        self.log().debug("Order Record File!")

        # Read line
        lfile = open(self._logfilename, "r")
        lines = lfile.readlines()
        lfile.close()

        # Create dictionary for the log value
        titlelines = []
        linedict = {}
        ilog = self._headerTitles.index(self.titleToOrder)

        totnumlines = 0
        for line in lines:
            cline = line.strip()
            if len(cline) == 0:
                continue

            if cline.startswith(self._headerTitles[0]) is True or cline.startswith('#'):
                # title line or comment line
                titlelines.append(line)
            else:
                # value line
                try:
                    keyvalue = line.split(self._csv_separator)[ilog].strip()
                except IndexError:
                    self.log().error("Order record failed.")
                    return
                if (keyvalue in linedict) is False:
                    linedict[keyvalue] = []
                linedict[keyvalue].append(line)
                totnumlines += 1
            # ENDIFELSE
        # ENDFOR

        # Check needs to re-order
        # This test does not work with python 3, you can not assume the order of a dictionary
        # if list(linedict.keys()) != sorted(linedict.keys()):
        if True: # temporary hack to get it working with python 3, always write a new file!
            # Re-write file
            wbuf = ""

            # title line
            for line in titlelines:
                wbuf += line

            # log entry line
            numlines = 0

            # consider the mode to remove duplicate
            if self._removeDupRecord is True:
                totnumlines = len(linedict.keys())

            for ivalue in sorted(linedict.keys()):
                if self._removeDupRecord is True:
                    # If duplicated records is to be removed, only consider the last record
                    line = linedict[ivalue][-1]
                    wbuf += line
                    if numlines != totnumlines-1 and wbuf[-1] != '\n':
                        wbuf += '\n'
                    numlines += 1

                else:
                    # Consider all!
                    for line in linedict[ivalue]:
                        wbuf += line
                        # Add extra \n in case reordered
                        if numlines != totnumlines-1 and wbuf[-1] != '\n':
                            wbuf += '\n'
                        numlines += 1
                    # ENDFOR
                # ENDFOR
            # ENDFOR

            # Last line should not ends with \n
            if wbuf[-1] == '\n':
                wbuf = wbuf[0:-1]

            # Remove unsupported character which may cause importing error of GNUMERIC
            wbuf = wbuf.replace(chr(0),"")

            # Re-write file
            ofile = open(self._logfilename, "w")
            ofile.write(wbuf)
            ofile.close()

        # ENDIF

        return

    def _reorderExistingFile(self):
        """ Re-order the columns of the existing experimental log file
        """
        raise RuntimeError("Too complicated")

    def _parse_overriding_logs(self):
        """ Parse the input overriding log value list
        :return: dictionary ([log name] = log value)
        """
        # log value dict init
        log_value_dict = dict()

        # Override log values: it will not work in 'FastAppend' mode (which hasn't been implemented) to override
        override_list = self.getProperty("OverrideLogValue").value
        if len(override_list) == 0:
            # empty
            pass
        elif len(override_list) % 2 == 1:
            # incomplete
            raise RuntimeError("Number of items in OverrideLogValue must be even. Now there are {}"
                               "items.".format(len(override_list)))
        else:
            # parse
            for i in range(int(len(override_list)/2)):
                title = override_list[2*i]
                if title in self._headerTitles:
                    log_value_dict[title] = override_list[2*i+1]
                else:
                    self.log().warning("Override title {} is not recognized. ".format(title))
            # END-FOR
        # END-IF-ELSE

        return log_value_dict

    #pylint: disable=too-many-branches
    def _get_sample_logs_value(self):
        """ Get sample logs' value from overriding dictionary and then wokspace
        :return: dictionary: key = sample log, value = log value
        """
        # set value dictionary from overriding
        value_dict = self._parse_overriding_logs()

        # read data from workspace
        run = self._wksp.getRun()
        if run is None:
            self.log().error("There is no Run object associated with workspace %s. " % (
                str(self._wksp)))
            return value_dict

        for il in range(len(self._sampleLogNames)):
            # get log name, check and operation type
            log_title = self._headerTitles[il]
            log_name = self._sampleLogNames[il]

            # check log name
            if log_name is None or log_name.lower() == 'none':
                # real log name is None or 'None' means overriding works
                continue
            elif log_title in value_dict:
                # overridden already: no need to read
                continue

            does_exist = run.hasProperty(log_name)
            # check whether this property does exist.
            if not does_exist:
                # If not exist and not overridden: use 0.00 as default
                self.log().warning("Sample log %s does not exist in given workspace %s."
                                   "" % (log_name, str(self._wksp)))
                value_dict[log_title] = 0.0
                continue
            else:
                log_property = run.getProperty(log_name)

            # operation type and value
            operation_type = self._sampleLogOperations[il]

            # Get log value according to type
            property_value = self._retrieve_log_value(log_property, operation_type)

            log_key = log_title
            if operation_type is not None:
                log_key = '{}-{}'.format(log_key, operation_type)
            value_dict[log_key] = property_value
        # END-FOR

        return value_dict

    #pylint: disable=too-many-branches
    def _retrieve_log_value(self, log_property, value_operation_type):
        """
        ge the log value from workspace'run object
        :param log_property:
        :param value_operation_type:
        :return:
        """
        log_class = log_property.__class__.__name__

        if log_class == "StringPropertyWithValue":
            # string
            log_value = log_property.value
            # special case for 'time'
            if value_operation_type.lower().count("time") > 0:
                log_value = self._convert_to_local_time(log_value)

        elif log_class.endswith("PropertyWithValue"):
            # single float value
            log_value = log_property.value

        elif log_class.endswith('TimeSeriesProperty'):
            # time series property
            if value_operation_type.lower() == "min":
                log_value = min(log_property.value)
            elif value_operation_type.lower() == "max":
                log_value = max(log_property.value)
            elif value_operation_type.lower() == "average":
                log_value = np.average(log_property.value)
            elif value_operation_type.lower() == "sum":
                log_value = np.sum(log_property.value)
            elif value_operation_type.lower() == "0":
                log_value = log_property.value[0]
            else:
                # unsupported TSP value operation
                raise RuntimeError("Operation {} for FloatTimeSeriesProperty {} is not supported."
                                   "".format(value_operation_type, log_property.name))
        else:
            # unsupported log type
            raise RuntimeError("Class type %d is not supported.".format(log_class))

        return log_value

    def _convert_to_local_time(self, utctimestr, addtimezone=True):
        """ Convert a UTC time in string to the local time in string
        and add
        """
        # Make certain that the input is utc time string
        utctimestr = str(utctimestr)

        # Return if time zone is UTC (no need to convert)
        if self._timezone == "UTC":
            if addtimezone is True:
                utctimestr = "%s UTC" % (utctimestr)
            return utctimestr

        # Convert
        self.log().debug("Input UTC time = %s" % (utctimestr))

        from_zone = tz.gettz('UTC')
        to_zone = tz.gettz(self._timezone)

        # Determine the parsing format
        if utctimestr.count("T") == 0:
            srctimeformat = '%Y-%m-%d %H:%M:%S.%f'
        else:
            srctimeformat = '%Y-%m-%dT%H:%M:%S.%f'

        try:
            extra = ""
            if utctimestr.count(".") == 1:
                # Time format's microsecond part %.f can take only 6 digit
                tail = utctimestr.split(".")[1]
                extralen = len(tail)-6
                if extralen > 0:
                    extra = utctimestr[-extralen:]
                    utctimestr = utctimestr[0:-extralen]
            elif utctimestr.count(".") == 0:
                # There is no .%f part in source time string:
                srctimeformat = srctimeformat.split(".")[0]
            else:
                # Un perceived situation
                raise RuntimeError("Is it possible to have time as %s?" % (utctimestr))
            utctime = datetime.datetime.strptime(utctimestr, srctimeformat)
        except ValueError as err:
            self.log().error("Unable to convert time string %s. Error message: %s" % (utctimestr, str(err)))
            raise err

        utctime = utctime.replace(tzinfo=from_zone)
        localtime = utctime.astimezone(to_zone)

        localtimestr = localtime.strftime("%Y-%m-%d %H:%M:%S.%f") + extra

        # Add time zone info
        if addtimezone is True:
            tzn = to_zone.tzname(localtime)
            localtimestr = "%s-%s" % (localtimestr, tzn)

        return localtimestr


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ExportExperimentLog)
