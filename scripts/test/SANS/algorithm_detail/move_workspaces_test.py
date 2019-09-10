# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

import mantid.simpleapi
from sans.algorithm_detail.move_workspaces import SANSMoveZOOM
from sans.common.enums import SANSFacility, SANSInstrument, DetectorType
from sans.state.data import get_data_builder
from sans.state.move import StateMoveZOOM, get_move_builder
from sans.test_helper.file_information_mock import SANSFileInformationMock

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
    def get_coordinates():
        # Co-ords are used for low angle bank movements
        return [0, 0]

    @staticmethod
    def get_state_move_obj(monitor_4_dist, monitor_5_dist):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.ZOOM, run_number=6113)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("ZOOM00006113")
        data_builder.set_sample_scatter_period(3)
        data_info = data_builder.build()

        # Act
        state_builder = get_move_builder(data_info)

        # Assert
        state = state_builder.build()
        state.monitor_n_offset = {4: monitor_4_dist, 5: monitor_5_dist}
        return state

    @staticmethod
    def get_rear_detector_pos(move_info, instrument):
        lab_detector = move_info.detectors[DetectorType.to_string(DetectorType.LAB)]
        detector_name = lab_detector.detector_name
        lab_detector_component = instrument.getComponentByName(detector_name)
        detector_position = lab_detector_component.getPos()
        return detector_position.getZ()

    @staticmethod
    def get_monitor_pos(ws, monitor_spectrum_no, move_info):
        monitor_name = move_info.monitor_names[str(monitor_spectrum_no)]
        instrument = ws.getInstrument()
        monitor = instrument.getComponentByName(monitor_name)

        monitor_z_pos = monitor.getPos().getZ()
        return monitor_z_pos

    def calculate_new_pos(self, ws, move_info, offset):
        rear_detector_z = self.get_rear_detector_pos(move_info=move_info, instrument=ws.getInstrument())
        expected_pos = rear_detector_z + offset
        return expected_pos

    def test_move_both_zoom_monitors(self):
        zoom_class = SANSMoveZOOM()

        # Unused on ZOOM
        component = None
        is_transmission_workspace = None

        mon_4_dist = 40.0
        mon_5_dist = 20.0

        move_info = self.get_state_move_obj(monitor_4_dist=mon_4_dist, monitor_5_dist=mon_5_dist)
        workspace = self.get_zoom_workspace()
        coordinates = self.get_coordinates()

        z_pos_mon_4_before = self.get_monitor_pos(ws=workspace, monitor_spectrum_no=4, move_info=move_info)
        z_pos_mon_5_before = self.get_monitor_pos(ws=workspace, monitor_spectrum_no=5, move_info=move_info)

        self.assertTrue(isinstance(z_pos_mon_4_before, float))
        self.assertTrue(isinstance(z_pos_mon_5_before, float))

        zoom_class.move_initial(move_info=move_info, workspace=workspace, coordinates=coordinates,
                                component=component, is_transmission_workspace=is_transmission_workspace)

        z_pos_mon_4_after = self.get_monitor_pos(ws=workspace, monitor_spectrum_no=4, move_info=move_info)
        z_pos_mon_5_after = self.get_monitor_pos(ws=workspace, monitor_spectrum_no=5, move_info=move_info)

        self.assertAlmostEqual(self.calculate_new_pos(ws=workspace, move_info=move_info, offset=mon_4_dist),
                               z_pos_mon_4_after)
        self.assertAlmostEqual(self.calculate_new_pos(ws=workspace, move_info=move_info, offset=mon_5_dist),
                               z_pos_mon_5_after)

    def test_moving_only_monitor_4(self):
        zoom_class = SANSMoveZOOM()

        # Unused on ZOOM
        component = None
        is_transmission_workspace = None

        mon_4_dist = 40.0
        mon_5_dist = 0.0

        move_info = self.get_state_move_obj(monitor_4_dist=mon_4_dist, monitor_5_dist=mon_5_dist)
        workspace = self.get_zoom_workspace()
        coordinates = self.get_coordinates()

        z_pos_mon_4_before = self.get_monitor_pos(ws=workspace, monitor_spectrum_no=4, move_info=move_info)
        z_pos_mon_5_before = self.get_monitor_pos(ws=workspace, monitor_spectrum_no=5, move_info=move_info)

        self.assertTrue(isinstance(z_pos_mon_4_before, float))
        self.assertTrue(isinstance(z_pos_mon_5_before, float))

        zoom_class.move_initial(move_info=move_info, workspace=workspace, coordinates=coordinates,
                                component=component, is_transmission_workspace=is_transmission_workspace)

        z_pos_mon_4_after = self.get_monitor_pos(ws=workspace, monitor_spectrum_no=4, move_info=move_info)
        z_pos_mon_5_after = self.get_monitor_pos(ws=workspace, monitor_spectrum_no=5, move_info=move_info)

        self.assertAlmostEqual(self.calculate_new_pos(ws=workspace, move_info=move_info, offset=mon_4_dist),
                               z_pos_mon_4_after)
        self.assertAlmostEqual(z_pos_mon_5_before, z_pos_mon_5_after)

    def test_moving_only_monitor_5(self):
        zoom_class = SANSMoveZOOM()

        # Unused on ZOOM
        component = None
        is_transmission_workspace = None

        mon_4_dist = 0.0
        mon_5_dist = 80.0

        move_info = self.get_state_move_obj(monitor_4_dist=mon_4_dist, monitor_5_dist=mon_5_dist)
        workspace = self.get_zoom_workspace()
        coordinates = self.get_coordinates()

        z_pos_mon_4_before = self.get_monitor_pos(ws=workspace, monitor_spectrum_no=4, move_info=move_info)
        z_pos_mon_5_before = self.get_monitor_pos(ws=workspace, monitor_spectrum_no=5, move_info=move_info)

        self.assertTrue(isinstance(z_pos_mon_4_before, float))
        self.assertTrue(isinstance(z_pos_mon_5_before, float))

        zoom_class.move_initial(move_info=move_info, workspace=workspace, coordinates=coordinates,
                                component=component, is_transmission_workspace=is_transmission_workspace)

        z_pos_mon_4_after = self.get_monitor_pos(ws=workspace, monitor_spectrum_no=4, move_info=move_info)
        z_pos_mon_5_after = self.get_monitor_pos(ws=workspace, monitor_spectrum_no=5, move_info=move_info)

        self.assertAlmostEqual(z_pos_mon_4_before, z_pos_mon_4_after)
        self.assertAlmostEqual(self.calculate_new_pos(ws=workspace, move_info=move_info, offset=mon_5_dist),
                               z_pos_mon_5_after)



if __name__ == "__main__":
    unittest.main()
