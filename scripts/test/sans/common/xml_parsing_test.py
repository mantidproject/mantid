# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.kernel import DateAndTime
from sans.common.file_information import SANSFileInformationFactory, get_instrument_paths_for_sans_file
from sans.common.xml_parsing import (
    get_named_elements_from_ipf_file,
    get_monitor_names_from_idf_file,
    get_valid_to_time_from_idf_string,
)


class XMLParsingTest(unittest.TestCase):
    def test_that_named_entries_in_instrument_parameter_file_can_be_retrieved(self):
        # Arrange
        test_file = "LARMOR00003368"
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information(test_file)
        full_file_path = file_information.get_file_name()

        _, ipf = get_instrument_paths_for_sans_file(full_file_path)
        to_search = ["low-angle-detector-name", "high-angle-detector-short-name"]

        # Act
        results = get_named_elements_from_ipf_file(ipf, to_search, str)

        # Assert
        self.assertEqual(len(results), 2)

        self.assertEqual(results["low-angle-detector-name"], "DetectorBench")
        self.assertEqual(results["high-angle-detector-short-name"], "front")

    def test_that_monitors_can_be_found(self):
        # Arrange
        test_file = "LARMOR00003368"
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information(test_file)
        full_file_path = file_information.get_file_name()

        idf, _ = get_instrument_paths_for_sans_file(full_file_path)

        # Act
        results = get_monitor_names_from_idf_file(idf)

        # Assert
        self.assertEqual(len(results), 10)
        for key, value in list(results.items()):
            self.assertEqual(value, ("monitor" + str(key)))

    def test_that_monitors_can_be_found_v2(self):
        # Arrange
        test_file = "LOQ74044"
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information(test_file)
        full_file_path = file_information.get_file_name()

        idf, _ = get_instrument_paths_for_sans_file(full_file_path)

        # Act
        results = get_monitor_names_from_idf_file(idf)

        # Assert
        self.assertEqual(len(results), 2)
        for key, value in list(results.items()):
            self.assertEqual(value, ("monitor" + str(key)))

    def test_that_get_valid_to_date_from_idf_string(self):
        # Arrange
        idf_string = (
            '<?xml version="1.0" encoding="UTF-8" ?>'
            "<!-- For help on the notation used to specify an Instrument Definition File "
            "see http://www.mantidproject.org/IDF -->"
            '<instrument xmlns="http://www.mantidproject.org/IDF/1.0" '
            '            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" '
            '            xsi:schemaLocation="http://www.mantidproject.org/IDF/1.0 '
            'http://schema.mantidproject.org/IDF/1.0/IDFSchema.xsd" '
            '            name="PEARL" valid-from   ="1900-01-31 23:59:59" '
            '            valid-to     ="2011-05-01 23:59:50" '
            '            last-modified="2008-09-17 05:00:00">'
            "</instrument>"
        )

        # Act
        extracted_time = get_valid_to_time_from_idf_string(idf_string)
        # Assert
        self.assertEqual(extracted_time, DateAndTime("2011-05-01 23:59:50"))


if __name__ == "__main__":
    unittest.main()
