"""*WIKI*

Algorithm ExportExperimentLog obtains run information, sample information and 
sample log information from a MatrixWorkspace and write them to a csv file. 

== File Mode ==
There are 3 modes to write the experiment log file. 
1. "new": A new file will be created with header line;
2. "appendfast": A line of experiment log information will be appended to an existing file; 
** It is assumed that the log names given are exactly same as those in the file, as well as their order;
** Input property ''SampleLogTitles'' will be ignored in this option; 
3. "append": A line of experiment log information will be appended to an existing file;
** The algorithm will check whether the specified log file names, titles and their orders are exactly same as those in the file to append to;
** If any difference is deteced, the old file will be renamed in the same directory.  And a new file will be generated. 

== Missing Sample Logs ==
If there is any sample log specified in the properites but does not exist in the workspace, 
a string as 'XXXX' will be put to the experiment log information line, because 
there is no sufficient information about its default. 

== Sample Log Operation ==
There are 5 types of operations that are supported for a TimeSeriesProperty.  
1. "min": minimum TimeSeriesProperty's values;
2. "max": maximum TimeSeriesProperty's values; 
3. "average": average of TimeSeriesProperty's values;
4. "sum": summation of TimeSeriesProperty's values;
5. "0": first value of TimeSeriesProperty's value. 

*WIKI*"""
import mantid.simpleapi as api
import mantid
from mantid.api import *
from mantid.kernel import *

class ExportExperimentLog(PythonAlgorithm):
    """ Algorithm to export experiment log
    """
    def PyInit(self):
        """ Declaration of properties
        """
        self.setWikiSummary("Export experimental log.")
        self.setOptionalMessage("Export experimental log.")
        
        # wsprop = mantid.api.MatrixWorkspaceProperty("InputWorkspace", "", mantid.kernel.Direction.Input)
        wsprop = MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input)
        self.declareProperty(wsprop, "Input workspace containing the sample log information. ")
        
        self.declareProperty(FileProperty("OutputFilename","", FileAction.Save, ['.txt']),
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
        """
        if self._filemode == "append":
            match = self._checkTitleMatch(self._logfilename, self._headerTitles)
            if match is False:
                self._renameLogFile(self._logfilename)
                self._filemode = "new"
        """
            
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
                wbuf += "\t"
        wbuf += "\n"
            
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
            wbuf += "%s\t" % (str(value))
        wbuf = wbuf[0:-1]

        # Append to file
        lfile = open(self._logfilename, "a")
        lfile.write("%s\n" %(wbuf))
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
                # If not exist, use XXXX as default
                self.log().warning("Sample log %s does not exist in given workspace %s. " % (logname, str(self._wksp)))
                valuedict[logname] = "XXXX"
                continue

            logproperty = run.getProperty(logname)
            logclass = logproperty.__class__.__name__ 

            # Get log value according to type
            if logclass == "StringPropertyWithValue":
                propertyvalue = logproperty.value
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
                    raise NotImplementedError("Operation %s is for FloatTimeSeriesProperty is not supported." % (operationtype))
            else:
                raise NotImplementedError("Class type %d is not supported." % (logclass))
            
            valuedict[logname] = propertyvalue
        # ENDFOR
        
        return valuedict
        
        
        
# Register algorithm with Mantid
AlgorithmFactory.subscribe(ExportExperimentLog)
