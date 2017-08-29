# pylint: disable=invalid-name
""" SANBatchReduction algorithm is the starting point for any new type reduction, event single reduction"""
from __future__ import (absolute_import, division, print_function)
from sans.state.state import State
from sans.algorithm_detail.batch_execution import (single_reduction_for_batch)
from sans.common.enums import (OutputMode)


class SANSBatchReduction(object):
    def __init__(self):
        super(SANSBatchReduction, self).__init__()

    def __call__(self, states, use_optimizations=True, output_mode=OutputMode.PublishToADS):
        """
        This is the start of any reduction.

        :param states: This is a list of sans states. Each state in the list corresponds to a single reduction.
        :param use_optimizations: if True then the optimizations for file reloading are used.
        :param output_mode: The output mode defines how the reduced data should be published. This can be
                            1. PublishToADS
                            2. SaveToFile
                            3. Both
        """
        self.validate_inputs(states, use_optimizations, output_mode)

        self._execute(states, use_optimizations, output_mode)

    @staticmethod
    def _execute(states, use_optimizations, output_mode):
        # Iterate over each state, load the data and perform the reduction
        for state in states:
            single_reduction_for_batch(state, use_optimizations, output_mode)

    def validate_inputs(self, states, use_optimizations, output_mode):
        # We are strict about the types here.
        # 1. states has to be a list of sans state objects
        # 2. use_optimizations has to be bool
        # 3. output_mode has to be an OutputMode enum
        if not isinstance(states, list):
            raise RuntimeError("The provided states are not in a list. They have to be in a list.")

        for state in states:
            if not isinstance(state, State):
                raise RuntimeError("The entries have to be sans state objects. "
                                   "The provided type is {0}".format(type(state)))

        if not isinstance(use_optimizations, bool):
            raise RuntimeError("The optimization has to be a boolean. The provided type is"
                               " {0}".format(type(use_optimizations)))

        if output_mode is not OutputMode.PublishToADS and output_mode is not OutputMode.SaveToFile and\
                        output_mode is not OutputMode.Both:  # noqa
            raise RuntimeError("The output mode has to be an enum of type OutputMode. The provided type is"
                               " {0}".format(type(output_mode)))

        errors = self._validate_inputs(states)
        if errors:
            raise RuntimeError("The provided states are not valid: {}".format(errors))

    @staticmethod
    def _validate_inputs(states):
        errors = dict()
        # Check that the input can be converted into the right state object
        try:
            for state in states:
                state.validate()
        except ValueError as err:
            errors.update({"SANSBatchReduction": str(err)})
        return errors
