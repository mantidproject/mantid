# pylint: disable=invalid-name

""" SANBatchReduction algorithm is the starting point for any new type reduction, event single reduction"""

from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory,
                        PropertyMode, AnalysisDataService)

from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager
from SANS.Batch.BatchExecution import (single_reduction_for_batch, OutputMode)


class SANSBatchReduction(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Reduction'

    def summary(self):
        return 'Performs a batch reduction of SANS data.'

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        self.declareProperty(PropertyManagerProperty('SANSStates'),
                             doc='This is a dictionary of SANSStates. Note that the key is irrelevant here. '
                                 'Each SANSState in the dictionary corresponds to a single reduction.')

        self.declareProperty("UseOptimizations", True, direction=Direction.Input,
                             doc="When enabled the ADS is being searched for already loaded and reduced workspaces. "
                                 "Depending on your concrete reduction, this could provide a significant"
                                 " performance boost")

        allowed_detectors = StringListValidator(["PublishToADS", "SaveToFile", "Both"])
        self.declareProperty("OutputMode", "PublishToADS", validator=allowed_detectors, direction=Direction.Input,
                             doc="There are two output modes available./n"
                                 "PublishToADS: publishes the workspaces to the ADS. /n"
                                 "SaveToFile: Saves the workspaces to file.")

    def PyExec(self):
        # Read the states.
        states = self._get_states()

        # Check if optimizations are to be used
        use_optimizations = self.getProperty("UseOptimizations").value

        # Check how the output is to be handled
        output_modes = self._get_output_modes()

        # We now iterate over each state, load the data and perform the reduction
        for state in states:
            single_reduction_for_batch(state, use_optimizations, output_modes)

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        try:
            states = self._get_states()
            for state in states:
                state.validate()
        except ValueError as err:
            errors.update({"SANSBatchReduction": str(err)})
        return errors

    def _get_output_modes(self):
        output_mode = self.getProperty("OutputMode").value
        if output_mode == "PublishToADS":
            mode = [OutputMode.PublishToADS]
        elif output_mode == "SaveToFile":
            mode = [OutputMode.SaveToFile]
        elif output_mode == "Both":
            mode = [OutputMode.PublishToADS, OutputMode.SaveToFile]
        else:
            raise ValueError("SANSBatchReduction: Unknown publication mode {0}".format(output_mode))
        return mode

    def _get_states(self):
        # The property manager contains a collection of states
        outer_property_manager = self.getProperty("SANSStates").value
        keys = outer_property_manager.keys()
        sans_states = []
        for key in keys:
            inner_property_manager = outer_property_manager.getProperty(key).value
            state = create_deserialized_sans_state_from_property_manager(inner_property_manager)
            state.property_manager = inner_property_manager
            sans_states.append(state)
        return sans_states

    def _publish_to_ads(self, batch_reduction_return_bundles):
        for bundle in batch_reduction_return_bundles:
            self._publish(bundle.lab)
            self._publish(bundle.hab)
            self._publish(bundle.merged)

    def _publish(self, workspace):
        if workspace is not None:
            name = workspace.name()
            title = workspace.getTitle()
            workspace_name = name if name else title
            AnalysisDataService.addOrReplace(workspace_name, workspace)

    def _add_property(self, workspace, prefix, current_count):
        if workspace is not None:
            total_number_of_workspaces = current_count + 1
            out_name = "OutputWorkspace" + prefix + "_" + str(total_number_of_workspaces)
            self.declareProperty(MatrixWorkspaceProperty(out_name, '',
                                                         optional=PropertyMode.Optional,
                                                         direction=Direction.Output),
                                 doc='The {0}th output workspace for a {1}-type'
                                     ' reduction mode.'.format(total_number_of_workspaces, prefix))
            self.setProperty(out_name, workspace)
        else:
            total_number_of_workspaces = current_count
        return total_number_of_workspaces


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSBatchReduction)
