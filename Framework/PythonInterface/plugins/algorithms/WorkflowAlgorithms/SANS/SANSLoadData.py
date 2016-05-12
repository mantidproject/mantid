#pylint: disable=no-init
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *


class SANSLoadData(DataProcessorAlgorithm):

    def category(self):
        return 'SANS\\Load'

    def summary(self):
        return 'Load SANS data'

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        self.declareProperty('SANSState', '', direction=Direction.In,
                             doc='A property manager which fulfills the SANSState contract.')

        self.declareProperty("PublishToCache", True,
                             "Publish the loaded files to a cache, in order to avoid reloading for subsequent runs.")

        self.declareProperty("UseCached", True,
                             "Checks if there are loaded files available. If they are, those files are used.")

        self.declareProperty("MoveWorkspace", False,
                             "Move the workspace according to the SANSState setting. This might be useful"
                             "for manual inspection.")

        # ------------
        #  OUTPUT
        # ------------
        # Sample Scatter Workspaces
        self.declareProperty(MatrixWorkspaceProperty('SampleScatterWorkspace', '', direction=Direction.Output),
                             doc='The sample scatter workspace. This workspace does not contain monitors.')
        self.declareProperty(MatrixWorkspaceProperty('SampleScatterMonitorWorkspace', '', direction=Direction.Output),
                             doc='The sample scatter monitor workspace. This workspace only contains monitors.')

        # Sample Transmission Workspace

        # Sample Direct Workspace

        # Can Scatter Workspaces

        # Can Transmission Workspace

        # Can Direct Workspace

    def PyExec(self):
        # Read the file names

        # Get the correct SANSLoader from the SANSLoaderFactory

        # Run the appropriate SANSLoader for scatter files

        # Run the appropriate SANSLoader for the transmission/direct files

        # Set output
        pass

    def validateInputs(self):
        issues = dict()
        return issues


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSLoad)
