# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.kernel import DateAndTime
from unittest import mock
from SANS.sans.common.enums import SampleShape
from SANS.sans.common.file_information import (
    SANSFileInformationFactory,
    FileType,
    SANSInstrument,
    get_instrument_paths_for_sans_file,
    SANSFileInformationISISAdded,
    SANSFileInformationISISNexus,
)
from sans.test_helper.file_information_mock import SANSFileInformationMock


class SANSFileInformationTest(unittest.TestCase):
    def test_add_file_without_add_extension_is_found(self):
        file_name = "ZOOM9947-a"
        factory = SANSFileInformationFactory()
        file_information = factory.create_sans_file_information(file_name)

        self.assertIsInstance(file_information, SANSFileInformationISISAdded)
        self.assertTrue(file_information.is_event_mode())

    def test_event_data_not_added_is_detected(self):
        file_name = "ZOOM9898"
        factory = SANSFileInformationFactory()
        file_information = factory.create_sans_file_information(file_name)

        self.assertIsInstance(file_information, SANSFileInformationISISNexus)
        self.assertTrue(file_information.is_event_mode())

    def test_that_can_extract_information_from_file_for_SANS2D_single_period_and_ISISNexus(self):
        # Arrange
        # The file is a single period
        file_name = "SANS2D00022024"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)

        # Assert
        self.assertEqual(file_information.get_number_of_periods(), 1)
        self.assertEqual(file_information.get_date(), DateAndTime("2013-10-25T14:21:19"))
        self.assertEqual(file_information.get_instrument(), SANSInstrument.SANS2D)
        self.assertEqual(file_information.get_type(), FileType.ISIS_NEXUS)
        self.assertEqual(file_information.get_run_number(), 22024)
        self.assertFalse(file_information.is_event_mode())
        self.assertFalse(file_information.is_added_data())
        self.assertEqual(file_information.get_width(), 8.0)
        self.assertEqual(file_information.get_height(), 8.0)
        self.assertEqual(file_information.get_thickness(), 1.0)
        self.assertEqual(file_information.get_shape(), SampleShape.DISC)

    def test_that_can_extract_information_from_file_for_LOQ_single_period_and_raw_format(self):
        # Arrange
        # The file is a single period
        file_name = "LOQ48094"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)

        # Assert
        self.assertEqual(file_information.get_number_of_periods(), 1)
        self.assertEqual(file_information.get_date(), DateAndTime("2008-12-18T11:20:58"))
        self.assertEqual(file_information.get_instrument(), SANSInstrument.LOQ)
        self.assertEqual(file_information.get_type(), FileType.ISIS_RAW)
        self.assertEqual(file_information.get_run_number(), 48094)
        self.assertFalse(file_information.is_added_data())
        self.assertEqual(file_information.get_width(), 8.0)
        self.assertEqual(file_information.get_height(), 8.0)
        self.assertEqual(file_information.get_thickness(), 1.0)
        self.assertEqual(file_information.get_shape(), SampleShape.DISC)

    def test_that_can_extract_information_from_file_for_SANS2D_multi_period_event_and_nexus_format(self):
        # Arrange
        # The file is a multi period and event-based
        file_name = "LARMOR00003368"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)

        # Assert
        self.assertEqual(file_information.get_number_of_periods(), 4)
        self.assertEqual(file_information.get_date(), DateAndTime("2015-06-05T14:43:49"))
        self.assertEqual(file_information.get_instrument(), SANSInstrument.LARMOR)
        self.assertEqual(file_information.get_type(), FileType.ISIS_NEXUS)
        self.assertEqual(file_information.get_run_number(), 3368)
        self.assertTrue(file_information.is_event_mode())
        self.assertFalse(file_information.is_added_data())
        self.assertEqual(file_information.get_width(), 8.0)
        self.assertEqual(file_information.get_height(), 8.0)
        self.assertEqual(file_information.get_thickness(), 2.0)
        self.assertEqual(file_information.get_shape(), SampleShape.FLAT_PLATE)

    def test_two_file_informations_are_eq(self):
        # Arrange
        # The file is a multi period and event-based
        file_name = "LARMOR00003368"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)
        file_information_2 = factory.create_sans_file_information(file_name)

        # Two identical file informations should be equal even if they are different objects
        self.assertEqual(file_information, file_information_2)

    def test_that_can_extract_information_for_added_histogram_data_and_nexus_format(self):
        # Arrange
        # The file is a single period, histogram-based and added
        file_name = "AddedHistogram-add"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)

        # Assert
        self.assertEqual(file_information.get_number_of_periods(), 1)
        self.assertEqual(file_information.get_date(), DateAndTime("2013-10-25T14:21:19"))
        self.assertEqual(file_information.get_instrument(), SANSInstrument.SANS2D)
        self.assertEqual(file_information.get_type(), FileType.ISIS_NEXUS_ADDED)
        self.assertEqual(file_information.get_run_number(), 22024)
        self.assertFalse(file_information.is_event_mode())
        self.assertTrue(file_information.is_added_data())
        self.assertEqual(file_information.get_width(), 8.0)
        self.assertEqual(file_information.get_height(), 8.0)
        self.assertEqual(file_information.get_thickness(), 1.0)
        self.assertEqual(file_information.get_shape(), SampleShape.DISC)

    def test_that_can_extract_information_for_LARMOR_added_event_data_and_multi_period_and_nexus_format(self):
        # Arrange
        # The file is a single period, histogram-based and added
        file_name = "AddedEvent-add"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)

        # Assert
        self.assertEqual(file_information.get_number_of_periods(), 4)
        self.assertEqual(file_information.get_date(), DateAndTime("2016-10-12T04:33:47"))
        self.assertEqual(file_information.get_instrument(), SANSInstrument.LARMOR)
        self.assertEqual(file_information.get_type(), FileType.ISIS_NEXUS_ADDED)
        self.assertEqual(file_information.get_run_number(), 13065)
        self.assertTrue(file_information.is_event_mode())
        self.assertTrue(file_information.is_added_data())
        self.assertEqual(file_information.get_width(), 6.0)
        self.assertEqual(file_information.get_height(), 8.0)
        self.assertEqual(file_information.get_thickness(), 1.0)
        self.assertEqual(file_information.get_shape(), SampleShape.FLAT_PLATE)

    def test_that_can_find_data_with_numbers_but_no_instrument(self):
        # Arrange
        # The file is a single period, histogram-based and added

        file_name = "74044-add"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)

        # Assert
        self.assertTrue(file_information)

    def test_that_run_number_is_from_file_with_ext(self):
        filename = "INST12345.nxs"
        expected = 12345

        file_info = SANSFileInformationMock(file_name=filename)
        self.assertEqual(expected, file_info.get_run_number())

    def test_run_number_without_ext(self):
        filename = "INST2D12345"
        expected = 12345

        file_info = SANSFileInformationMock(file_name=filename)
        self.assertEqual(expected, file_info.get_run_number())

    def test_run_number_with_n001_ext(self):
        filename = "INST2345.n001"
        expected = 2345

        file_info = SANSFileInformationMock(file_name=filename)
        self.assertEqual(expected, file_info.get_run_number())

    def test_run_number_with_appendix(self):
        filename = "INST2D101-001"
        expected = 101

        file_info = SANSFileInformationMock(file_name=filename)
        self.assertEqual(expected, file_info.get_run_number())

    def test_bad_run_name_gets_file_no_from_file(self):
        # Make sure we don't log before the mock gets injected as it will warn during init
        file_info = SANSFileInformationMock(file_name="0")
        logger_mock = mock.Mock()
        file_info.logger = logger_mock

        filename = "NoDigits"
        # Should get the mock's returned name - overriding classes should call hdf5
        expected = int(file_info._get_run_number_from_file(""))
        # Call init again now we have injected our mock
        file_info.__init__(file_name=filename)

        self.assertEqual(expected, file_info.get_run_number())
        logger_mock.warning.assert_called_once_with(mock.ANY)


class SANSFileInformationGeneralFunctionsTest(unittest.TestCase):
    def test_that_finds_idf_and_ipf_paths(self):
        # Arrange
        file_name = "SANS2D00022024"
        # Act
        idf_path, ipf_path = get_instrument_paths_for_sans_file(file_name)
        # Assert
        self.assertNotEqual(idf_path, None)
        self.assertNotEqual(ipf_path, None)
        self.assertTrue("Definition" in idf_path)
        self.assertTrue("Parameters" in ipf_path)


if __name__ == "__main__":
    unittest.main()
