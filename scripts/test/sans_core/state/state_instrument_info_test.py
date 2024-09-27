# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from sans_core.common.enums import SANSInstrument, DetectorType
from sans_core.state.StateObjects import state_instrument_info
from sans_core.state.StateObjects.state_instrument_info import StateInstrumentInfo


class StateInstrumentInfoTest(unittest.TestCase):
    def test_invalid_monitor_types(self):
        has_no_hab = (SANSInstrument.LARMOR, SANSInstrument.ZOOM)
        for i in has_no_hab:
            invalid_types = state_instrument_info._get_invalid_monitor_types(i)
            self.assertEqual([DetectorType.HAB], invalid_types)

        for i in SANSInstrument:
            if i in has_no_hab:
                continue
            self.assertIsNone(state_instrument_info._get_invalid_monitor_types(i))

    def test_invalid_monitors(self):
        has_invalid_monitors = (SANSInstrument.LARMOR, SANSInstrument.SANS2D, SANSInstrument.ZOOM)
        for i in SANSInstrument:
            returned = state_instrument_info._get_invalid_monitor_names(i)

            if i in has_invalid_monitors:
                self.assertTrue(len(returned) >= 1)
            else:
                self.assertIsNone(returned)

    @staticmethod
    def _get_ipf_names():
        return {
            "lab_short": "low-angle-detector-short-name",
            "lab_full": "low-angle-detector-name",
            "hab_short": "high-angle-detector-short-name",
            "hab_full": "high-angle-detector-name",
        }

    def _prep_mock_det_ipf(self):
        lab_short, lab_full = mock.NonCallableMock(), mock.NonCallableMock()
        hab_short, hab_full = mock.NonCallableMock(), mock.NonCallableMock()
        ipf_names = self._get_ipf_names()
        mocked_ipf = {
            ipf_names["lab_short"]: lab_short,
            ipf_names["lab_full"]: lab_full,
            ipf_names["hab_short"]: hab_short,
            ipf_names["hab_full"]: hab_full,
        }
        return mocked_ipf

    @mock.patch("state_instrument_info_test.state_instrument_info.get_named_elements_from_ipf_file")
    def test_setting_det_names(self, mocked_ipf_loader):
        mocked_ipf_data = self._prep_mock_det_ipf()
        mocked_ipf_loader.return_value = mocked_ipf_data
        mocked_data_info = mock.NonCallableMock()

        inst_info = StateInstrumentInfo()
        state_instrument_info._set_detector_names(inst_info, mocked_data_info)
        ipf_names = self._get_ipf_names()

        lab_values = inst_info.detector_names[DetectorType.LAB.value]
        hab_values = inst_info.detector_names[DetectorType.HAB.value]
        self.assertEqual(mocked_ipf_data[ipf_names["lab_short"]], lab_values.detector_name_short)
        self.assertEqual(mocked_ipf_data[ipf_names["lab_full"]], lab_values.detector_name)
        self.assertEqual(mocked_ipf_data[ipf_names["hab_short"]], hab_values.detector_name_short)
        self.assertEqual(mocked_ipf_data[ipf_names["hab_full"]], hab_values.detector_name)

    @mock.patch("state_instrument_info_test.state_instrument_info.get_named_elements_from_ipf_file")
    def test_setting_det_names_no_hab(self, mocked_ipf_loader):
        mocked_ipf_data = self._prep_mock_det_ipf()
        mocked_ipf_loader.return_value = mocked_ipf_data
        mocked_data_info = mock.NonCallableMock()

        inst_info = StateInstrumentInfo()
        state_instrument_info._set_detector_names(inst_info, mocked_data_info, invalid_detector_types=[DetectorType.HAB])

        ipf_names = self._get_ipf_names()
        lab_values = inst_info.detector_names[DetectorType.LAB.value]
        self.assertEqual(mocked_ipf_data[ipf_names["lab_short"]], lab_values.detector_name_short)
        self.assertEqual(mocked_ipf_data[ipf_names["lab_full"]], lab_values.detector_name)

        hab_values = inst_info.detector_names[DetectorType.HAB.value]
        self.assertIsNone(hab_values.detector_name_short)
        self.assertIsNone(hab_values.detector_name)

    @mock.patch("state_instrument_info_test.state_instrument_info._get_invalid_monitor_types")
    @mock.patch("state_instrument_info_test.state_instrument_info._set_detector_names")
    def test_init_detector_names_forwards_invalid(self, set_det_names, get_invalid_monitors):
        expected = mock.NonCallableMock()
        get_invalid_monitors.return_value = expected

        state_instrument_info._init_detector_names(data_info=mock.NonCallableMock(), inst_info=mock.NonCallableMock())

        set_det_names.assert_called_once_with(mock.ANY, mock.ANY, invalid_detector_types=expected)

    @mock.patch("state_instrument_info_test.state_instrument_info._get_invalid_monitor_names")
    @mock.patch("state_instrument_info_test.state_instrument_info._set_monitor_names")
    def test_init_monitor_names_forwards_invalid(self, set_monitor_names, get_invalid_names):
        expected = mock.NonCallableMock()
        get_invalid_names.return_value = expected

        state_instrument_info._init_monitor_names(data_info=mock.NonCallableMock(), inst_info=mock.NonCallableMock())

        set_monitor_names.assert_called_once_with(mock.ANY, mock.ANY, invalid_monitor_names=expected)

    @mock.patch("state_instrument_info_test.state_instrument_info.get_monitor_names_from_idf_file")
    def test_set_monitor_names(self, mocked_idf_names):
        expected = mock.NonCallableMock()
        mocked_idf_names.return_value = expected

        inst_info = StateInstrumentInfo()
        state_instrument_info._set_monitor_names(inst_info_state=inst_info, idf_path=None, invalid_monitor_names=None)

        self.assertEqual(expected, inst_info.monitor_names)

    @mock.patch("state_instrument_info_test.state_instrument_info._init_detector_names")
    @mock.patch("state_instrument_info_test.state_instrument_info._init_monitor_names")
    def test_builder(self, init_monitor_names, init_detector_names):
        data_info = mock.NonCallableMock()
        data_info.idf_file_path = "idf_path"
        returned = StateInstrumentInfo.build_from_data_info(data_info=data_info)

        init_detector_names.assert_called_once()
        init_monitor_names.assert_called_once()
        self.assertIsInstance(returned, StateInstrumentInfo)
        self.assertEqual("idf_path", returned.idf_path)


if __name__ == "__main__":
    unittest.main()
