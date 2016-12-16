# pylint: disable=invalid-name

""" SANSLoad algorithm which handles loading SANS files"""

from mantid.kernel import (Direction, PropertyManagerProperty, FloatArrayProperty,
                           EnabledWhenProperty, PropertyCriterion)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode)

from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager
from SANS2.State.SANSStateData import SANSDataType
from SANS2.Common.SANSFunctions import create_unmanaged_algorithm
from SANS.Load.SANSLoadData import SANSLoadDataFactory


class SANSLoad(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Load'

    def summary(self):
        return 'Load SANS data'

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        self.declareProperty("PublishToCache", True, direction=Direction.Input,
                             doc="Publish the loaded files to a cache, in order to avoid reloading "
                                 "for subsequent runs.")

        self.declareProperty("UseCached", True, direction=Direction.Input,
                             doc="Checks if there are loaded files available. If they are, those files are used.")

        self.declareProperty("MoveWorkspace", defaultValue=False, direction=Direction.Input,
                             doc="Move the workspace according to the SANSState setting. This might be useful"
                             "for manual inspection.")

        # Beam coordinates if an initial move of the workspace is requested
        enabled_condition = EnabledWhenProperty("MoveWorkspace", PropertyCriterion.IsNotDefault)
        self.declareProperty(FloatArrayProperty(name='BeamCoordinates', values=[]),
                             doc='The coordinates which is used to position the instrument component(s). '
                                 'If the workspaces should be loaded with an initial move, then this '
                                 'needs to be specified')
        self.setPropertySettings("BeamCoordinates", enabled_condition)

        # ------------
        #  OUTPUT
        # ------------
        default_number_of_workspaces = 0

        # Sample Scatter Workspaces
        self.declareProperty(MatrixWorkspaceProperty('SampleScatterWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The sample scatter workspace. This workspace does not contain monitors.')
        self.declareProperty(MatrixWorkspaceProperty('SampleScatterMonitorWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The sample scatter monitor workspace. This workspace only contains monitors.')
        self.declareProperty('NumberOfSampleScatterWorkspaces', defaultValue=default_number_of_workspaces,
                             direction=Direction.Output,
                             doc='The number of workspace for sample scatter.')

        # Sample Transmission Workspace
        self.declareProperty(MatrixWorkspaceProperty('SampleTransmissionWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The sample transmission workspace.')
        self.declareProperty('NumberOfSampleTransmissionWorkspaces', defaultValue=default_number_of_workspaces,
                             direction=Direction.Output,
                             doc='The number of workspace for sample transmission.')

        # Sample Direct Workspace
        self.declareProperty(MatrixWorkspaceProperty('SampleDirectWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The sample scatter direct workspace.')
        self.declareProperty('NumberOfSampleDirectWorkspaces', defaultValue=default_number_of_workspaces,
                             direction=Direction.Output,
                             doc='The number of workspace for sample direct.')

        self.setPropertyGroup("SampleScatterWorkspace", 'Sample')
        self.setPropertyGroup("SampleScatterMonitorWorkspace", 'Sample')
        self.setPropertyGroup("SampleTransmissionWorkspace", 'Sample')
        self.setPropertyGroup("SampleDirectWorkspace", 'Sample')

        # Can Scatter Workspaces
        self.declareProperty(MatrixWorkspaceProperty('CanScatterWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can scatter workspace. This workspace does not contain monitors.')
        self.declareProperty(MatrixWorkspaceProperty('CanScatterMonitorWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can scatter monitor workspace. This workspace only contains monitors.')
        self.declareProperty('NumberOfCanScatterWorkspaces', defaultValue=default_number_of_workspaces,
                             direction=Direction.Output,
                             doc='The number of workspace for can scatter.')

        # Sample Transmission Workspace
        self.declareProperty(MatrixWorkspaceProperty('CanTransmissionWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can transmission workspace.')
        self.declareProperty('NumberOfCanTransmissionWorkspaces', defaultValue=default_number_of_workspaces,
                             direction=Direction.Output,
                             doc='The number of workspace for can transmission.')

        # Sample Direct Workspace
        self.declareProperty(MatrixWorkspaceProperty('CanDirectWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The sample scatter direct workspace.')
        self.declareProperty('NumberOfCanDirectWorkspaces', defaultValue=default_number_of_workspaces,
                             direction=Direction.Output,
                             doc='The number of workspace for can direct.')

        self.setPropertyGroup("CanScatterWorkspace", 'Can')
        self.setPropertyGroup("CanScatterMonitorWorkspace", 'Can')
        self.setPropertyGroup("CanTransmissionWorkspace", 'Can')
        self.setPropertyGroup("CanDirectWorkspace", 'Can')

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

        # Check if a move has been requested and perform it. This can be useful if scientists want to load the data and
        # have it moved in order to inspect it with other tools
        move_workspaces = self.getProperty("MoveWorkspace").value
        if move_workspaces:
            self._perform_initial_move(workspaces, state)

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
        except ValueError as err:
            errors.update({"SANSLoad": str(err)})
        return errors

    def set_output_for_workspaces(self, workspace_type, workspaces):
        if workspace_type == SANSDataType.SampleScatter:
            self.set_property_with_number_of_workspaces("SampleScatterWorkspace", workspaces)
        elif workspace_type == SANSDataType.SampleTransmission:
            self.set_property_with_number_of_workspaces("SampleTransmissionWorkspace", workspaces)
        elif workspace_type == SANSDataType.SampleDirect:
            self.set_property_with_number_of_workspaces("SampleDirectWorkspace", workspaces)
        elif workspace_type == SANSDataType.CanScatter:
            self.set_property_with_number_of_workspaces("CanScatterWorkspace", workspaces)
        elif workspace_type == SANSDataType.CanTransmission:
            self.set_property_with_number_of_workspaces("CanTransmissionWorkspace", workspaces)
        elif workspace_type == SANSDataType.CanDirect:
            self.set_property_with_number_of_workspaces("CanDirectWorkspace", workspaces)
        else:
            raise RuntimeError("SANSLoad: Unknown data output workspace format: {0}".format(str(workspace_type)))

    def set_output_for_monitor_workspaces(self, workspace_type, workspaces):
        if workspace_type == SANSDataType.SampleScatter:
            self.set_property("SampleScatterMonitorWorkspace", workspaces)
        elif workspace_type == SANSDataType.CanScatter:
            self.set_property("CanScatterMonitorWorkspace", workspaces)
        else:
            raise RuntimeError("SANSLoad: Unknown data output workspace format: {0}".format(str(workspace_type)))

    def set_property(self, name, workspace_collection):
        """
        We receive a name for a property and a collection of workspaces. If the workspace is a group workspace, then
        we dynamically create output properties and inform the user that he needs to query the output workspaces
        individually and we need to communicate how many there are.
        :param name: The name of the output property
        :param workspace_collection: A list of workspaces which corresponds to the name. Note that normally there
                                    there will be only one element in this list. Only when dealing with multiperiod
                                    data can we expected to see more workspaces in the list.
        """
        self.setProperty(name, workspace_collection[0])
        number_of_workspaces = 1
        if len(workspace_collection) > 1:
            counter = 1
            for workspace in workspace_collection:
                output_name = name + "_" + str(counter)
                self.declareProperty(MatrixWorkspaceProperty(output_name, '',
                                                             optional=PropertyMode.Optional,
                                                             direction=Direction.Output),
                                     doc='A child workspace of a multi-period file.')
                self.setProperty(output_name, workspace)
                counter += 1
            number_of_workspaces = counter - 1
        return number_of_workspaces

    def set_property_with_number_of_workspaces(self, name, workspace_collection):
        counter = self.set_property(name, workspace_collection)
        # The property name for the number of workspaces
        number_of_workspaces_name = "NumberOf" + name + "s"
        self.setProperty(number_of_workspaces_name, counter)

    def _perform_initial_move(self, workspaces, state):
        beam_coordinates = self.getProperty("BeamCoordinates").value
        move_name = "SANSMove"
        state_dict = state.property_manager
        move_options = {"SANSState": state_dict,
                        "BeamCoordinates": beam_coordinates,
                        "MoveType": "InitialMove"}
        move_alg = create_unmanaged_algorithm(move_name, **move_options)

        for workspace in workspaces:
            move_alg.setProperty("Workspace", workspace)
            move_alg.execute()


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSLoad)
