from mantid.kernel import Direction
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory)
from SANS2.State.SANSStateSerializer import create_deserialized_sans_state_from_property_manager
from SANS.Load.SANSLoadData import SANSLoadDataFactory
from SANS2.State.SANSStateData import SANSDataType


class SANSLoad(DataProcessorAlgorithm):

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
        # Read the state
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)
        
        # Get the correct SANSLoader from the SANSLoaderFactory
        load_factory = SANSLoadDataFactory()
        loader = load_factory.create_loader(state)

        # Run the appropriate SANSLoader and get the workspaces and the workspace monitors
        use_cached = self.getProperty("UseCached").value
        publish_to_ads = self.getProperty("PublishToCache").value
        data = state.data
        workspaces, workspace_monitors = loader.execute(data_info=data, use_cached=use_cached,
                                                        publish_to_ads=publish_to_ads)

        # Check if a move has been requested and perform it
        move_workspaces = self.getProperty("MoveWorkspace").value
        if move_workspaces:
            # TODO: Implement the move option
            pass

        # Set output workspaces
        for workspace_type, workspace in workspaces.iteritems():
            self.set_output_for_workspaces(workspace_type, workspace)

        # Set the output monitor workspaces
        for workspace_type, workspace in workspace_monitors.iteritems():
            self.set_output_for_monitor_workspaces(workspace_type, workspace)

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        state_property_manager = self.getProperty("SANSState").value
        try:
            state = create_deserialized_sans_state_from_property_manager(state_property_manager)
            state.property_manager = state_property_manager
            state.validate()
        except ValueError, e:
            errors.update({"SANSStatePrototype": str(e)})
        return errors

    def set_output_for_workspaces(self, workspace_type, workspace):
        if workspace_type == SANSDataType.SampleScatter:
            self.setProperty("SampleScatterWorkspace", workspace)
        elif workspace_type == SANSDataType.SampleTransmission:
            self.setProperty("SampleTransmissionWorkspace", workspace)
        elif workspace_type == SANSDataType.SampleDirect:
            self.setProperty("SampleDirectWorkspace", workspace)
        elif workspace_type == SANSDataType.CanScatter:
            self.setProperty("CanScatterWorkspace", workspace)
        elif workspace_type == SANSDataType.CanTransmission:
            self.setProperty("CanTransmissionWorkspace", workspace)
        elif workspace_type == SANSDataType.CanDirect:
            self.setProperty("CanDirectWorkspace", workspace)
        else:
            raise RuntimeError("SANSLoad: Unknown data output workspace format: {}".format(str(workspace_type)))

    def set_output_for_monitor_workspaces(self, workspace_type, workspace):
        if workspace_type == SANSDataType.SampleScatter:
            self.setProperty("SampleScatterMonitorWorkspace", workspace)
        elif workspace_type == SANSDataType.CanScatter:
            self.setProperty("CanScatterMonitorWorkspace", workspace)
        else:
            raise RuntimeError("SANSLoad: Unknown data output workspace format: {}".format(str(workspace_type)))

# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSLoad)
