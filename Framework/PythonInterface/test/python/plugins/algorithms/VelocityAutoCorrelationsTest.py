from __future__ import (absolute_import, division, print_function)

import unittest
import mantid
from mantid.simpleapi import VelocityAutoCorrelations

class VelocityAutoCorrelationsTest(unittest.TestCase):

    def test_simple(self):
        output_ws = VelocityAutoCorrelations(InputFile = 'trajectories.nc',
                                              Timestep = '2.0')

        self.assertEqual(output_ws.getNumberHistograms(), 2)
        data_y = output_ws.readY(0)
        self.assertAlmostEqual(data_y[0], 0.000206830078819335)
        self.assertAlmostEqual(output_ws.blocksize(), 1053)
        data_y = output_ws.readY(1)
        self.assertAlmostEqual(data_y[0], 0.000247266521347895)

if __name__ == "__main__":
	unittest.main()