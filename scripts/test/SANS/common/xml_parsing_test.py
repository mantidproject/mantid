import unittest
import mantid

from sans.common.file_information import (SANSFileInformationFactory, get_instrument_paths_for_sans_file)
from sans.common.xml_parsing import (get_named_elements_from_ipf_file, get_monitor_names_from_idf_file)


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
        self.assertTrue(len(results) == 2)

        self.assertTrue(results["low-angle-detector-name"] == "DetectorBench")
        self.assertTrue(results["high-angle-detector-short-name"] == "front")

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
        self.assertTrue(len(results) == 10)
        for key, value in results.items():
            self.assertTrue(value == ("monitor"+str(key)))

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
        self.assertTrue(len(results) == 2)
        for key, value in results.items():
            self.assertTrue(value == ("monitor"+str(key)))


if __name__ == '__main__':
    unittest.main()
