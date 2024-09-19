# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

"""SANSLoad algorithm which handles loading SANS files"""

from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress, WorkspaceProperty
from mantid.kernel import Direction, FloatArrayProperty
from sans.algorithm_detail.load_data import SANSLoadDataFactory
from sans.algorithm_detail.move_sans_instrument_component import move_component, MoveTypes
from SANS.sans.common.enums import SANSDataType

from sans.state.Serializer import Serializer


class SANSLoad(DataProcessorAlgorithm):
    def category(self):
        return "SANS\\Load"

    def summary(self):
        return "Load SANS data"

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        self.declareProperty("SANSState", "", doc="A JSON String which fulfills the SANSState contract.")

        self.declareProperty(
            "PublishToCache",
            True,
            direction=Direction.Input,
            doc="Publish the calibration workspace to a cache, in order to avoid reloading " "for subsequent runs.",
        )

        self.declareProperty(
            "UseCached",
            True,
            direction=Direction.Input,
            doc="Checks if there are loaded files available. If they are, those files are used.",
        )

        # Beam coordinates if an initial move of the workspace is requested
        self.declareProperty(
            FloatArrayProperty(name="BeamCoordinates", values=[]),
            doc="The coordinates which is used to position the instrument component(s). "
            "If the workspaces should be loaded with an initial move, then this "
            "needs to be specified.",
        )
        # Components which are to be moved
        self.declareProperty(
            "Component",
            "",
            direction=Direction.Input,
            doc="Component that should be moved. "
            "If the workspaces should be loaded with an initial move, then this "
            "needs to be specified.",
        )

        # ------------
        #  OUTPUT
        # ------------
        default_number_of_workspaces = 0

        # Sample Scatter Workspaces
        self.declareProperty(
            WorkspaceProperty("SampleScatterWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The sample scatter workspace. This workspace does not contain monitors.",
        )
        self.declareProperty(
            WorkspaceProperty("SampleScatterMonitorWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The sample scatter monitor workspace. This workspace only contains monitors.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("SampleTransmissionWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The sample transmission workspace.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("SampleDirectWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The sample scatter direct workspace.",
        )

        self.setPropertyGroup("SampleScatterWorkspace", "Sample")
        self.setPropertyGroup("SampleScatterMonitorWorkspace", "Sample")
        self.setPropertyGroup("SampleTransmissionWorkspace", "Sample")
        self.setPropertyGroup("SampleDirectWorkspace", "Sample")

        # Number of sample workspaces
        self.declareProperty(
            "NumberOfSampleScatterWorkspaces",
            defaultValue=default_number_of_workspaces,
            direction=Direction.Output,
            doc="The number of workspace for sample scatter.",
        )
        self.declareProperty(
            "NumberOfSampleTransmissionWorkspaces",
            defaultValue=default_number_of_workspaces,
            direction=Direction.Output,
            doc="The number of workspace for sample transmission.",
        )
        self.declareProperty(
            "NumberOfSampleDirectWorkspaces",
            defaultValue=default_number_of_workspaces,
            direction=Direction.Output,
            doc="The number of workspace for sample direct.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("CanScatterWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can scatter workspace. This workspace does not contain monitors.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("CanScatterMonitorWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can scatter monitor workspace. This workspace only contains monitors.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("CanTransmissionWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can transmission workspace.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("CanDirectWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The sample scatter direct workspace.",
        )
        self.setPropertyGroup("CanScatterWorkspace", "Can")
        self.setPropertyGroup("CanScatterMonitorWorkspace", "Can")
        self.setPropertyGroup("CanTransmissionWorkspace", "Can")
        self.setPropertyGroup("CanDirectWorkspace", "Can")

        self.declareProperty(
            "NumberOfCanScatterWorkspaces",
            defaultValue=default_number_of_workspaces,
            direction=Direction.Output,
            doc="The number of workspace for can scatter.",
        )
        self.declareProperty(
            "NumberOfCanTransmissionWorkspaces",
            defaultValue=default_number_of_workspaces,
            direction=Direction.Output,
            doc="The number of workspace for can transmission.",
        )
        self.declareProperty(
            "NumberOfCanDirectWorkspaces",
            defaultValue=default_number_of_workspaces,
            direction=Direction.Output,
            doc="The number of workspace for can direct.",
        )

    def PyExec(self):
        # Read the state
        state_property_manager = self.getProperty("SANSState").value
        state = Serializer.from_json(state_property_manager)

        # Run the appropriate SANSLoader and get the workspaces and the workspace monitors
        # Note that cache optimization is only applied to the calibration workspace since it is not available as a
        # return property and it is also something which is most likely not to change between different reductions.
        use_cached = self.getProperty("UseCached").value
        publish_to_ads = self.getProperty("PublishToCache").value

        data = state.data
        state_adjustment = state.adjustment
        progress = self._get_progress_for_file_loading(data, state_adjustment)

        # Get the correct SANSLoader from the SANSLoaderFactory
        load_factory = SANSLoadDataFactory()
        loader = load_factory.create_loader(state)

        workspaces, workspace_monitors = loader.execute(
            data_info=data,
            use_cached=use_cached,
            publish_to_ads=publish_to_ads,
            progress=progress,
            parent_alg=self,
            adjustment_info=state.adjustment,
        )
        progress.report("Loaded the data.")

        progress_move = Progress(self, start=0.8, end=1.0, nreports=2)
        progress_move.report("Starting to move the workspaces.")
        self._perform_initial_move(workspaces, state)
        progress_move.report("Finished moving the workspaces.")

        # Set output workspaces
        for workspace_type, workspace in workspaces.items():
            self.set_output_for_workspaces(workspace_type, workspace)

        # Set the output monitor workspaces
        for workspace_type, workspace in workspace_monitors.items():
            self.set_output_for_monitor_workspaces(workspace_type, workspace)

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        state_json = self.getProperty("SANSState").value
        try:
            state = Serializer.from_json(state_json)
            state.validate()
        except ValueError as err:
            errors.update({"SANSState": str(err)})
            return errors

        # We need to validate that the for each expected output workspace of the SANSState a output workspace name
        # was supplied in the PyInit
        # For sample scatter
        sample_scatter = self.getProperty("SampleScatterWorkspace").value
        sample_scatter_as_string = self.getProperty("SampleScatterWorkspace").valueAsStr
        if sample_scatter is None and not sample_scatter_as_string:
            errors.update({"SampleScatterWorkspace": "A sample scatter output workspace needs to be specified."})

        # For sample scatter monitor
        sample_scatter_monitor = self.getProperty("SampleScatterMonitorWorkspace").value
        sample_scatter_monitor_as_string = self.getProperty("SampleScatterMonitorWorkspace").valueAsStr
        if sample_scatter_monitor is None and not sample_scatter_monitor_as_string:
            errors.update({"SampleScatterMonitorWorkspace": "A sample scatter output workspace needs to be specified."})

        # ------------------------------------
        # Check the optional output workspaces
        # If they are specified in the SANSState, then we require them to be set on the output as well.
        data_info = state.data

        # For sample transmission
        sample_transmission = self.getProperty("SampleTransmissionWorkspace").value
        sample_transmission_as_string = self.getProperty("SampleTransmissionWorkspace").valueAsStr
        sample_transmission_was_set = sample_transmission is not None or len(sample_transmission_as_string) > 0

        sample_transmission_from_state = data_info.sample_transmission
        if not sample_transmission_was_set and sample_transmission_from_state is not None:
            errors.update(
                {
                    "SampleTransmissionWorkspace": "You need to set the output for the sample transmission"
                    " workspace since it is specified to be loaded in your "
                    "reduction configuration."
                }
            )
        if sample_transmission_was_set and sample_transmission_from_state is None:
            errors.update(
                {
                    "SampleTransmissionWorkspace": "You set an output workspace for sample transmission, "
                    "although none is specified in the reduction configuration."
                }
            )

        # For sample direct
        sample_direct = self.getProperty("SampleDirectWorkspace").value
        sample_direct_as_string = self.getProperty("SampleDirectWorkspace").valueAsStr
        sample_direct_was_set = sample_direct is not None or len(sample_direct_as_string) > 0

        sample_direct_from_state = data_info.sample_direct
        if not sample_direct_was_set and sample_direct_from_state is not None:
            errors.update(
                {
                    "SampleDirectWorkspace": "You need to set the output for the sample direct"
                    " workspace since it is specified to be loaded in your "
                    "reduction configuration."
                }
            )
        if sample_direct_was_set and sample_direct_from_state is None:
            errors.update(
                {
                    "SampleDirectWorkspace": "You set an output workspace for sample direct, "
                    "although none is specified in the reduction configuration."
                }
            )

        # For can scatter + monitor
        can_scatter = self.getProperty("CanScatterWorkspace").value
        can_scatter_as_string = self.getProperty("CanScatterWorkspace").valueAsStr
        can_scatter_was_set = can_scatter is not None or len(can_scatter_as_string) > 0

        can_scatter_from_state = data_info.can_scatter
        if not can_scatter_was_set and can_scatter_from_state is not None:
            errors.update(
                {
                    "CanScatterWorkspace": "You need to set the output for the can scatter"
                    " workspace since it is specified to be loaded in your "
                    "reduction configuration."
                }
            )
        if can_scatter_was_set and can_scatter_from_state is None:
            errors.update(
                {
                    "CanScatterWorkspace": "You set an output workspace for can scatter, "
                    "although none is specified in the reduction configuration."
                }
            )

        # For can scatter monitor
        can_scatter_monitor = self.getProperty("CanScatterMonitorWorkspace").value
        can_scatter_monitor_as_string = self.getProperty("CanScatterMonitorWorkspace").valueAsStr
        can_scatter_monitor_was_set = can_scatter_monitor is not None or len(can_scatter_monitor_as_string) > 0
        if not can_scatter_monitor_was_set and can_scatter_from_state is not None:
            errors.update(
                {
                    "CanScatterMonitorWorkspace": "You need to set the output for the can scatter monitor"
                    " workspace since it is specified to be loaded in your "
                    "reduction configuration."
                }
            )
        if can_scatter_monitor_was_set and can_scatter_from_state is None:
            errors.update(
                {
                    "CanScatterMonitorWorkspace": "You set an output workspace for can scatter monitor, "
                    "although none is specified in the reduction configuration."
                }
            )

        # For sample transmission
        can_transmission = self.getProperty("CanTransmissionWorkspace").value
        can_transmission_as_string = self.getProperty("CanTransmissionWorkspace").valueAsStr
        can_transmission_was_set = can_transmission is not None or len(can_transmission_as_string) > 0
        can_transmission_from_state = data_info.can_transmission
        if not can_transmission_was_set and can_transmission_from_state is not None:
            errors.update(
                {
                    "CanTransmissionWorkspace": "You need to set the output for the can transmission"
                    " workspace since it is specified to be loaded in your "
                    "reduction configuration."
                }
            )
        if can_transmission_was_set and can_transmission_from_state is None:
            errors.update(
                {
                    "CanTransmissionWorkspace": "You set an output workspace for can transmission, "
                    "although none is specified in the reduction configuration."
                }
            )

        # For can direct
        can_direct = self.getProperty("CanDirectWorkspace").value
        can_direct_as_string = self.getProperty("CanDirectWorkspace").valueAsStr
        can_direct_was_set = can_direct is not None or len(can_direct_as_string) > 0
        can_direct_from_state = data_info.can_direct
        if not can_direct_was_set and can_direct_from_state is not None:
            errors.update(
                {
                    "CanDirectWorkspace": "You need to set the output for the can direct"
                    " workspace since it is specified to be loaded in your "
                    "reduction configuration."
                }
            )
        if can_direct_was_set and can_direct_from_state is None:
            errors.update(
                {
                    "CanDirectWorkspace": "You set an output workspace for can direct, "
                    "although none is specified in the reduction configuration."
                }
            )
        return errors

    def set_output_for_workspaces(self, workspace_type, workspaces):
        if workspace_type is SANSDataType.SAMPLE_SCATTER:
            self.set_property_with_number_of_workspaces("SampleScatterWorkspace", workspaces)
        elif workspace_type is SANSDataType.SAMPLE_TRANSMISSION:
            self.set_property_with_number_of_workspaces("SampleTransmissionWorkspace", workspaces)
        elif workspace_type is SANSDataType.SAMPLE_DIRECT:
            self.set_property_with_number_of_workspaces("SampleDirectWorkspace", workspaces)
        elif workspace_type is SANSDataType.CAN_SCATTER:
            self.set_property_with_number_of_workspaces("CanScatterWorkspace", workspaces)
        elif workspace_type is SANSDataType.CAN_TRANSMISSION:
            self.set_property_with_number_of_workspaces("CanTransmissionWorkspace", workspaces)
        elif workspace_type is SANSDataType.CAN_DIRECT:
            self.set_property_with_number_of_workspaces("CanDirectWorkspace", workspaces)
        else:
            raise RuntimeError("SANSLoad: Unknown data output workspace format: {0}".format(str(workspace_type)))

    def set_output_for_monitor_workspaces(self, workspace_type, workspaces):
        if workspace_type is SANSDataType.SAMPLE_SCATTER:
            self.set_property("SampleScatterMonitorWorkspace", workspaces)
        elif workspace_type is SANSDataType.CAN_SCATTER:
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
        if len(workspace_collection) > 1:
            # Note that the first output is the same as we have set above.
            counter = 1
            for workspace in workspace_collection:
                output_name = name + "_" + str(counter)
                self.declareProperty(
                    MatrixWorkspaceProperty(output_name, "", optional=PropertyMode.Optional, direction=Direction.Output),
                    doc="A child workspace of a multi-period file.",
                )
                # We need to set a name on here if one was set
                user_specified_name = self.getProperty(name).valueAsStr
                if user_specified_name:
                    user_specified_name += "_" + str(counter)
                    self.setProperty(output_name, user_specified_name)
                self.setProperty(output_name, workspace)
                counter += 1
        else:
            self.setProperty(name, workspace_collection[0])
        return len(workspace_collection)

    def set_property_with_number_of_workspaces(self, name, workspace_collection):
        counter = self.set_property(name, workspace_collection)
        # The property name for the number of workspaces
        number_of_workspaces_name = "NumberOf" + name + "s"
        self.setProperty(number_of_workspaces_name, counter)

    def _perform_initial_move(self, workspaces, state):
        # If beam centre was specified then use it
        beam_coordinates = self.getProperty("BeamCoordinates").value

        # The workspaces are stored in a dict: workspace_names (sample_scatter, etc) : ListOfWorkspaces
        for key, workspace_list in workspaces.items():
            is_trans = key in [
                SANSDataType.CAN_DIRECT,
                SANSDataType.CAN_TRANSMISSION,
                SANSDataType.SAMPLE_TRANSMISSION,
                SANSDataType.SAMPLE_DIRECT,
            ]

            for workspace in workspace_list:
                move_component(component_name="", state=state, workspace=workspace, move_type=MoveTypes.RESET_POSITION)

                move_component(
                    component_name="LAB",
                    state=state,
                    beam_coordinates=beam_coordinates,
                    move_type=MoveTypes.INITIAL_MOVE,
                    workspace=workspace,
                    is_transmission_workspace=is_trans,
                )

    def _get_progress_for_file_loading(self, data, state_adjustment):
        # Get the number of workspaces which are to be loaded
        number_of_files_to_load = sum(
            x is not None
            for x in [
                data.sample_scatter,
                data.sample_transmission,
                data.sample_direct,
                data.can_transmission,
                data.can_transmission,
                data.can_direct,
                state_adjustment.calibration,
            ]
        )
        progress_steps = number_of_files_to_load + 1
        # Check if there is a move operation to be performed

        # The partitioning of the progress bar is 80% for loading if there is a move else 100%
        end = 1.0
        progress = Progress(self, start=0.0, end=end, nreports=progress_steps)
        return progress


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSLoad)
