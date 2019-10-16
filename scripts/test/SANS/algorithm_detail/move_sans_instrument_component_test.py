# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.kernel import (Quat, V3D)
from mantid.simpleapi import AddSampleLog, LoadEmptyInstrument
from sans.algorithm_detail.move_sans_instrument_component import move_component, MoveTypes
from sans.algorithm_detail.move_workspaces import (create_mover, SANSMoveLOQ, SANSMoveSANS2D, SANSMoveLARMORNewStyle,
                                                   SANSMoveZOOM)
from sans.common.enums import (SANSFacility, DetectorType, SANSInstrument)
from sans.state.data import get_data_builder
from sans.state.move import get_move_builder
from sans.test_helper.file_information_mock import SANSFileInformationMock


def load_empty_instrument(instrument_name):
    sans_move_test_workspace = LoadEmptyInstrument(InstrumentName=instrument_name)
    return sans_move_test_workspace


class SANSMoveFactoryTest(unittest.TestCase):
    def _do_test(self, instrument_name, mover_type):
        # Arrange
        workspace = load_empty_instrument(instrument_name)
        # Act
        mover = create_mover(workspace)
        # Assert
        self.assertTrue(isinstance(mover, mover_type))

    def test_that_LOQ_strategy_is_selected(self):
        file_name = "LOQ"
        mover_type = SANSMoveLOQ
        self._do_test(file_name, mover_type)

    def test_that_SANS2D_strategy_is_selected(self):
        file_name = "SANS2D"
        mover_type = SANSMoveSANS2D
        self._do_test(file_name, mover_type)

    def test_that_LARMOR_new_style_strategy_is_selected(self):
        file_name = "LARMOR"
        mover_type = SANSMoveLARMORNewStyle
        self._do_test(file_name, mover_type)

    def test_that_ZOOM_strategy_is_selected(self):
        file_name = "ZOOM"
        mover_type = SANSMoveZOOM
        self._do_test(file_name, mover_type)


################################################
# Component movement tests


def compare_expected_position(instance, expected_position, expected_rotation, component, move_info, workspace):
    position, rotation = _get_position_and_rotation(workspace, move_info, component)
    for index in range(0, 3):
        instance.assertAlmostEqual(position[index], expected_position[index], delta=1e-4)
    for index in range(0, 4):
        instance.assertAlmostEqual(rotation[index], expected_rotation[index], delta=1e-4)


def check_elementry_displacement_with_translation(instance, workspace, move_info,
                                                  coordinates, component, component_key):
    # Get position and rotation before the move
    position_before_move, rotation_before_move = _get_position_and_rotation(workspace, move_info,
                                                                            component_key)
    expected_position_elementary_move = V3D(position_before_move[0] - coordinates[0],
                                            position_before_move[1] - coordinates[1],
                                            position_before_move[2])
    expected_rotation = rotation_before_move

    move_component(component_name=component, move_info=move_info, workspace=workspace,
                   beam_coordinates=coordinates, move_type=MoveTypes.ELEMENTARY_DISPLACEMENT)

    compare_expected_position(instance, expected_position_elementary_move, expected_rotation,
                              component_key, move_info, workspace)


def check_that_sets_to_zero(instance, workspace, move_info, comp_name=None):
    def _get_components_to_compare(_key, _move_info, _component_names):
        if _key in _move_info.detectors:
            _name = _move_info.detectors[_key].detector_name
            _component_names.append(_name)

    # Reset the position to zero
    move_component(component_name=comp_name, move_info=move_info, workspace=workspace,
                   move_type=MoveTypes.RESET_POSITION)

    # Get the components to compare
    if comp_name is None:
        component_names = list(move_info.monitor_names.values())
        hab_name = DetectorType.to_string(DetectorType.HAB)
        lab_name = DetectorType.to_string(DetectorType.LAB),
        _get_components_to_compare(hab_name, move_info, component_names)
        _get_components_to_compare(lab_name, move_info, component_names)
        component_names.append("some-sample-holder")
    else:
        component_names = [comp_name]

    # Ensure that the positions on the base instrument and the instrument are the same
    instrument = workspace.getInstrument()
    base_instrument = instrument.getBaseInstrument()
    for component_name in component_names:
        # Confirm that the positions are the same
        component = instrument.getComponentByName(component_name)
        base_component = base_instrument.getComponentByName(component_name)

        # If we are dealing with a monitor which has not been implemented we need to continue
        if component is None or base_component is None:
            continue

        position = component.getPos()
        position_base = base_component.getPos()
        for index in range(0, 3):
            instance.assertAlmostEquals(position[index], position_base[index], delta=1e-4)
        rotation = component.getRotation()
        rotation_base = base_component.getRotation()
        for index in range(0, 4):
            instance.assertAlmostEquals(rotation[index], rotation_base[index], delta=1e-4)


