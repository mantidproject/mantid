from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty    # , WorkspaceUnitValidator
from mantid.kernel import Direction
from mantid.simpleapi import CropWorkspace


class TOFTOFCropWorkspace(PythonAlgorithm):
    """ Crop empty time channels
    """
    def __init__(self):
        PythonAlgorithm.__init__(self)

    def category(self):
        """ Return category
        """
        return "PythonAlgorithms\\MLZ\\TOFTOF;Utility"

    def name(self):
        """ Return summary
        """
        return "TOFTOFCropWorkspace"

    def summary(self):
        return "Crop empty time channels."

    def PyInit(self):
        """ Declare properties
        """
        # better would be to use the validator, but it fails if WorkspaceGroup is given as an input
        # self.declareProperty(WorkspaceProperty("InputWorkspace", "", direction=Direction.Input,
        #                                       validator=WorkspaceUnitValidator('TOF')),
        #                     doc="Input workspace.")
        self.declareProperty(WorkspaceProperty("InputWorkspace", "", direction=Direction.Input),
                             doc="Input workspace.")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
                             doc="Name of the workspace that will contain the results")
        return

    def PyExec(self):
        """ Main execution body
        """
        inputws = self.getProperty("InputWorkspace").value
        # check X units, will be not needed if validator will work
        xunit = inputws.getAxis(0).getUnit().unitID()
        if xunit != 'TOF':
            message = "Workspace " + inputws.getName() + " has invalid X axis units " + str(xunit) +\
                ". X axis units must be TOF."
            self.log().error(message)
            raise ValueError(message)

        outputws = self.getProperty("OutputWorkspace").value

        # check for required properties
        run = inputws.getRun()
        if run.hasProperty('channel_width') and run.hasProperty('full_channels'):
            channel_width = float(run.getLogData('channel_width').value)
            full_channels = float(run.getLogData('full_channels').value)
        else:
            message = "Workspace " + inputws.getName() + " does not contain required sample logs. Cannot crop."
            self.log().error("message")
            raise RuntimeError(message)

        outputws = CropWorkspace(inputws, XMin=0., XMax=full_channels*channel_width, OutputWorkspace=outputws)
        self.setProperty("OutputWorkspace", outputws)


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(TOFTOFCropWorkspace)
