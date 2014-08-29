import unittest
from mantid.kernel import InstrumentInfo, ConfigService

class InstrumentInfoTest(object):

    def test_construction_raies_an_error(self):
        self.assertRaises(RuntimeError, InstrumentInfo)

    def test_instrument_name(self):
        pass

    def test_instrument_shortName(self):
        pass

if __name__ == '__main__':
    unittest.main()
