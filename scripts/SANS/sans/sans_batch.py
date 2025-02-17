# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
"""SANBatchReduction algorithm is the starting point for any new type reduction, event single reduction"""

from sans.state.AllStates import AllStates
from sans.algorithm_detail.batch_execution import single_reduction_for_batch
from sans.common.enums import OutputMode, FindDirectionEnum, DetectorType
from sans.algorithm_detail.centre_finder_new import centre_finder_new, centre_finder_mass


class SANSBatchReduction(object):
    def __init__(self):
        super(SANSBatchReduction, self).__init__()

    def __call__(
        self, states, use_optimizations=True, output_mode=OutputMode.PUBLISH_TO_ADS, plot_results=False, output_graph="", save_can=False
    ):
        """
        This is the start of any reduction.

        :param states: This is a list of sans states. Each state in the list corresponds to a single reduction.
        :param use_optimizations: if True then the optimizations for file reloading are used.
        :param output_mode: The output mode defines how the reduced data should be published. This can be
                            1. PublishToADS
                            2. SaveToFile
                            3. Both
        """
        self.validate_inputs(states, use_optimizations, output_mode, plot_results, output_graph)

        return self._execute(states, use_optimizations, output_mode, plot_results, output_graph, save_can=save_can)

    @staticmethod
    def _execute(states, use_optimizations, output_mode, plot_results, output_graph, save_can=False):
        # Iterate over each state, load the data and perform the reduction
        out_scale_factors_list = []
        out_shift_factors_list = []
        for state in states:
            out_scale_factors, out_shift_factors = single_reduction_for_batch(
                state, use_optimizations, output_mode, plot_results, output_graph, save_can=save_can
            )
            out_shift_factors_list.append(out_shift_factors)
            out_scale_factors_list.append(out_scale_factors)
        return out_scale_factors_list, out_shift_factors_list

    def validate_inputs(self, states, use_optimizations, output_mode, plot_results, output_graph):
        # We are strict about the types here.
        # 1. states has to be a list of sans state objects
        # 2. use_optimizations has to be bool
        # 3. output_mode has to be an OutputMode enum
        if not isinstance(states, list):
            raise RuntimeError("The provided states are not in a list. They have to be in a list.")

        for state in states:
            if not isinstance(state, AllStates):
                raise RuntimeError("The entries have to be sans state objects. The provided type is {0}".format(type(state)))

        if not isinstance(use_optimizations, bool):
            raise RuntimeError("The optimization has to be a boolean. The provided type is {0}".format(type(use_optimizations)))

        if not isinstance(plot_results, bool):
            raise RuntimeError("The plot_result has to be a boolean. The provided type is {0}".format(type(plot_results)))

        if plot_results and not output_graph:
            raise RuntimeError("The output_graph must be set if plot_results is true. The provided value is {0}".format(output_graph))

        if (
            output_mode is not OutputMode.PUBLISH_TO_ADS
            and output_mode is not OutputMode.SAVE_TO_FILE
            and output_mode is not OutputMode.BOTH
        ):
            raise RuntimeError("The output mode has to be an enum of type OutputMode. The provided type is {0}".format(type(output_mode)))

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


class SANSCentreFinder(object):
    def __init__(self):
        super(SANSCentreFinder, self).__init__()

    def __call__(
        self,
        state,
        r_min=60,
        r_max=280,
        max_iter=20,
        x_start=0.0,
        y_start=0.0,
        tolerance=1.251e-4,
        find_direction=FindDirectionEnum.ALL,
        reduction_method=True,
        verbose=False,
        component=DetectorType.LAB,
    ):
        """
        This is the start of the beam centre finder algorithm.

        :param state: This is a sans state, to find the beam centre for.
        :param r_min: This is the inner radius of the quartile mask in mm.
        :param r_max: This is the outer radius of the quartile mask in mm.
        :param max_iter: This is the maximum number of iterations.
        :param x_start: This is the starting position of the search on the x axis in metres or degrees.
        :param y_start: This is the starting position of the search on the y axis in metres.
        :param tolerance: This is the tolerance for the search.
        :param fine_direction: This is an enumerator controlling which axis or both should be searched.
        :param reduction_method: This is a bool controlling which centre finder algorithm to use. By default the
        reduction method is used.
        """
        self.validate_inputs(state, r_min, r_max, max_iter, x_start, y_start, tolerance)

        if reduction_method:
            return self._execute_reduction_method(
                state, r_min, r_max, max_iter, x_start, y_start, tolerance, find_direction, verbose, component
            )
        else:
            return self._execute_mass_method(state, r_min, max_iter, x_start, y_start, tolerance, component)

    @staticmethod
    def _execute_reduction_method(state, r_min, r_max, max_iter, xstart, ystart, tolerance, find_direction, verbose, component):
        # Perform the beam centre finder algorithm
        return centre_finder_new(state, r_min, r_max, max_iter, xstart, ystart, tolerance, find_direction, verbose, component)

    @staticmethod
    def _execute_mass_method(state, r_min, max_iter, xstart, ystart, tolerance, component):
        # Perform the beam centre finder algorithm
        return centre_finder_mass(state, r_min, max_iter, xstart, ystart, tolerance, component)

    def validate_inputs(self, state, r_min, r_max, max_iter, xstart, ystart, tolerance):
        # We are strict about the types here.
        # 1. state has to be a sans state object
        # 2. r_min, r_max, tolerance have to be positive floats
        # 3. xstart, ystart have to be floats
        # 4. max_iter has to be an integer

        if not isinstance(state, AllStates):
            raise RuntimeError("The entries have to be sans state objects. The provided type is {0}".format(type(state)))

        if not isinstance(r_min, float):
            raise RuntimeError("The minimum radius has to be a float. The provided type is {0}".format(type(r_min)))

        if not isinstance(r_max, float):
            raise RuntimeError("The maximum radius has to be a float. The provided type is {0}".format(type(r_max)))

        if not isinstance(xstart, float):
            raise RuntimeError("The x starting position has to be a float. The provided type is {0}".format(type(xstart)))

        if not isinstance(tolerance, float):
            raise RuntimeError("The tolerance has to be a float. The provided type is {0}".format(type(tolerance)))

        if not isinstance(max_iter, int):
            raise RuntimeError("The iteration number must be an integer. The provided type is {0}".format(type(max_iter)))

        if not isinstance(ystart, float):
            raise RuntimeError("The y starting position has to be a float. The provided type is {0}".format(type(ystart)))

        errors = self._validate_inputs(state)
        if errors:
            raise RuntimeError("The provided states are not valid: {}".format(errors))

    @staticmethod
    def _validate_inputs(state):
        errors = dict()
        # Check that the input can be converted into the right state object
        try:
            state.validate()
        except ValueError as err:
            errors.update({"SANSBatchReduction": str(err)})
        return errors
