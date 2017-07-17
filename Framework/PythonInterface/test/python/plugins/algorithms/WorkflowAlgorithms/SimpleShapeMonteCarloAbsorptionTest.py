from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import SimpleShapeMonteCarloAbsorption

from mantid.kernel import *
from mantid.api import *

import unittest

class SimpleShapeMonteCarloAbsorptionTest(unittest.TestCase):

    def setUp(self):
        """

        """
        red_ws = Load('irs26176_graphite002_red.nxs')
        red_ws = ConvertUnits(InputWorkspace=red_ws, Target="Wavelength", EMode="Indirect", EFixed=1.845)

        self._red_ws = red_ws

    def _test_corrections_workspace(self, corr_ws):

        x_unit = corr_ws.getAxis(0).getUnit().unitID()
        self.assertEquals(x_unit,'Wavelength')

        y_unit = corr_ws.YUnitLabel()
        self.assertEquals(y_unit, 'Attenuation Factor')

        num_hists = corr_ws.getNumberHistograms()


if __name__=="__main__":
    unittest.main()
