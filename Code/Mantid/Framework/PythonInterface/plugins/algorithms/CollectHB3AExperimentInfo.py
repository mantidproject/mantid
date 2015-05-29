#pylint: disable=no-init,invalid-name,too-many-instance-attributes
from mantid.api import *
from mantid.kernel import *
import os

class ExportSampleLogsToCSVFile(PythonAlgorithm):
    """ Python algorithm to export sample logs to spread sheet file
    for VULCAN
    """

    def category(self):
        """ Category
        """
        return "Diffraction"

    def name(self):
        """ Algorithm name
        """
        return "CollectHB3AExperimentInfo"

    def summary(self):
        return "Collect HB3A experiment information for data reduction by ConvertCWSDToMomentum."

    def PyInit(self):
        """ Declare properties
        """
        ## Input workspace
        #self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),
        #                     "Name of data workspace containing sample logs to be exported. ")

        ## Output file name
        #self.declareProperty(FileProperty("OutputFilename", "", FileAction.Save, [".txt"]),
        #                     "Name of the output sample environment log file name.")

        ## Sample log names
        #self.declareProperty(StringArrayProperty("SampleLogNames", values=[], direction=Direction.Input),
        #                     "Names of sample logs to be exported in a same file.")

        ## Header
        #self.declareProperty("WriteHeaderFile", False, "Flag to generate a sample log header file.")

        #self.declareProperty("Header", "", "String in the header file.")

        ## Time zone
        #timezones = ["UTC", "America/New_York", "Asia/Shanghai", "Australia/Sydney", "Europe/London", "GMT+0",\
        #        "Europe/Paris", "Europe/Copenhagen"]

        #description = "Sample logs recorded in NeXus files (in SNS) are in UTC time.  TimeZone " + \
        #    "can allow the algorithm to output the log with local time."
        #self.declareProperty("TimeZone", "America/New_York", StringListValidator(timezones), description)

        ## Log time tolerance
        #self.declareProperty("TimeTolerance", 0.01,
        #                     "If any 2 log entries with log times within the time tolerance, " + \
        #                     "they will be recorded in one line. Unit is second. ")

        return


    def PyExec(self):
        """ Main executor
        """
        # Read inputs
        self._getProperties()



# Register algorithm with Mantid
AlgorithmFactory.subscribe(CollectHB3AExperimentInfo)
