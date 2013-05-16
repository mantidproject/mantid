import unittest
from mantid.kernel import DateAndTime

class DateAndTimeTest(unittest.TestCase):

    iso_str = "2008-12-18T17:58:38"

    def test_construction_with_ISO_string_produces_expected_object(self):
        dt = DateAndTime(self.iso_str)
        self.assertEquals(self.iso_str, str(dt))
        self.assertEquals(dt.totalNanoseconds(), 598471118000000000)
        
    def test_construction_with_total_nano_seconds(self):
        dt = DateAndTime(598471118000000000)
        self.assertEquals(self.iso_str, str(dt))
        
if __name__ == "__main__":
    unittest.main()