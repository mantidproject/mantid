#from mantid.api import PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty, WorkspaceFactory, FileProperty, FileAction
#from mantid.kernel import Direction, StringListValidator, FloatBoundedValidator

import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
from mantid.api import AnalysisDataService

_OUTPUTLEVEL = "NOOUTPUT"

class CreateLeBailFitInput(PythonAlgorithm):
    """ Create the input TableWorkspaces for LeBail Fitting
    """
    def category(self):
        """
        """
        return "Utility;Text"


    def name(self):
        """
        """
        return "LoadSPICEAscii"

    def summary(self):
        return "Load data file generate from SPICE in ASCII format to table workspaces."

    def PyInit(self):
        """ Declare properties
        """
        self.declareProperty(FileProperty("Filename", "", FileAction.Load, ['.dat']),
                "Name of SPICE data file.")

        self.declareProperty(ITableWorkspaceProperty("OutputWorkspace", "", Direction.Output),
                "Name of TableWorkspace containing experimental data.")

        return

    def PyExec(self):
        """ Main Execution Body
        """
        # Input
        filename = self.getPropertyValue("Filename")

	# Parse 
	scandict, runinfodict = self.parseSPICEAscii(filename)

	# Build output workspaces
	outws = self.createDataWS(scandict)

	# Build run information workspace
	runinfows = self.createRunInfoWS(runinfodict)

	# Set properties
        self.setProperty("OutputWorkspace", outws)

        return


    def parseSPICEAscii(self, filename):
        """ 
        """
	# TODO - Implement
	raise NotImplementedError("Implement ASAP")

    
    def createDataWS(self, datadict):
	"""
	"""
        # Create an empty workspace
        tablews = WorkspaceFactory.createTable()

        tablews.addColumn("int",    "Scan")

	# TODO - Implement

        return tablews


# Register algorithm with Mantid
AlgorithmFactory.subscribe(LoadSPICEAscii)