def _get_state_move_obj(instrument, x_translation=None, z_translation=None):
    facility = SANSFacility.ISIS
    file_information = SANSFileInformationMock(instrument=instrument, run_number=123)
    data_builder = get_data_builder(facility, file_information)
    data_builder.set_sample_scatter("NotSet")
    data_builder.set_sample_scatter_period(3)
    data_info = data_builder.build()

    state_builder = get_move_builder(data_info)
    if x_translation:
        state_builder.set_LAB_x_translation_correction(x_translation)
    if z_translation:
        state_builder.set_LAB_z_translation_correction(z_translation)

    state = state_builder.build()
    return state


def _get_position_and_rotation(workspace, move_info, component):
    instrument = workspace.getInstrument()
    component_name = move_info.detectors[component].detector_name
    detector = instrument.getComponentByName(component_name)
    position = detector.getPos()
    rotation = detector.getRotation()
    return position, rotation


class LOQMoveTest(unittest.TestCase):
    def test_that_LOQ_can_perform_move(self):
        lab_x_translation_correction = 123.
        beam_coordinates = [45, 25]
        component = "main-detector-bank"
        component_key = DetectorType.to_string(DetectorType.LAB)

        workspace = load_empty_instrument("LOQ")
        move_info = _get_state_move_obj(SANSInstrument.LOQ, lab_x_translation_correction)

        # Initial move
        move_component(component_name=component, workspace=workspace, move_info=move_info,
                       move_type=MoveTypes.INITIAL_MOVE, beam_coordinates=beam_coordinates)

        center_position = move_info.center_position
        initial_z_position = 15.15
        expected_position = V3D(center_position - beam_coordinates[0] + lab_x_translation_correction,
                                center_position - beam_coordinates[1],
                                initial_z_position)
        expected_rotation = Quat(1., 0., 0., 0.)
        compare_expected_position(self, expected_position, expected_rotation, component_key, move_info, workspace)

        # Elementary Move
        component_elementary_move = "HAB"
        component_elementary_move_key = DetectorType.to_string(DetectorType.HAB)

        beam_coordinates_elementary_move = [120, 135]
        check_elementry_displacement_with_translation(self, workspace, move_info,
                                                      beam_coordinates_elementary_move,
                                                      component_elementary_move,
                                                      component_elementary_move_key)

        # Reset to zero
        check_that_sets_to_zero(self, workspace, move_info=move_info)


