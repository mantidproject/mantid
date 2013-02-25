import unittest
from mantid.kernel import MemoryStats

class MemoryStatsTest(unittest.TestCase):

    def test_values_are_all_greater_than_zero(self):
        # Best we can do is test that something is returned
        mem = MemoryStats()

        self.assertTrue(hasattr(mem, "update"))
        self.assertGreater(mem.availMem, 0.0, "Value should be larger than 0.0")
        self.assertGreater(mem.totalMem, 0.0, "Value should be larger than 0.0")
        self.assertGreater(mem.residentMem, 0.0, "Value should be larger than 0.0")
        self.assertGreater(mem.virtualMem, 0.0, "Value should be larger than 0.0")
        self.assertGreater(mem.reservedMem, 0.0, "Value should be larger than 0.0")
        self.assertGreater(mem.getFreeRatio, 0.0, "Value should be larger than 0.0")

if __name__ == '__main__':
    unittest.main()
