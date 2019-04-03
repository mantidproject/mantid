# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

""" Finds the beam centre for SANS"""

from __future__ import (absolute_import, division, print_function)

import numpy as np

from mantid import AnalysisDataService
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress)
from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator, Logger)
from mantid.simpleapi import CloneWorkspace, GroupWorkspaces
from sans.algorithm_detail.beamcentrefinder_plotting import can_plot_beamcentrefinder
from sans.algorithm_detail.crop_helper import get_component_name
from sans.algorithm_detail.single_execution import perform_can_subtraction
from sans.algorithm_detail.strip_end_nans_and_infs import strip_end_nans
from sans.common.constants import EMPTY_NAME
from sans.common.enums import (DetectorType, MaskingQuadrant, FindDirectionEnum)
from sans.common.file_information import get_instrument_paths_for_sans_file
from sans.common.general_functions import create_child_algorithm
from sans.common.xml_parsing import get_named_elements_from_ipf_file
from sans.state.state_base import create_deserialized_sans_state_from_property_manager

PYQT4, IN_MANTIDPLOT, WITHOUT_GUI = can_plot_beamcentrefinder()
do_plotting = not PYQT4 or IN_MANTIDPLOT


class SANSBeamCentreFinder(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\BeamCentreFinder'

    def summary(self):
        return 'Finds the position of the beam centre'

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        # Workspace which is to be cropped
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        self.declareProperty(MatrixWorkspaceProperty("SampleScatterWorkspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The sample scatter data')

        self.declareProperty(MatrixWorkspaceProperty("SampleScatterMonitorWorkspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The sample scatter monitor data')

        self.declareProperty(MatrixWorkspaceProperty("SampleTransmissionWorkspace", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The sample transmission data')

        self.declareProperty(MatrixWorkspaceProperty("SampleDirectWorkspace", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The sample direct data')

        self.declareProperty(MatrixWorkspaceProperty("CanScatterWorkspace", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The can scatter data')

        self.declareProperty(MatrixWorkspaceProperty("CanScatterMonitorWorkspace", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The can scatter monitor data')

        self.declareProperty(MatrixWorkspaceProperty("CanTransmissionWorkspace", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The can transmission data')

        self.declareProperty(MatrixWorkspaceProperty("CanDirectWorkspace", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The can direct data')

        # The component, i.e. HAB or LAB
        allowed_detectors = StringListValidator([DetectorType.to_string(DetectorType.LAB),
                                                 DetectorType.to_string(DetectorType.HAB)])
        self.declareProperty("Component", DetectorType.to_string(DetectorType.LAB),
                             validator=allowed_detectors, direction=Direction.Input,
                             doc="The component of the instrument which is to be reduced.")

        self.declareProperty("Iterations", 10, direction=Direction.Input, doc="The maximum number of iterations.")

        self.declareProperty("RMin", 0.6, direction=Direction.Input, doc="The inner radius of the quartile mask")

        self.declareProperty('RMax', 0.28, direction=Direction.Input, doc="The outer radius of the quartile mask")

        self.declareProperty('Position1Start', 0.0, direction=Direction.Input, doc="The search start position1")

        self.declareProperty('Position2Start', 0.0, direction=Direction.Input, doc="The search start position2")

        self.declareProperty('Tolerance', 0.0001251, direction=Direction.Input, doc="The search tolerance")

        self.declareProperty('Direction', FindDirectionEnum.to_string(FindDirectionEnum.All), direction=Direction.Input,
                             doc="The search direction is an enumerable which can be either All, LeftRight or UpDown")

        self.declareProperty('Verbose', False, direction=Direction.Input,
                             doc="Whether to keep workspaces from each iteration in ADS.")

        # ----------
        # Output
        # ----------
        # Workspace which is to be cropped

        self.declareProperty('Centre1', 0.0, direction=Direction.Output,
                             doc="The centre position found in the first dimension")
        self.declareProperty('Centre2', 0.0, direction=Direction.Output,
                             doc="The centre position found in the second dimension")

    def PyExec(self):
        state = self._get_state()
        state_serialized = state.property_manager
        self.logger = Logger("CentreFinder")
        self.logger.notice("Starting centre finder routine...")
        progress = self._get_progress()
        self.scale_1 = 1000
        self.scale_2 = 1000
        verbose = self.getProperty('Verbose').value
        x_start = self.getProperty("Position1Start").value
        y_start = self.getProperty("Position2Start").value

        sample_scatter = self._get_cloned_workspace("SampleScatterWorkspace")
        sample_scatter_monitor = self._get_cloned_workspace("SampleScatterMonitorWorkspace")
        sample_transmission = self._get_cloned_workspace("SampleTransmissionWorkspace")
        sample_direct = self._get_cloned_workspace("SampleDirectWorkspace")

        instrument = sample_scatter.getInstrument()
        if instrument.getName() == 'LARMOR':
            self.scale_1 = 1.0

        can_scatter = self._get_cloned_workspace("CanScatterWorkspace")
        can_scatter_monitor = self._get_cloned_workspace("CanScatterMonitorWorkspace")
        can_transmission = self._get_cloned_workspace("CanTransmissionWorkspace")
        can_direct = self._get_cloned_workspace("CanDirectWorkspace")

        component = self.getProperty("Component").value
        tolerance = self.getProperty("Tolerance").value
        max_iterations = self.getProperty("Iterations").value

        r_min = self.getProperty("RMin").value
        r_max = self.getProperty("RMax").value

        instrument_file = get_instrument_paths_for_sans_file(state.data.sample_scatter)
        position_1_step = get_named_elements_from_ipf_file(
            instrument_file[1], ["centre-finder-step-size"], float)['centre-finder-step-size']
        try:
            position_2_step = get_named_elements_from_ipf_file(
                instrument_file[1], ["centre-finder-step-size2"], float)['centre-finder-step-size2']
        except:
            position_2_step = position_1_step

        find_direction = self.getProperty("Direction").value
        if find_direction == FindDirectionEnum.to_string(FindDirectionEnum.Left_Right):
            position_2_step = 0.0
        elif find_direction == FindDirectionEnum.to_string(FindDirectionEnum.Up_Down):
            position_1_step = 0.0
        centre1 = x_start
        centre2 = y_start
        residueLR = []
        residueTB = []
        centre_1_hold = x_start
        centre_2_hold = y_start
        for j in range(0, max_iterations + 1):
            if(j != 0):
                centre1 += position_1_step
                centre2 += position_2_step

            progress.report("Reducing ... Pos1 " + str(centre1) + " Pos2 " + str(centre2))
            sample_quartiles = self._run_quartile_reduction(sample_scatter, sample_transmission, sample_direct,
                                                            "Sample", sample_scatter_monitor, component,
                                                            state_serialized, centre1, centre2, r_min, r_max)

            if can_scatter:
                can_quartiles = self._run_quartile_reduction(can_scatter, can_transmission, can_direct, "Can",
                                                             can_scatter_monitor, component, state_serialized, centre1,
                                                             centre2, r_min, r_max)
                for key in sample_quartiles:
                    sample_quartiles[key] = perform_can_subtraction(sample_quartiles[key], can_quartiles[key], self)

            if do_plotting:
                output_workspaces = self._publish_to_ADS(sample_quartiles)
                if verbose:
                    self._rename_and_group_workspaces(j, output_workspaces)

            residueLR.append(self._calculate_residuals(sample_quartiles[MaskingQuadrant.Left],
                                                       sample_quartiles[MaskingQuadrant.Right]))
            residueTB.append(self._calculate_residuals(sample_quartiles[MaskingQuadrant.Top],
                                                       sample_quartiles[MaskingQuadrant.Bottom]))
            if j == 0:
                self.logger.notice("Itr {0}: ( {1:.3f}, {2:.3f} )  SX={3:.5f}  SY={4:.5f}".
                                   format(j, self.scale_1 * centre1,
                                          self.scale_2 * centre2, residueLR[j], residueTB[j]))
                if do_plotting:
                    break_loop = self._plot_workspaces(output_workspaces, state.data.sample_scatter)
                    if break_loop:
                        # If workspaces contain NaN values, stop the process.
                        break
            else:
                # have we stepped across the y-axis that goes through the beam center?
                if residueLR[j] > residueLR[j-1]:
                    # yes with stepped across the middle, reverse direction and half the step size
                    position_1_step = - position_1_step / 2
                if residueTB[j] > residueTB[j-1]:
                    position_2_step = - position_2_step / 2

                self.logger.notice("Itr {0}: ( {1:.3f}, {2:.3f} )  SX={3:.5f}  SY={4:.5f}".
                                   format(j, self.scale_1 * centre1,
                                          self.scale_2 * centre2, residueLR[j], residueTB[j]))

                if (residueLR[j]+residueTB[j]) < (residueLR[j-1]+residueTB[j-1]) or \
                        state.compatibility.use_compatibility_mode:
                    centre_1_hold = centre1
                    centre_2_hold = centre2

                if abs(position_1_step) < tolerance and abs(position_2_step) < tolerance:
                    # this is the success criteria, we've close enough to the center
                    self.logger.notice("Converged - check if stuck in local minimum! ")
                    break

            if j == max_iterations:
                self.logger.notice("Out of iterations, new coordinates may not be the best")

        self.setProperty("Centre1", centre_1_hold)
        self.setProperty("Centre2", centre_2_hold)

        self.logger.notice("Centre coordinates updated: [{}, {}]".format(centre_1_hold*self.scale_1, centre_2_hold*self.scale_2))

    def _plot_workspaces(self, output_workspaces, sample_scatter):
        try:
            # Check for NaNs in workspaces
            output_workspaces_matplotlib = self._validate_workspaces(output_workspaces)
        except ValueError as e:
            self.logger.notice("Stopping process: {}. Check radius limits.".format(str(e)))
            return True
        else:
            if not PYQT4:
                # matplotlib plotting can take a list of workspaces (not names)
                self._plot_quartiles_matplotlib(output_workspaces_matplotlib, sample_scatter)
            elif IN_MANTIDPLOT:
                self._plot_quartiles(output_workspaces, sample_scatter)
        return False

    def _rename_and_group_workspaces(self, index, output_workspaces):
        to_group = []
        for workspace in output_workspaces:
            CloneWorkspace(InputWorkspace=workspace, OutputWorkspace='{}_{}'.format(workspace, index))
            to_group.append('{}_{}'.format(workspace, index))
        GroupWorkspaces(InputWorkspaces=to_group,OutputWorkspace='Iteration_{}'.format(index))

    def _publish_to_ADS(self, sample_quartiles):
        output_workspaces = []
        for key in sample_quartiles:
            output_workspaces.append(MaskingQuadrant.to_string(key))
            AnalysisDataService.addOrReplace(MaskingQuadrant.to_string(key), sample_quartiles[key])

        return output_workspaces

    def _plot_quartiles(self, output_workspaces, sample_scatter):
        title = '{}_beam_centre_finder'.format(sample_scatter)
        graph_handle = mantidplot.plotSpectrum(output_workspaces, 0)
        graph_handle.activeLayer().logLogAxes()
        graph_handle.activeLayer().setTitle(title)
        graph_handle.setName(title)
        return graph_handle

    def _plot_quartiles_matplotlib(self, output_workspaces, sample_scatter):
        title = '{}_beam_centre_finder'.format(sample_scatter)
        ax_properties = {'xscale': 'log',
                         'yscale': 'log'}

        plot_kwargs = {"scalex": True,
                       "scaley": True}

        if not isinstance(output_workspaces, list):
            output_workspaces = [output_workspaces]

        if not WITHOUT_GUI:
            plot(output_workspaces, wksp_indices=[0], ax_properties=ax_properties, overplot=True,
                 plot_kwargs=plot_kwargs, window_title=title)

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
            clone_options = {"InputWorkspace": workspace,
                             "OutputWorkspace": EMPTY_NAME}
            clone_alg = create_child_algorithm(self, clone_name, **clone_options)
            clone_alg.execute()
            return clone_alg.getProperty("OutputWorkspace").value
        return ''

    def _run_quartile_reduction(self, scatter_workspace, transmission_workspace, direct_workspace, data_type,
                                scatter_monitor_workspace, component, state, centre1, centre2, r_min, r_max):
        algorithm_name = "SANSBeamCentreFinderCore"
        alg_options = {"ScatterWorkspace": scatter_workspace,
                       "ScatterMonitorWorkspace": scatter_monitor_workspace,
                       "TransmissionWorkspace": transmission_workspace,
                       "DirectWorkspace": direct_workspace,
                       "Component": component,
                       "SANSState": state,
                       "DataType": data_type,
                       "Centre1": centre1,
                       "Centre2": centre2,
                       "OutputWorkspaceLeft": EMPTY_NAME,
                       "OutputWorkspaceRight": EMPTY_NAME,
                       "OutputWorkspaceTop": EMPTY_NAME,
                       "OutputWorkspaceBottom": EMPTY_NAME,
                       "RMax": r_max,
                       "RMin": r_min}
        alg = create_child_algorithm(self, algorithm_name, **alg_options)
        alg.execute()
        out_left = strip_end_nans(alg.getProperty("OutputWorkspaceLeft").value, self)
        out_right = strip_end_nans(alg.getProperty("OutputWorkspaceRight").value, self)
        out_top = strip_end_nans(alg.getProperty("OutputWorkspaceTop").value, self)
        out_bottom = strip_end_nans(alg.getProperty("OutputWorkspaceBottom").value, self)
        return {MaskingQuadrant.Left: out_left, MaskingQuadrant.Right: out_right, MaskingQuadrant.Top: out_top,
                MaskingQuadrant.Bottom: out_bottom}

    def _get_component(self, workspace):
        component_as_string = self.getProperty("Component").value
        component = DetectorType.from_string(component_as_string)
        return get_component_name(workspace, component)

    def _get_state(self):
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)
        state.property_manager = state_property_manager
        return state

    def _calculate_residuals(self, quartile1, quartile2):
        yvalsAX = quartile1.readY(0)
        yvalsBX = quartile2.readY(0)
        qvalsAX = quartile1.readX(0)
        qvalsBX = quartile2.readX(0)
        A_vals_dict = dict(zip(qvalsAX, yvalsAX))
        B_vals_dict = dict(zip(qvalsBX, yvalsBX))

        residue = 0.0
        for key in B_vals_dict:
            if key not in A_vals_dict:
                A_vals_dict[key] = 0.0

        for key in A_vals_dict:
            if key not in B_vals_dict:
                B_vals_dict[key] = 0.0

        for key in A_vals_dict and B_vals_dict:
            residue += pow(A_vals_dict[key] - B_vals_dict[key], 2)

        return residue

    def _get_progress(self):
        return Progress(self, start=0.0, end=1.0, nreports=10)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSBeamCentreFinder)