class SANS2DMoveTest(unittest.TestCase):
    @staticmethod
    def _prepare_sans2d_empty_ws():
        workspace = load_empty_instrument("SANS2D")

        # Taken from SANS2D00028784
        AddSampleLog(Workspace=workspace, LogName="Front_Det_X", LogText="-859.968")
        AddSampleLog(Workspace=workspace, LogName="Front_Det_Z", LogText="5001.97")
        AddSampleLog(Workspace=workspace, LogName="Front_Det_ROT", LogText="-9.02552")
        AddSampleLog(Workspace=workspace, LogName="Rear_Det_X", LogText="100.048")
        AddSampleLog(Workspace=workspace, LogName="Rear_Det_Z", LogText="11999.0")
        return workspace

    def test_that_SANS2D_can_move(self):
        workspace = self._prepare_sans2d_empty_ws()

        lab_z_translation_correction = 123.
        beam_coordinates = [0.1076, -0.0835]

        # All detectors are moved on SANS2D
        component = None

        move_type = MoveTypes.INITIAL_MOVE
        move_info = _get_state_move_obj(SANSInstrument.SANS2D, z_translation=lab_z_translation_correction)

        move_component(component_name=component, workspace=workspace, move_info=move_info,
                       move_type=MoveTypes.INITIAL_MOVE, beam_coordinates=beam_coordinates)

        # Assert for initial move for low angle bank
        # These values are on the workspace and in the sample logs,
        component_to_investigate = DetectorType.to_string(DetectorType.LAB)
        initial_z_position = 23.281
        rear_det_z = 11.9989755859
        offset = 4.
        total_x = 0.
        total_y = 0.
        total_z = initial_z_position + rear_det_z - offset + lab_z_translation_correction
        expected_position = V3D(total_x - beam_coordinates[0], total_y - beam_coordinates[1], total_z)
        expected_rotation = Quat(1., 0., 0., 0.)
        compare_expected_position(self, expected_position, expected_rotation,
                                  component_to_investigate, move_info, workspace)

        # Assert for initial move for high angle bank
        # These values are on the workspace and in the sample logs
        component_to_investigate = DetectorType.to_string(DetectorType.HAB)
        initial_x_position = 1.1
        x_correction = -0.187987540973
        initial_z_position = 23.281
        z_correction = 1.00575649188
        total_x = initial_x_position + x_correction
        total_y = 0.
        total_z = initial_z_position + z_correction
        expected_position = V3D(total_x - beam_coordinates[0], total_y - beam_coordinates[1], total_z)
        expected_rotation = Quat(0.9968998362876025, 0., 0.07868110579898738, 0.)
        compare_expected_position(self, expected_position, expected_rotation,
                                  component_to_investigate, move_info, workspace)

        # Act + Assert for elementary move
        component_elementary_move = "rear-detector"
        component_elementary_move_key = DetectorType.to_string(DetectorType.LAB)
        beam_coordinates_elementary_move = [120, 135]
        check_elementry_displacement_with_translation(self, workspace, move_info,
                                                      beam_coordinates_elementary_move,
                                                      component_elementary_move,
                                                      component_elementary_move_key)

        # # Act + Assert for setting to zero position for all
        check_that_sets_to_zero(self, workspace, move_info, comp_name=None)

    def test_that_missing_beam_centre_is_taken_from_move_state(self):
        lab_z_translation_correction = 123.

        workspace = self._prepare_sans2d_empty_ws()
        move_info = _get_state_move_obj(instrument=SANSInstrument.SANS2D,
                                        z_translation=lab_z_translation_correction)

        # These values should be used instead of an explicitly specified beam centre
        move_info.detectors[DetectorType.to_string(DetectorType.HAB)].sample_centre_pos1 = 26.
        move_info.detectors[DetectorType.to_string(DetectorType.HAB)].sample_centre_pos2 = 98.

        # The component input is not relevant for SANS2D's initial move. All detectors are moved
        component = "front-detector"
        move_component(component_name=component, move_info=move_info, workspace=workspace,
                       move_type=MoveTypes.INITIAL_MOVE)

        # These values are on the workspace and in the sample logs
        component_to_investigate = DetectorType.to_string(DetectorType.HAB)
        initial_x_position = 1.1
        x_correction = -0.187987540973
        initial_z_position = 23.281
        z_correction = 1.00575649188
        total_x = initial_x_position + x_correction
        total_y = 0.
        total_z = initial_z_position + z_correction
        expected_position = V3D(total_x - 26., total_y - 98., total_z)
        expected_rotation = Quat(0.9968998362876025, 0., 0.07868110579898738, 0.)
        compare_expected_position(self, expected_position, expected_rotation,
                                  component_to_investigate, move_info, workspace)

    def test_that_missing_beam_centre_is_taken_from_lab_move_state_when_no_component_is_specified(self):
        lab_z_translation_correction = 123.

        workspace = self._prepare_sans2d_empty_ws()
        move_info = _get_state_move_obj(instrument=SANSInstrument.SANS2D,
                                        z_translation=lab_z_translation_correction)

        # These values should be used instead of an explicitly specified beam centre
        x_offset = 26.
        y_offset = 98.
        move_info.detectors[DetectorType.to_string(DetectorType.LAB)].sample_centre_pos1 = x_offset
        move_info.detectors[DetectorType.to_string(DetectorType.LAB)].sample_centre_pos2 = y_offset

        # The component input is not relevant for SANS2D's initial move. All detectors are moved
        component = None
        move_component(component_name=component, move_info=move_info, workspace=workspace,
                       move_type=MoveTypes.INITIAL_MOVE)

        # Assert for initial move for low angle bank
        # These values are on the workspace and in the sample logs,
        component_to_investigate = DetectorType.to_string(DetectorType.LAB)
        initial_z_position = 23.281
        rear_det_z = 11.9989755859
        offset = 4.
        total_x = 0.
        total_y = 0.
        total_z = initial_z_position + rear_det_z - offset + lab_z_translation_correction
        expected_position = V3D(total_x - x_offset, total_y - y_offset, total_z)
        expected_rotation = Quat(1., 0., 0., 0.)
        compare_expected_position(self, expected_position, expected_rotation,
                                  component_to_investigate, move_info, workspace)


