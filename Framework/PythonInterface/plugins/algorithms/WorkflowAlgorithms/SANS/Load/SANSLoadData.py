from mantid.kernel import Direction
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory)


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
        self.declareProperty(MatrixWorkspaceProperty('SampleTransmissionWorkspace', '', direction=Direction.Output),
                             doc='The sample transmission workspace.')

        # Sample Direct Workspace
        self.declareProperty(MatrixWorkspaceProperty('SampleDirectWorkspace', '', direction=Direction.Output),
                             doc='The sample scatter direct workspace.')

        # Can Scatter Workspaces
        self.declareProperty(MatrixWorkspaceProperty('CanScatterWorkspace', '', direction=Direction.Output),
                             doc='The can scatter workspace. This workspace does not contain monitors.')

        self.declareProperty(MatrixWorkspaceProperty('CanScatterMonitorWorkspace', '', direction=Direction.Output),
                             doc='The can scatter monitor workspace. This workspace only contains monitors.')

        # Sample Transmission Workspace
        self.declareProperty(MatrixWorkspaceProperty('CanTransmissionWorkspace', '', direction=Direction.Output),
                             doc='The can transmission workspace.')

        # Sample Direct Workspace
        self.declareProperty(MatrixWorkspaceProperty('CanDirectWorkspace', '', direction=Direction.Output),
                             doc='The sample scatter direct workspace.')

    def PyExec(self):
        # Read the file names
        sans_state = self.getProperty("SANSState").value

        # Get the correct SANSLoader from the SANSLoaderFactory

        # Run the appropriate SANSLoader for all files

        # Reset the workspaces to a zero position

        # Check if a move has been requested and perform it

        # Set output
        pass

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        state_dict = self.getProperty("SANSState").value
        try:
             state = SANSStateISIS()
             state.property_manager = state_dict
             state.validate()
        except ValueError, e:
             errors.update({"SANSStatePrototype": str(e)})
        return errors


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSLoadData)
