# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.simpleapi import CreateSampleWorkspace, CloneWorkspace, Load, Rebin
from sans_core.algorithm_detail.CreateSANSAdjustmentWorkspaces import CreateSANSAdjustmentWorkspaces
from sans_core.common.enums import DetectorType, DataType
from sans_core.test_helper.test_director import TestDirector


class CreateSANSAdjustmentWorkspacesTest(unittest.TestCase):
    sample_workspace = None
    test_tof_min = 1000
    test_tof_max = 10000
    test_tof_width = 1000
    test_wav_min = 1.0
    test_wav_max = 11.0
    test_wav_width = 2.0

    @classmethod
    def setUpClass(cls):
        _original_sans_ws = Load(Filename="SANS2D00022024", StoreInADS=False)
        cls._original_sans_ws = _original_sans_ws

    def setUp(self):
        sans_data = CloneWorkspace(InputWorkspace=self._original_sans_ws, StoreInADS=False)
        self.sans_data = sans_data

    @staticmethod
    def _get_state():
        test_director = TestDirector()
        return test_director.construct()

    @staticmethod
    def _load_workspace(filename):
        ws = Load(Filename=filename, StoreInADS=False)
        return ws

    def _get_sample_monitor_data(self, value):
        name = "test_monitor_workspace"
        monitor_workspace = CreateSampleWorkspace(
            OutputWorkspace=name,
            NumBanks=0,
            NumMonitors=8,
            XMin=self.test_tof_min,
            XMax=self.test_tof_max,
            BinWidth=self.test_tof_width,
            StoreInADS=False,
        )

        for hist in range(monitor_workspace.getNumberHistograms()):
            data_y = monitor_workspace.dataY(hist)
            for index in range(len(data_y)):
                data_y[index] = value
            # This will be the background bin
            data_y[0] = 0.1
        return monitor_workspace

    def _get_sample_data(self):
        name = "test_workspace"
        workspace = CreateSampleWorkspace(
            OutputWorkspace=name,
            NumBanks=1,
            NumMonitors=1,
            XMin=self.test_wav_min,
            XMax=self.test_wav_max,
            BinWidth=self.test_wav_width,
            XUnit="Wavelength",
            StoreInADS=False,
        )

        return workspace

    def _prepare_trans_data(self, ws, value):
        rebin_string = "{0}, {1}, {2}".format(self.test_tof_min, self.test_tof_width, self.test_tof_max)
        ws = Rebin(InputWorkspace=ws, Params=rebin_string, StoreInADS=False)
        # Set all entries to value
        for hist in range(ws.getNumberHistograms()):
            data_y = ws.dataY(hist)
            for index in range(len(data_y)):
                data_y[index] = value
            # This will be the background bin
            data_y[0] = 0.1
        return ws

    @staticmethod
    def _run_test(state, sample_data, sample_monitor_data, transmission_data, direct_data, is_lab=True, is_sample=True):
        data_type = DataType.SAMPLE.value if is_sample else DataType.CAN.value
        component = DetectorType.LAB.value if is_lab else DetectorType.HAB.value
        wav_range = state.adjustment.wavelength_and_pixel_adjustment.wavelength_interval.wavelength_full_range

        alg = CreateSANSAdjustmentWorkspaces(state=state, component=component, data_type=data_type)
        returned = alg.create_sans_adjustment_workspaces(
            direct_ws=direct_data,
            monitor_ws=sample_monitor_data,
            transmission_ws=transmission_data,
            sample_data=sample_data,
            wav_range=wav_range,
        )

        wavelength_adjustment = returned["wavelength_adj"]
        pixel_adjustment = returned["pixel_adj"]
        wavelength_and_pixel_adjustment = returned["wavelength_pixel_adj"]
        calculated_transmission = returned["calculated_trans_ws"]
        unfitted_transmission = returned["unfitted_trans_ws"]

        return wavelength_adjustment, pixel_adjustment, wavelength_and_pixel_adjustment, calculated_transmission, unfitted_transmission

    def test_that_adjustment_workspaces_are_produced_wavelength_and_wavelength_plus_pixel(self):
        # Arrange
        state = CreateSANSAdjustmentWorkspacesTest._get_state()
        state.adjustment.wide_angle_correction = True

        sample_data = self._get_sample_data()
        sample_monitor_data = self._get_sample_monitor_data(3.0)

        transmission_ws = CloneWorkspace(self.sans_data, StoreInADS=False)
        self._prepare_trans_data(ws=transmission_ws, value=1.0)

        direct_ws = CloneWorkspace(self.sans_data, StoreInADS=False)
        direct_ws = self._prepare_trans_data(ws=direct_ws, value=2.0)

        (
            wavelength_adjustment,
            pixel_adjustment,
            wavelength_and_pixel_adjustment,
            calculated_transmission,
            unfitted_transmission,
        ) = CreateSANSAdjustmentWorkspacesTest._run_test(state, sample_data, sample_monitor_data, transmission_ws, direct_ws)

        # We expect a wavelength adjustment workspace
        self.assertTrue(wavelength_adjustment)
        # We don't expect a pixel adjustment workspace since no files where specified
        self.assertFalse(pixel_adjustment)
        # We expect a wavelength and pixel adjustment workspace since we set the flag to true and provided a
        # sample data set
        self.assertTrue(wavelength_and_pixel_adjustment)
        self.assertTrue(calculated_transmission)
        self.assertTrue(unfitted_transmission)

    def test_that_transmission_runs_are_output(self):
        state = CreateSANSAdjustmentWorkspacesTest._get_state()
        state.adjustment.wide_angle_correction = True

        sample_data = self._get_sample_data()

        sample_monitor_data = self._get_sample_monitor_data(3.0)

        transmission_ws = CloneWorkspace(self.sans_data, StoreInADS=False)
        transmission_ws = self._prepare_trans_data(ws=transmission_ws, value=1.0)

        direct_ws = CloneWorkspace(self.sans_data, StoreInADS=False)
        direct_ws = self._prepare_trans_data(ws=direct_ws, value=2.0)

        (
            wavelength_adjustment,
            pixel_adjustment,
            wavelength_and_pixel_adjustment,
            calculated_transmission,
            unfitted_transmisison,
        ) = CreateSANSAdjustmentWorkspacesTest._run_test(state, sample_data, sample_monitor_data, transmission_ws, direct_ws)

        # We expect a wavelength adjustment workspace
        self.assertTrue(wavelength_adjustment)
        # We don't expect a pixel adjustment workspace since no files where specified
        self.assertFalse(pixel_adjustment)
        # We expect a wavelength and pixel adjustment workspace since we set the flag to true and provided a
        # sample data set
        self.assertTrue(wavelength_and_pixel_adjustment)
        # We expect transmission workspaces
        self.assertTrue(calculated_transmission)
        self.assertTrue(unfitted_transmisison)


if __name__ == "__main__":
    unittest.main()
