# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""Finds the beam centre for SANS"""

import numpy as np

from mantid import AnalysisDataService
from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress
from mantid.kernel import Direction, StringListValidator, Logger
from mantid.simpleapi import CloneWorkspace, GroupWorkspaces
from sans.algorithm_detail.beamcentrefinder_plotting import can_plot_beamcentrefinder, plot_workspace_quartiles
from sans.algorithm_detail.crop_helper import get_component_name
from sans.algorithm_detail.single_execution import perform_can_subtraction
from sans.algorithm_detail.strip_end_nans_and_infs import strip_end_nans
from SANS.sans.common.constants import EMPTY_NAME
from SANS.sans.common.enums import DetectorType, MaskingQuadrant, FindDirectionEnum
from SANS.sans.common.file_information import get_instrument_paths_for_sans_file
from SANS.sans.common.general_functions import create_child_algorithm
from SANS.sans.common.xml_parsing import get_named_elements_from_ipf_file
from sans.state.Serializer import Serializer


class SANSBeamCentreFinder(DataProcessorAlgorithm):
    def category(self):
        return "SANS\\BeamCentreFinder"

    def summary(self):
        return "Finds the position of the beam centre"

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        # Workspace which is to be cropped
        self.declareProperty("SANSState", "", doc="A JSON string which fulfills the SANSState contract.")

        self.declareProperty(
            MatrixWorkspaceProperty("SampleScatterWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="The sample scatter data",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("SampleScatterMonitorWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="The sample scatter monitor data",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("SampleTransmissionWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The sample transmission data",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("SampleDirectWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The sample direct data",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("CanScatterWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The can scatter data",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("CanScatterMonitorWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The can scatter monitor data",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("CanTransmissionWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The can transmission data",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("CanDirectWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The can direct data",
        )

        # The component, i.e. HAB or LAB
        allowed_detectors = StringListValidator([DetectorType.LAB.value, DetectorType.HAB.value])
        self.declareProperty(
            "Component",
            DetectorType.LAB.value,
            validator=allowed_detectors,
            direction=Direction.Input,
            doc="The component of the instrument which is to be reduced.",
        )

        self.declareProperty("Iterations", 10, direction=Direction.Input, doc="The maximum number of iterations.")

        self.declareProperty("RMin", 0.6, direction=Direction.Input, doc="The inner radius of the quartile mask")

        self.declareProperty("RMax", 0.28, direction=Direction.Input, doc="The outer radius of the quartile mask")

        self.declareProperty("Position1Start", 0.0, direction=Direction.Input, doc="The search start position1")

        self.declareProperty("Position2Start", 0.0, direction=Direction.Input, doc="The search start position2")

        self.declareProperty("Tolerance", 0.0001251, direction=Direction.Input, doc="The search tolerance")

        self.declareProperty(
            "Direction",
            FindDirectionEnum.ALL.value,
            direction=Direction.Input,
            doc="The search direction is an enumerable which can be either All, LeftRight or UpDown",
        )

        self.declareProperty("Verbose", False, direction=Direction.Input, doc="Whether to keep workspaces from each iteration in ADS.")

        # ----------
        # Output
        # ----------
        # Workspace which is to be cropped

        self.declareProperty("Centre1", 0.0, direction=Direction.Output, doc="The centre position found in the first dimension")
        self.declareProperty("Centre2", 0.0, direction=Direction.Output, doc="The centre position found in the second dimension")

    def PyExec(self):
        self.logger = Logger("CentreFinder")
        self.logger.notice("Starting centre finder routine...")

        self.sample_scatter = self._get_cloned_workspace("SampleScatterWorkspace")
        self.sample_scatter_monitor = self._get_cloned_workspace("SampleScatterMonitorWorkspace")
        self.sample_transmission = self._get_cloned_workspace("SampleTransmissionWorkspace")
        self.sample_direct = self._get_cloned_workspace("SampleDirectWorkspace")

        self.can_scatter = self._get_cloned_workspace("CanScatterWorkspace")
        self.can_scatter_monitor = self._get_cloned_workspace("CanScatterMonitorWorkspace")
        self.can_transmission = self._get_cloned_workspace("CanTransmissionWorkspace")
        self.can_direct = self._get_cloned_workspace("CanDirectWorkspace")

        self.component = self.getProperty("Component").value

        self.r_min = self.getProperty("RMin").value
        self.r_max = self.getProperty("RMax").value

        self.state = self._get_state()

        instrument = self.sample_scatter.getInstrument()
        self.scale_1 = 1.0 if instrument.getName() == "LARMOR" else 1000
        self.scale_2 = 1000

        centre_1_hold, centre_2_hold = self._find_centres()

        self.setProperty("Centre1", centre_1_hold)
        self.setProperty("Centre2", centre_2_hold)

        self.logger.notice("Centre coordinates updated: [{}, {}]".format(centre_1_hold * self.scale_1, centre_2_hold * self.scale_2))

    def _find_centres(self):
        progress = self._get_progress()
        verbose = self.getProperty("Verbose").value

        position_lr_step, position_tb_step = self.get_position_steps(self.state)

        centre_lr = self.getProperty("Position1Start").value
        centre_tb = self.getProperty("Position2Start").value

        tolerance = self.getProperty("Tolerance").value

        diff_left_right = []
        diff_top_bottom = []
        centre_lr_hold = centre_lr
        centre_tb_hold = centre_tb

        max_iterations = self.getProperty("Iterations").value
        for i in range(0, max_iterations + 1):
            if i != 0:
                centre_lr += position_lr_step
                centre_tb += position_tb_step

            progress.report("Reducing ... Pos1 " + str(centre_lr) + " Pos2 " + str(centre_tb))
            sample_quartiles = self._run_all_reductions(centre_lr, centre_tb)

            output_workspaces = self._publish_to_ADS(sample_quartiles)
            if verbose:
                self._rename_and_group_workspaces(i, output_workspaces)

            lr_results = self._calculate_residuals(sample_quartiles[MaskingQuadrant.LEFT], sample_quartiles[MaskingQuadrant.RIGHT])
            tb_results = self._calculate_residuals(sample_quartiles[MaskingQuadrant.TOP], sample_quartiles[MaskingQuadrant.BOTTOM])

            self._print_results(lr_results=lr_results, tb_results=tb_results, centre_lr=centre_lr, centre_tb=centre_tb, iteration=i)

            diff_left_right.append(lr_results.total_residual)
            diff_top_bottom.append(tb_results.total_residual)

            if i == 0:
                self._plot_current_result(output_workspaces)
            else:
                # have we stepped across the y-axis that goes through the beam center?
                if diff_left_right[i] > diff_left_right[i - 1]:
                    # yes with stepped across the middle, reverse direction and half the step size
                    position_lr_step = -position_lr_step / 2
                if diff_top_bottom[i] > diff_top_bottom[i - 1]:
                    position_tb_step = -position_tb_step / 2

                if (diff_left_right[i] + diff_top_bottom[i]) < (
                    diff_left_right[i - 1] + diff_top_bottom[i - 1]
                ) or self.state.compatibility.use_compatibility_mode:
                    centre_lr_hold = centre_lr
                    centre_tb_hold = centre_tb

                if abs(position_lr_step) < tolerance and abs(position_tb_step) < tolerance:
                    # this is the success criteria, we've close enough to the center
                    self.logger.notice("Converged - check if stuck in local minimum! ")
                    break

            if i == max_iterations:
                self.logger.notice("Out of iterations, new coordinates may not be the best")
        return centre_lr_hold, centre_tb_hold

    def _print_results(self, iteration, centre_lr, centre_tb, lr_results, tb_results):
        scaled_lr = self.scale_1 * centre_lr
        scaled_tb = self.scale_2 * centre_tb

        avg_lr_residual = lr_results.total_residual / lr_results.num_points_considered
        avg_tb_residual = tb_results.total_residual / tb_results.num_points_considered

        iter_details = "Itr {:02d}: ({:7.3f}, {:7.3f})  SX={:.3e}  SY={:.3e}  Points: {:3d} (Unaligned: {:2d})".format(
            iteration,
            scaled_lr,
            scaled_tb,
            avg_lr_residual,
            avg_tb_residual,
            lr_results.num_points_considered,
            lr_results.mismatched_points,
        )

        self.logger.notice(iter_details)

    def _plot_current_result(self, output_workspaces):
        if can_plot_beamcentrefinder():
            break_loop = self._plot_workspaces(output_workspaces, self.state.data.sample_scatter)
            if break_loop:
                # If workspaces contain NaN values, stop the process.
                raise WorkspaceContainsNanValues()

    def _run_all_reductions(self, centre1, centre2):
        sample_quartiles = self._run_quartile_reduction(
            scatter_workspace=self.sample_scatter,
            transmission_workspace=self.sample_transmission,
            direct_workspace=self.sample_direct,
            scatter_monitor_workspace=self.sample_scatter_monitor,
            data_type="Sample",
            centre1=centre1,
            centre2=centre2,
        )
        if self.can_scatter:
            can_quartiles = self._run_quartile_reduction(
                scatter_workspace=self.can_scatter,
                transmission_workspace=self.can_transmission,
                direct_workspace=self.can_direct,
                data_type="Can",
                scatter_monitor_workspace=self.can_scatter_monitor,
                centre1=centre1,
                centre2=centre2,
            )
            for key in sample_quartiles:
                sample_quartiles[key] = perform_can_subtraction(sample_quartiles[key], can_quartiles[key], self)
        return sample_quartiles

    def get_position_steps(self, state):
        instrument_file = get_instrument_paths_for_sans_file(state.data.sample_scatter)
        position_1_step = get_named_elements_from_ipf_file(instrument_file[1], ["centre-finder-step-size"], float)[
            "centre-finder-step-size"
        ]
        try:
            position_2_step = get_named_elements_from_ipf_file(instrument_file[1], ["centre-finder-step-size2"], float)[
                "centre-finder-step-size2"
            ]
        except:
            position_2_step = position_1_step

        find_direction = self.getProperty("Direction").value
        if find_direction == FindDirectionEnum.LEFT_RIGHT.value:
            position_2_step = 0.0
        elif find_direction == FindDirectionEnum.UP_DOWN.value:
            position_1_step = 0.0

        return position_1_step, position_2_step

    def _plot_workspaces(self, output_workspaces, sample_scatter):
        try:
            # Check for NaNs in workspaces
            self._validate_workspaces(output_workspaces)
        except ValueError as e:
            self.logger.notice("Stopping process: {}. Check radius limits.".format(str(e)))
            return True
        else:
            plot_workspace_quartiles(output_workspaces, sample_scatter)
        return False

    @staticmethod
    def _rename_and_group_workspaces(index, output_workspaces):
        to_group = []
        for workspace in output_workspaces:
            CloneWorkspace(InputWorkspace=workspace, OutputWorkspace="{}_{}".format(workspace, index))
            to_group.append("{}_{}".format(workspace, index))
        GroupWorkspaces(InputWorkspaces=to_group, OutputWorkspace="Iteration_{}".format(index))

    @staticmethod
    def _publish_to_ADS(sample_quartiles):
        output_workspaces = []
        for key in sample_quartiles:
            assert isinstance(key, MaskingQuadrant)
            output_workspaces.append(key.value)
            AnalysisDataService.addOrReplace(key.value, sample_quartiles[key])

        return output_workspaces

    @staticmethod
    def _validate_workspaces(workspaces):
        """
        This method checks if any of the workspaces to plot contain NaN values.
        :param workspaces: A list of workspace names
        :return: A list of workspaces (used in matplotlib plotting). Raises if NaN values present.
        """
        workspaces = AnalysisDataService.Instance().retrieveWorkspaces(workspaces, unrollGroups=True)
        for ws in workspaces:
            if np.isnan(ws.readY(0)).any():
                # All data can be NaN if bounds are too close together
                # this makes the data unplottable
                raise ValueError("Workspace contains NaN values.")
        return workspaces

    def _get_cloned_workspace(self, workspace_name):
        workspace = self.getProperty(workspace_name).value
        if workspace:
            clone_name = "CloneWorkspace"
            clone_options = {"InputWorkspace": workspace, "OutputWorkspace": EMPTY_NAME}
            clone_alg = create_child_algorithm(self, clone_name, **clone_options)
            clone_alg.execute()
            return clone_alg.getProperty("OutputWorkspace").value
        return ""

    def _run_quartile_reduction(
        self, scatter_workspace, transmission_workspace, direct_workspace, data_type, scatter_monitor_workspace, centre1, centre2
    ):
        serialized_state = self.getProperty("SANSState").value

        algorithm_name = "SANSBeamCentreFinderCore"
        alg_options = {
            "ScatterWorkspace": scatter_workspace,
            "ScatterMonitorWorkspace": scatter_monitor_workspace,
            "TransmissionWorkspace": transmission_workspace,
            "DirectWorkspace": direct_workspace,
            "Component": self.component,
            "SANSState": serialized_state,
            "DataType": data_type,
            "Centre1": centre1,
            "Centre2": centre2,
            "OutputWorkspaceLeft": EMPTY_NAME,
            "OutputWorkspaceRight": EMPTY_NAME,
            "OutputWorkspaceTop": EMPTY_NAME,
            "OutputWorkspaceBottom": EMPTY_NAME,
            "RMax": self.r_max,
            "RMin": self.r_min,
        }
        alg = create_child_algorithm(self, algorithm_name, **alg_options)
        alg.execute()
        out_left = strip_end_nans(alg.getProperty("OutputWorkspaceLeft").value, self)
        out_right = strip_end_nans(alg.getProperty("OutputWorkspaceRight").value, self)
        out_top = strip_end_nans(alg.getProperty("OutputWorkspaceTop").value, self)
        out_bottom = strip_end_nans(alg.getProperty("OutputWorkspaceBottom").value, self)
        return {
            MaskingQuadrant.LEFT: out_left,
            MaskingQuadrant.RIGHT: out_right,
            MaskingQuadrant.TOP: out_top,
            MaskingQuadrant.BOTTOM: out_bottom,
        }

    def _get_component(self, workspace):
        component = DetectorType(self.component)
        return get_component_name(workspace, component)

    def _get_state(self):
        state_json = self.getProperty("SANSState").value
        state = Serializer.from_json(state_json)

        return state

    def _calculate_residuals(self, quartile1, quartile2):
        yvalsAX = quartile1.readY(0)
        yvalsBX = quartile2.readY(0)
        qvalsAX = quartile1.readX(0)
        qvalsBX = quartile2.readX(0)
        A_vals_dict = dict(zip(qvalsAX, yvalsAX))
        B_vals_dict = dict(zip(qvalsBX, yvalsBX))

        residue = 0.0
        mismatched_points = 0
        for key in B_vals_dict:
            if key not in A_vals_dict:
                A_vals_dict[key] = 0.0
                mismatched_points += 1

        for key in A_vals_dict:
            if key not in B_vals_dict:
                B_vals_dict[key] = 0.0
                mismatched_points += 1

        assert len(A_vals_dict) == len(B_vals_dict)

        for key in A_vals_dict and B_vals_dict:
            residue += pow(A_vals_dict[key] - B_vals_dict[key], 2)

        self.logger.information("Beam Centre Diff: {0}".format(residue))

        return _ResidualsDetails(mismatched_points=mismatched_points, num_points_considered=len(A_vals_dict), total_residual=residue)

    def _get_progress(self):
        return Progress(self, start=0.0, end=1.0, nreports=10)


class WorkspaceContainsNanValues(Exception):
    pass


class _ResidualsDetails(object):
    def __init__(self, num_points_considered, mismatched_points, total_residual):
        self.num_points_considered = num_points_considered
        self.mismatched_points = mismatched_points
        self.total_residual = total_residual


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSBeamCentreFinder)
