from __future__ import (absolute_import, division, print_function)

import unittest
import mantid
from mantid.simpleapi import VelocityCrossCorrelations

class VelocityCrossCorrelationsTest(unittest.TestCase):

    def test_simple(self):
        output_ws = VelocityCrossCorrelations(InputFile = 'trajectories.nc',
                                              Timestep = '2.0')

        self.assertEqual(output_ws.getNumberHistograms(), 3)
        data_y = output_ws.readY(0)
        self.assertAlmostEqual(data_y[0], -4.08068817863366e-05)
        self.assertAlmostEqual(output_ws.blocksize(), 1053)
        data_y = output_ws.readY(1)
        self.assertAlmostEqual(data_y[0], -8.60158623672771e-05)
        data_y = output_ws.readY(2)
        self.assertAlmostEqual(data_y[0], -8.76322385197998e-05)

if __name__ == "__main__":
	unittest.main()