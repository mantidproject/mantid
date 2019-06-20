# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from sans.common.file_information import (SANSFileInformationFactory, SANSFileInformation, FileType,
                                          SANSInstrument, get_instrument_paths_for_sans_file)
from sans.common.enums import SampleShape
from mantid.kernel import DateAndTime


class SANSFileInformationTest(unittest.TestCase):
    def test_that_can_extract_information_from_file_for_SANS2D_single_period_and_ISISNexus(self):
        # Arrange
        # The file is a single period
        file_name = "SANS2D00022024"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)

        # Assert
        self.assertEqual(file_information.get_number_of_periods(),  1)
        self.assertEqual(file_information.get_date(),  DateAndTime("2013-10-25T14:21:19"))
        self.assertEqual(file_information.get_instrument(),  SANSInstrument.SANS2D)
        self.assertEqual(file_information.get_type(),  FileType.ISISNexus)
        self.assertEqual(file_information.get_run_number(),  22024)
        self.assertFalse(file_information.is_event_mode())
        self.assertFalse(file_information.is_added_data())
        self.assertEqual(file_information.get_width(),  8.0)
        self.assertEqual(file_information.get_height(),  8.0)
        self.assertEqual(file_information.get_thickness(),  1.0)
        self.assertEqual(file_information.get_shape(), SampleShape.Disc)

    def test_that_can_extract_information_from_file_for_LOQ_single_period_and_raw_format(self):
        # Arrange
        # The file is a single period
        file_name = "LOQ48094"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)

        # Assert
        self.assertEqual(file_information.get_number_of_periods(),  1)
        self.assertEqual(file_information.get_date(),  DateAndTime("2008-12-18T11:20:58"))
        self.assertEqual(file_information.get_instrument(),  SANSInstrument.LOQ)
        self.assertEqual(file_information.get_type(),  FileType.ISISRaw)
        self.assertEqual(file_information.get_run_number(),  48094)
        self.assertFalse(file_information.is_added_data())
        self.assertEqual(file_information.get_width(),  8.0)
        self.assertEqual(file_information.get_height(),  8.0)
        self.assertEqual(file_information.get_thickness(),  1.0)
        self.assertEqual(file_information.get_shape(), SampleShape.Disc)

    def test_that_can_extract_information_from_file_for_SANS2D_multi_period_event_and_nexus_format(self):
        # Arrange
        # The file is a multi period and event-based
        file_name = "LARMOR00003368"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)

        # Assert
        self.assertEqual(file_information.get_number_of_periods(),  4)
        self.assertEqual(file_information.get_date(),  DateAndTime("2015-06-05T14:43:49"))
        self.assertEqual(file_information.get_instrument(),  SANSInstrument.LARMOR)
        self.assertEqual(file_information.get_type(),  FileType.ISISNexus)
        self.assertEqual(file_information.get_run_number(),  3368)
        self.assertTrue(file_information.is_event_mode())
        self.assertFalse(file_information.is_added_data())
        self.assertEqual(file_information.get_width(),  8.0)
        self.assertEqual(file_information.get_height(),  8.0)
        self.assertEqual(file_information.get_thickness(),  2.0)
        self.assertEqual(file_information.get_shape(), SampleShape.FlatPlate)

    def test_that_can_extract_information_for_added_histogram_data_and_nexus_format(self):
        # Arrange
        # The file is a single period, histogram-based and added
        file_name = "AddedHistogram-add"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)

        # Assert
        self.assertEqual(file_information.get_number_of_periods(),  1)
        self.assertEqual(file_information.get_date(),  DateAndTime("2013-10-25T14:21:19"))
        self.assertEqual(file_information.get_instrument(),  SANSInstrument.SANS2D)
        self.assertEqual(file_information.get_type(),  FileType.ISISNexusAdded)
        self.assertEqual(file_information.get_run_number(),  22024)
        self.assertFalse(file_information.is_event_mode())
        self.assertTrue(file_information.is_added_data())
        self.assertEqual(file_information.get_width(),  8.0)
        self.assertEqual(file_information.get_height(),  8.0)
        self.assertEqual(file_information.get_thickness(),  1.0)
        self.assertEqual(file_information.get_shape(), SampleShape.Disc)

    def test_that_can_extract_information_for_LARMOR_added_event_data_and_multi_period_and_nexus_format(self):
        # Arrange
        # The file is a single period, histogram-based and added
        file_name = "AddedEvent-add"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)

        # Assert
        self.assertEqual(file_information.get_number_of_periods(),  4)
        self.assertEqual(file_information.get_date(),  DateAndTime("2016-10-12T04:33:47"))
        self.assertEqual(file_information.get_instrument(),  SANSInstrument.LARMOR)
        self.assertEqual(file_information.get_type(),  FileType.ISISNexusAdded)
        self.assertEqual(file_information.get_run_number(),  13065)
        self.assertTrue(file_information.is_event_mode())
        self.assertTrue(file_information.is_added_data())
        self.assertEqual(file_information.get_width(),  6.0)
        self.assertEqual(file_information.get_height(),  8.0)
        self.assertEqual(file_information.get_thickness(),  1.0)
        self.assertEqual(file_information.get_shape(), SampleShape.FlatPlate)

    def test_that_can_find_data_with_numbers_but_no_instrument(self):
        # Arrange
        # The file is a single period, histogram-based and added

        file_name = "74044-add"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)

        # Assert
        self.assertTrue(file_information)


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


if __name__ == '__main__':
    unittest.main()
