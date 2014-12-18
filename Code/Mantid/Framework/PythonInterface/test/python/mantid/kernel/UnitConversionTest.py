import unittest
from mantid.kernel import UnitConversion, DeltaEModeType
import math

class UnitConversionTest(unittest.TestCase):

    def test_run_accepts_string_units(self):
        src_unit = "Wavelength"
        src_value = 1.5
        dest_unit = "Momentum"

        l1 = l2 = twoTheta = efixed = 0.0
        emode = DeltaEModeType.Indirect;
        expected = 2.0*math.pi/src_value

        result = UnitConversion.run(src_unit, dest_unit, src_value, l1, l2, twoTheta, emode, efixed)
        self.assertAlmostEqual(result, expected, 12)

if __name__ == '__main__':
    unittest.main()
