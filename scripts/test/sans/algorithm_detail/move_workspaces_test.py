# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import mantid.simpleapi
from sans.algorithm_detail.move_workspaces import SANSMoveZOOM, SANSMoveSANS2D
from sans.common.enums import SANSFacility, SANSInstrument, DetectorType
from sans.state.AllStates import AllStates
from sans.state.StateObjects.StateData import get_data_builder
from sans.state.StateObjects.StateMoveDetectors import get_move_builder
from sans.state.StateObjects.state_instrument_info import StateInstrumentInfo
from sans.test_helper.file_information_mock import SANSFileInformationMock


def get_coordinates():
    # Co-ords are used for low angle bank movements
    return [0, 0]


def get_monitor_pos(ws, monitor_spectrum_no, inst_info):
    monitor_name = inst_info.monitor_names[str(monitor_spectrum_no)]

    comp_info = ws.componentInfo()
    monitor_index = comp_info.indexOfAny(monitor_name)
    monitor_z_pos = comp_info.position(monitor_index).getZ()
    return monitor_z_pos


def calculate_new_pos_rel_to_rear(ws, inst_info, offset):
    # Calculates the new position relative to the rear detector offset
    rear_detector_z = get_rear_detector_pos(inst_info=inst_info, ws=ws)
    expected_pos = rear_detector_z + offset
    return expected_pos


def get_rear_detector_pos(inst_info, ws):
    lab_detector = inst_info.detector_names[DetectorType.LAB.value]
    detector_name = lab_detector.detector_name
    comp_info = ws.componentInfo()
    lab_detector_index = comp_info.indexOfAny(detector_name)
    lab_detector_component = comp_info.position(lab_detector_index)
    detector_position = lab_detector_component.getZ()
    return detector_position


class MoveSans2DMonitor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sans_ws = mantid.simpleapi.LoadEmptyInstrument(InstrumentName="SANS2D")
        log_names = ["Front_Det_X", "Front_Det_Z", "Front_Det_ROT", "Rear_Det_X", "Rear_Det_Z"]
        log_values = [0.0 for _ in log_names]
        mantid.simpleapi.AddSampleLogMultiple(Workspace=sans_ws, LogNames=log_names, LogValues=log_values)

        cls.sans_ws = sans_ws

    @classmethod
    def tearDownClass(cls):
        mantid.simpleapi.DeleteWorkspace(cls.sans_ws)
        cls.sans_ws = None

    @classmethod
    def get_sans_workspace(cls):
        return cls.sans_ws

    @staticmethod
    def get_state_obj(monitor_4_dist):
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.SANS2D, run_number=22048)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("SANS2D22048")
        data_builder.set_sample_scatter_period(3)
        data_info = data_builder.build()

        state_builder = get_move_builder(data_info)

        state = AllStates()
        state.move = state_builder.build()
        state.instrument_info = StateInstrumentInfo.build_from_data_info(data_info)

        state.move.monitor_4_offset = monitor_4_dist
        return state

    def test_move_sans_monitors(self):
        # Unused for unit test
        component = None
        is_transmission_workspace = None

        mon_4_dist = 15.0

        state = self.get_state_obj(monitor_4_dist=mon_4_dist)

        zoom_class = SANSMoveSANS2D(state=state)
        workspace = self.get_sans_workspace()
        coordinates = get_coordinates()

        inst_info = state.instrument_info
        z_pos_mon_4_before = get_monitor_pos(ws=workspace, monitor_spectrum_no=4, inst_info=inst_info)

        self.assertTrue(isinstance(z_pos_mon_4_before, float))

        zoom_class.move_initial(
            workspace=workspace, coordinates=coordinates, component=component, is_transmission_workspace=is_transmission_workspace
        )

        z_pos_mon_4_after = get_monitor_pos(ws=workspace, monitor_spectrum_no=4, inst_info=inst_info)

        self.assertAlmostEqual(calculate_new_pos_rel_to_rear(ws=workspace, inst_info=inst_info, offset=mon_4_dist), z_pos_mon_4_after)

    def test_not_moving_monitors(self):
        # Unused for unit test
        component = None
        is_transmission_workspace = None

        mon_4_dist = 0.0

        state = self.get_state_obj(monitor_4_dist=mon_4_dist)
        zoom_class = SANSMoveSANS2D(state=state)
        workspace = self.get_sans_workspace()
        coordinates = get_coordinates()
        inst_info = state.instrument_info

        z_pos_mon_4_before = get_monitor_pos(ws=workspace, monitor_spectrum_no=4, inst_info=inst_info)

        self.assertTrue(isinstance(z_pos_mon_4_before, float))

        zoom_class.move_initial(
            workspace=workspace, coordinates=coordinates, component=component, is_transmission_workspace=is_transmission_workspace
        )

        z_pos_mon_4_after = get_monitor_pos(ws=workspace, monitor_spectrum_no=4, inst_info=inst_info)

        self.assertAlmostEqual(calculate_new_pos_rel_to_rear(ws=workspace, offset=mon_4_dist, inst_info=inst_info), z_pos_mon_4_after)


