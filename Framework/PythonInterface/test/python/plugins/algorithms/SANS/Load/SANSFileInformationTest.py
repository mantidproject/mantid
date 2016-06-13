import unittest
import mantid

from Load.SANSFileInformation import (SANSFileInformationFactory, SANSFileInformation, SANSFileType, SANSInstrument)
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
        self.assertTrue(file_information.get_number_of_periods() == 1)
        self.assertTrue(file_information.get_date() == DateAndTime("2013-10-25T14:21:19"))
        self.assertTrue(file_information.get_instrument() == SANSInstrument.SANS2D)
        self.assertTrue(file_information.get_type() == SANSFileType.ISISNexus)
        self.assertTrue(file_information.get_run_number() == 22024)
        self.assertTrue(file_information.is_event_mode() == False)

    def test_that_can_extract_information_from_file_for_LOQ_single_period_and_raw_format(self):
        # Arrange
        # The file is a single period
        file_name = "LOQ48094"
        factory = SANSFileInformationFactory()

        # Act
        file_information = factory.create_sans_file_information(file_name)

        # Assert
        self.assertTrue(file_information.get_number_of_periods() == 1)
        self.assertTrue(file_information.get_date() == DateAndTime("2008-12-18T11:20:58"))
        self.assertTrue(file_information.get_instrument() == SANSInstrument.LOQ)
        self.assertTrue(file_information.get_type() == SANSFileType.ISISRaw)
        self.assertTrue(file_information.get_run_number() == 48094)


if __name__ == '__main__':
    unittest.main()
