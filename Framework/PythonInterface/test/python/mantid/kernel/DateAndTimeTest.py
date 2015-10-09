import unittest
from mantid.kernel import DateAndTime

class DateAndTimeTest(unittest.TestCase):

    iso_str = "2008-12-18T17:58:38"
    # We had to add a space to the end of the string representation to get around an IPython bug (#8351)
    iso_str_plus_space = iso_str + " "

    def test_construction_with_ISO_string_produces_expected_object(self):
        dt = DateAndTime(self.iso_str)
        self.assertEquals(self.iso_str_plus_space, str(dt))
        self.assertEquals(dt.totalNanoseconds(), 598471118000000000)

    def test_construction_with_total_nano_seconds(self):
        dt = DateAndTime(598471118000000000)
        self.assertEquals(self.iso_str_plus_space, str(dt))

if __name__ == "__main__":
    unittest.main()