class MoveZoomMonitors(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        zoom_empty = mantid.simpleapi.LoadEmptyInstrument(InstrumentName="Zoom")
        cls.zoom_empty = zoom_empty

    @classmethod
    def tearDownClass(cls):
        mantid.simpleapi.DeleteWorkspace(cls.zoom_empty)
        cls.zoom_empty = None

    @classmethod
    def get_zoom_workspace(cls):
        return cls.zoom_empty

    @staticmethod
    def get_state_obj(monitor_4_dist, monitor_5_dist):
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.ZOOM, run_number=6113)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("ZOOM00006113")
        data_builder.set_sample_scatter_period(3)
        data_info = data_builder.build()

        state = AllStates()
        state.move = get_move_builder(data_info).build()
        state.instrument_info = StateInstrumentInfo.build_from_data_info(data_info)
        state.move.monitor_4_offset = monitor_4_dist
        state.move.monitor_5_offset = monitor_5_dist
        return state

    def test_move_both_zoom_monitors(self):
        # Unused for unit test
        component = None
        is_transmission_workspace = None

        mon_4_dist = 40.0
        mon_5_dist = 20.0

        state = self.get_state_obj(monitor_4_dist=mon_4_dist, monitor_5_dist=mon_5_dist)
        zoom_class = SANSMoveZOOM(state=state)
        workspace = self.get_zoom_workspace()
        coordinates = get_coordinates()

        inst_info = state.instrument_info
        z_pos_mon_4_before = get_monitor_pos(ws=workspace, monitor_spectrum_no=4, inst_info=inst_info)
        z_pos_mon_5_before = get_monitor_pos(ws=workspace, monitor_spectrum_no=5, inst_info=inst_info)

        self.assertTrue(isinstance(z_pos_mon_4_before, float))
        self.assertTrue(isinstance(z_pos_mon_5_before, float))

        zoom_class.move_initial(
            workspace=workspace, coordinates=coordinates, component=component, is_transmission_workspace=is_transmission_workspace
        )

        # Monitor 4 shifts relative to itself rather than the rear detector on ZOOM
        z_pos_mon_4_after = get_monitor_pos(ws=workspace, monitor_spectrum_no=4, inst_info=inst_info)
        z_pos_mon_5_after = get_monitor_pos(ws=workspace, monitor_spectrum_no=5, inst_info=inst_info)

        self.assertAlmostEqual(z_pos_mon_4_before + mon_4_dist, z_pos_mon_4_after)
        self.assertAlmostEqual(calculate_new_pos_rel_to_rear(ws=workspace, offset=mon_5_dist, inst_info=inst_info), z_pos_mon_5_after)

    def test_moving_only_monitor_4(self):
        component = None
        is_transmission_workspace = None

        mon_4_dist = 40.0
        mon_5_dist = 0.0

        state = self.get_state_obj(monitor_4_dist=mon_4_dist, monitor_5_dist=mon_5_dist)
        zoom_class = SANSMoveZOOM(state)
        workspace = self.get_zoom_workspace()
        coordinates = get_coordinates()

        inst_info = state.instrument_info
        z_pos_mon_4_before = get_monitor_pos(ws=workspace, monitor_spectrum_no=4, inst_info=inst_info)
        z_pos_mon_5_before = get_monitor_pos(ws=workspace, monitor_spectrum_no=5, inst_info=inst_info)

        self.assertTrue(isinstance(z_pos_mon_4_before, float))
        self.assertTrue(isinstance(z_pos_mon_5_before, float))

        zoom_class.move_initial(
            workspace=workspace, coordinates=coordinates, component=component, is_transmission_workspace=is_transmission_workspace
        )

        # Monitor 4 shifts relative to itself rather than the rear detector on ZOOM
        z_pos_mon_4_after = get_monitor_pos(ws=workspace, monitor_spectrum_no=4, inst_info=inst_info)
        z_pos_mon_5_after = get_monitor_pos(ws=workspace, monitor_spectrum_no=5, inst_info=inst_info)

        self.assertAlmostEqual(z_pos_mon_4_before + mon_4_dist, z_pos_mon_4_after)
        self.assertAlmostEqual(calculate_new_pos_rel_to_rear(ws=workspace, offset=mon_5_dist, inst_info=inst_info), z_pos_mon_5_after)

    def test_moving_only_monitor_5(self):
        component = None
        is_transmission_workspace = None

        mon_4_dist = 0.0
        mon_5_dist = 80.0

        state = self.get_state_obj(monitor_4_dist=mon_4_dist, monitor_5_dist=mon_5_dist)
        zoom_class = SANSMoveZOOM(state=state)
        workspace = self.get_zoom_workspace()
        coordinates = get_coordinates()

        inst_info = state.instrument_info
        z_pos_mon_4_before = get_monitor_pos(ws=workspace, monitor_spectrum_no=4, inst_info=inst_info)
        z_pos_mon_5_before = get_monitor_pos(ws=workspace, monitor_spectrum_no=5, inst_info=inst_info)

        self.assertTrue(isinstance(z_pos_mon_4_before, float))
        self.assertTrue(isinstance(z_pos_mon_5_before, float))

        zoom_class.move_initial(
            workspace=workspace, coordinates=coordinates, component=component, is_transmission_workspace=is_transmission_workspace
        )

        z_pos_mon_4_after = get_monitor_pos(ws=workspace, monitor_spectrum_no=4, inst_info=inst_info)
        z_pos_mon_5_after = get_monitor_pos(ws=workspace, monitor_spectrum_no=5, inst_info=inst_info)

        self.assertAlmostEqual(z_pos_mon_4_before, z_pos_mon_4_after)
        self.assertAlmostEqual(calculate_new_pos_rel_to_rear(ws=workspace, offset=mon_5_dist, inst_info=inst_info), z_pos_mon_5_after)


if __name__ == "__main__":
    unittest.main()