class LARMORMoveTest(unittest.TestCase):
    def test_that_LARMOR_new_style_can_move(self):
        lab_x_translation_correction = 123.

        workspace = load_empty_instrument("LARMOR")

        # Taken from LARMOR00002260
        AddSampleLog(Workspace=workspace, LogName="Bench_Rot", LogText="0.000973305")

        # Note that the first entry is an angle while the second is a translation (in meter)
        beam_coordinates = [24., 38.]

        # Act for initial move
        component = None
        move_info = _get_state_move_obj(instrument=SANSInstrument.LARMOR,
                                        x_translation=lab_x_translation_correction)
        move_component(component_name=component, workspace=workspace, move_info=move_info,
                       move_type=MoveTypes.INITIAL_MOVE, beam_coordinates=beam_coordinates)

        # Assert low angle bank for initial move
        # These values are on the workspace and in the sample logs
        component_to_investigate = DetectorType.to_string(DetectorType.LAB)

        # The rotation couples the movements, hence we just insert absolute value, to have a type of regression test.
        expected_position = V3D(0, -38, 25.3)
        expected_rotation = Quat(0.978146, 0, -0.20792, 0)
        compare_expected_position(self, expected_position, expected_rotation,
                                  component_to_investigate, move_info, workspace)

        check_that_sets_to_zero(self, workspace, move_info, comp_name=None)


class ZOOMMoveTest(unittest.TestCase):
    def test_that_ZOOM_can_perform_move(self):
        beam_coordinates = [45, 25]
        component = "main-detector-bank"
        component_key = DetectorType.to_string(DetectorType.LAB)

        workspace = load_empty_instrument("ZOOM")
        move_info = _get_state_move_obj(SANSInstrument.ZOOM)

        # Initial move
        move_component(component_name=component, workspace=workspace, move_info=move_info,
                       move_type=MoveTypes.INITIAL_MOVE, beam_coordinates=beam_coordinates)

        initial_z_position = 20.77408
        expected_position = V3D(-beam_coordinates[0],
                                -beam_coordinates[1],
                                initial_z_position)
        expected_rotation = Quat(1., 0., 0., 0.)
        compare_expected_position(self, expected_position, expected_rotation, component_key, move_info, workspace)

        # Elementary Move
        component_elementary_move = "LAB"
        component_elementary_move_key = DetectorType.to_string(DetectorType.LAB)

        beam_coordinates_elementary_move = [120, 135]
        check_elementry_displacement_with_translation(self, workspace, move_info,
                                                      beam_coordinates_elementary_move,
                                                      component_elementary_move,
                                                      component_elementary_move_key)

        # Reset to zero
        check_that_sets_to_zero(self, workspace, move_info=move_info)


if __name__ == '__main__':
    unittest.main()
