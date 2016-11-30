from __future__ import (absolute_import, division, print_function)

import unittest
import mantid
from mantid.simpleapi import AngularAutoCorrelationsTwoAxes

class AngularAutoCorrelationsTwoAxesTest(unittest.TestCase):

    def test_simple(self):
        output_ws, output_ws_ft = AngularAutoCorrelationsTwoAxes(InputFile = 'trajectory_methyliodide.nc',
                                 Timestep = '10.0',
                                 SpeciesOne = 'C',
                                 SpeciesTwo = 'I',
                                 SpeciesThree = 'H',
                                 OutputWorkspace = 'output_ws',
                                 OutputWorkspaceFT = 'output_ws_ft')

        data_y = output_ws.readY(0)
        self.assertAlmostEqual(data_y[1], 0.998192889883855)
        data_y = output_ws.readY(1)
        self.assertAlmostEqual(data_y[1],  0.993160180815704)
        self.assertAlmostEqual(output_ws.blocksize(), 501)

if __name__ == "__main__":
	unittest.main()