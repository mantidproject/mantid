from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty
import mantid.simpleapi
from mantid.kernel import Direction, logger

class CheckForSampleLogs(PythonAlgorithm):
    """ Check if certain sample logs exists on a workspace
    """
    def category(self):
        """ Return category
        """
        return "PythonAlgorithms;Utility\\Workspaces"

    def name(self):
        """ Return name
        """
        return "CheckForSampleLogs"

    def summary(self):
        return "Check if the workspace has some given sample logs"

    def PyInit(self):
        """ Declare properties
        """
        self.declareProperty(WorkspaceProperty("Workspace", "",Direction.Input), "The workspace to check.")
        self.declareProperty("LogNames","","Names of the logs to look for")
        self.declareProperty("Result","A string that will be empty if all the logs are found, otherwise will contain an error message",Direction.Output)
        return

    def PyExec(self):
        """ Main execution body
        """
        #get parameters
        w = self.getProperty("Workspace").value
        logNames = self.getProperty("LogNames").value
        resultString=''
        #check for parameters and build the result string
        for value in logNames.split(','):
            value=value.strip()
            if len(value)>0:
                if not w.run().hasProperty(value):
                    resultString+='Property '+value+' not found\n'

        #return the result
        logger.notice(resultString)
        self.setProperty("Result",resultString)
        return


AlgorithmFactory.subscribe(CheckForSampleLogs)
