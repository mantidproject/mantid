# pylint: disable=too-few-public-methods

""" Finds the beam centre for SANS"""

from __future__ import (absolute_import, division, print_function)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress, IEventWorkspace, ITableWorkspace)
from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator, Logger)
from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import create_child_algorithm, append_to_sans_file_tag
from sans.state.state_base import create_deserialized_sans_state_from_property_manager
from sans.common.enums import (DetectorType, DataType, MaskingQuadrant, FindDirectionEnum)
from sans.algorithm_detail.crop_helper import get_component_name
from sans.algorithm_detail.strip_end_nans_and_infs import strip_end_nans
from sans.common.file_information import get_instrument_paths_for_sans_file
from sans.common.xml_parsing import get_named_elements_from_ipf_file
C
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
                             doc='The sample scatter data')

        self.declareProperty(MatrixWorkspaceProperty("SampleTransmissionWorkspace", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The sample transmission data')

        self.declareProperty(MatrixWorkspaceProperty("SampleDirectWorkspace", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The sample direct data')

        self.declareProperty(MatrixWorkspaceProperty("CanScatterWorkspace", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The sample scatter data')

        self.declareProperty(MatrixWorkspaceProperty("CanScatterMonitorWorkspace", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The sample scatter data')

        self.declareProperty(MatrixWorkspaceProperty("CanTransmissionWorkspace", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The sample transmission data')

        self.declareProperty(MatrixWorkspaceProperty("CanDirectWorkspace", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The sample transmission data')

        # The component, i.e. HAB or LAB
        allowed_detectors = StringListValidator([DetectorType.to_string(DetectorType.LAB),
                                                 DetectorType.to_string(DetectorType.HAB)])
        self.declareProperty("Component", DetectorType.to_string(DetectorType.LAB),
                             validator=allowed_detectors, direction=Direction.Input,
                             doc="The component of the instrument which is to be reduced.")

        self.declareProperty("Iterations", 10, direction=Direction.Input)

        self.declareProperty("RMin", 0.6, direction=Direction.Input)

        self.declareProperty('RMax', 0.28, direction=Direction.Input)

        self.declareProperty('Position1Start', 0.0, direction=Direction.Input)

        self.declareProperty('Position2Start', 0.0, direction=Direction.Input)

        self.declareProperty('Tolerance', 0.0001251, direction=Direction.Input)

        self.declareProperty('Direction', FindDirectionEnum.to_string(FindDirectionEnum.All), direction=Direction.Input)

        # ----------
        # Output
        # ----------
        # Workspace which is to be cropped

        self.declareProperty('Centre1', 0.0, direction=Direction.Output)
        self.declareProperty('Centre2', 0.0, direction=Direction.Output)

    def PyExec(self):
        state = self._get_state()
        state_serialized = state.property_manager
        # Need to set the starting step size
        logger = Logger("CentreFinder")
        progress = self._get_progress()

        # Need to set the scale factors for the x and y directions. These are to convert between units.
        scale_x = 1
        scale_y = 1

        # Set the start positions
        x_start = self.getProperty("Position1Start").value
        y_start = self.getProperty("Position2Start").value

        sample_scatter = self._get_cloned_workspace("SampleScatterWorkspace")
        sample_scatter_monitor = self._get_cloned_workspace("SampleScatterMonitorWorkspace")
        sample_transmission = self._get_cloned_workspace("SampleTransmissionWorkspace")
        sample_direct = self._get_cloned_workspace("SampleDirectWorkspace")

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
        position_1_step = get_named_elements_from_ipf_file(instrument_file[1], "centre-finder-step-size", float)['centre-finder-step-size']
        position_2_step = get_named_elements_from_ipf_file(instrument_file[1], "centre-finder-step-size", float)['centre-finder-step-size']

        find_direction = self.getProperty("Direction").value
        if find_direction == FindDirectionEnum.to_string(FindDirectionEnum.Left_Right):
            position_2_step = 0.0
        elif find_direction == FindDirectionEnum.to_string(FindDirectionEnum.Up_Down):
            position_1_step = 0.0

        centre1 = x_start
        centre2 = y_start
        for j in range(0, max_iterations + 1):
            progress.report("Reducing ... Pos1 " + str(centre1) + " Pos2 " + str(centre2))
            sample_quartiles = self._run_quartile_reduction(sample_scatter, sample_transmission, sample_direct,
                                                            "Sample", sample_scatter_monitor, component, state_serialized, centre1,
                                                            centre2, r_min, r_max)

            if can_scatter:
                can_quartiles = self._run_quartile_reduction(can_scatter, can_transmission, can_direct, "Can",
                                                            can_scatter_monitor, component, state_serialized, centre1, centre2, r_min, r_max)
                for i in range(0,3):
                    sample_quartiles -= can_quartiles

            residueLR = self._calculate_residuals(sample_quartiles[0], sample_quartiles[1])
            residueTB = self._calculate_residuals(sample_quartiles[2], sample_quartiles[3])
            if(j == 0):
                resLR_old = residueLR
                resTB_old = residueTB
                logger.notice("Iteration " + str(j) + "  PosX " + str(centre1) + "  PosY " + str(centre2) + "  ResX=" + str(
                    residueLR) + "  ResY=" + str(residueTB))
            else:
                # have we stepped across the y-axis that goes through the beam center?
                if residueLR > resLR_old:
                    # yes with stepped across the middle, reverse direction and half the step size
                    position_1_step = - position_1_step / 2
                if residueTB > resTB_old:
                    position_2_step = - position_2_step / 2

                logger.notice("Iteration " + str(j) + "  PosX " + str(centre1) + "  PosY " + str(centre2) + "  ResX=" + str(
                    residueLR) + "  ResY=" + str(residueTB))

                if abs(position_1_step) < tolerance and abs(position_2_step) < tolerance:
                    # this is the success criteria, we've close enough to the center
                    logger.notice("Converged - check if stuck in local minimum! ")
                    break

            centre1 += position_1_step
            centre2 += position_2_step
            resLR_old = residueLR
            resTB_old = residueTB

        self.setProperty("Centre1", centre1)
        self.setProperty("Centre2", centre2)

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
        return [out_left, out_right, out_top, out_bottom]

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
        qrangeX = [len(yvalsAX), len(yvalsBX)]
        nvalsX = min(qrangeX)
        residue = self._residual_calculation_for_single_direction(yvalsA=yvalsAX,
                                                                   yvalsB=yvalsBX,
                                                                   qvalsA=qvalsAX,
                                                                   qvalsB=qvalsBX,
                                                                   nvals=nvalsX,
                                                                   )
        return residue

    def _residual_calculation_for_single_direction(self, yvalsA, yvalsB, qvalsA, qvalsB, nvals):
        residue = 0
        indexB = 0
        for indexA in range(0, nvals):
            if qvalsA[indexA] < qvalsB[indexB]:
                #self.logger.notice(id1 + " " +str(indexA)+" "+str(indexB))
                continue
            elif qvalsA[indexA] > qvalsB[indexB]:
                while qvalsA[indexA] > qvalsB[indexB]:
                    #self.logger(id2 + " " +str(indexA)+" "+str(indexB))
                    indexB += 1
            if indexA > nvals - 1 or indexB > nvals - 1:
                break
            residue += pow(yvalsA[indexA] - yvalsB[indexB], 2)
            indexB += 1
        return residue

    def _get_progress(self):
        return Progress(self, start=0.0, end=1.0, nreports=10)



# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSBeamCentreFinder)
