from __future__ import (absolute_import, division, print_function)

from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty    # , WorkspaceUnitValidator
from mantid.kernel import Direction
import mantid.simpleapi as api


class TOFTOFCropWorkspace(PythonAlgorithm):
    """ Crop empty time channels
    """

    def __init__(self):
        PythonAlgorithm.__init__(self)

    def category(self):
        """ Return category
        """
        return "Workflow\\MLZ\\TOFTOF;Transforms\\Splitting"

    def seeAlso(self):
        return [ "TOFTOFMergeRuns","CorrectTOF" ]

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

    def validateInputs(self):
        issues = dict()
        input_workspace = self.getProperty("InputWorkspace").value

        xunit = input_workspace.getAxis(0).getUnit().unitID()
        if xunit != 'TOF':
            issues['InputWorkspace'] = "X axis units must be TOF. "

        # check for required properties
        run = input_workspace.getRun()
        if not run.hasProperty('channel_width'):
            issues['InputWorkspace'] = "Input workpsace must have sample log channel_width."
        if not run.hasProperty('full_channels'):
            issues['InputWorkspace'] = "Input workpsace must have sample log full_channels."
        if not run.hasProperty('TOF1'):
            issues['InputWorkspace'] = "Input workpsace must have sample log TOF1."

        return issues

    def PyExec(self):
        """ Main execution body
        """
        inputws = self.getProperty("InputWorkspace").value

        run = inputws.getRun()
        channel_width = float(run.getLogData('channel_width').value)
        full_channels = float(run.getLogData('full_channels').value)
        tof1 = float(run.getLogData('TOF1').value)

        outputws = api.CropWorkspace(inputws, XMin=0., XMax=full_channels*channel_width + tof1, StoreInADS=False)
        self.setProperty("OutputWorkspace", outputws)


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(TOFTOFCropWorkspace)
