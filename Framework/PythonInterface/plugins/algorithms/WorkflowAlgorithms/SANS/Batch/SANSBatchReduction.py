# pylint: disable=invalid-name

""" SANBatchReduction algorithm is the starting point for any new type reduction, event single reduction"""

from mantid.kernel import (Direction, PropertyManagerProperty, FloatArrayProperty,
                           EnabledWhenProperty, PropertyCriterion)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode)

from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager
from SANS.Batch.BatchExecution import single_reduction_for_batch


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

        self.declareProperty("UseAdsOptimizations", True, direction=Direction.Input,
                             doc="When enabled the ADS is being searched for already loaded and reduced workspaces. "
                                 "Depending on your concrete reduction, this could provide a significant"
                                 " performance boost")

    def PyExec(self):
        # Read the states.
        states = self._get_states()

        # Check if optimizations are to be used
        use_optimizations = self.getProperty("UseAdsOptimizations").value

        # We now iterate over each state, load the data and perform the reduction
        for state in states:
            single_reduction_for_batch(state, use_optimizations)

        # Handle the results
        # TODO

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

    def _get_states(self):
        # The property manager contains a collection of states
        outer_property_manager = self.getProperty("SANSStates").value
        keys = outer_property_manager.keys()
        sans_states = []
        for key in keys:
            inner_property_manager = outer_property_manager[key]
            state = create_deserialized_sans_state_from_property_manager(inner_property_manager)
            state.property_manager = inner_property_manager
            sans_states.append(state)
        return sans_states


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSBatchReduction)
