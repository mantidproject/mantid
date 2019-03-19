# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.simpleapi import AnalysisDataService as ads, ConjoinSpectra
from testhelpers import run_algorithm


single_spec_ws = "single_spectra_ws"
multi_spec_ws = "conjoined"


class ConjoinSpectraTest(unittest.TestCase):
    def setUp(self):
        data_x = range(0, 6)
        data_y = range(0, 5)
        data_e = [1] * 5

        create_ws_alg = run_algorithm("CreateWorkspace", DataX=data_x, DataY=data_y, DataE=data_e, NSpec=1,
                                      UnitX="Wavelength", OutputWorkspace=single_spec_ws)
        self._aWS = create_ws_alg.getPropertyValue("OutputWorkspace")

    def test_basic_run(self):
        ConjoinSpectra(InputWorkspaces=single_spec_ws + "," + single_spec_ws, OutputWorkspace=multi_spec_ws)
        conjoined_ws = ads.retrieve(multi_spec_ws)

        ws_index = 0
        in_data_y = ads.retrieve(single_spec_ws).readY(ws_index)
        in_data_e = ads.retrieve(single_spec_ws).readE(ws_index)
        out_data_y1 = conjoined_ws.readY(0)
        out_data_y2 = conjoined_ws.readY(1)
        out_data_e1 = conjoined_ws.readE(0)
        out_data_e2 = conjoined_ws.readE(1)

        # Check output shape
        self.assertEqual(len(in_data_y), len(out_data_y1))
        self.assertEqual(len(in_data_y), len(out_data_y2))
        self.assertEqual(len(in_data_e), len(out_data_e1))
        self.assertEqual(len(in_data_e), len(out_data_e2))
        self.assertEqual(2, conjoined_ws.getNumberHistograms())  # Should always have 2 histograms

        self.assertEquals(set(in_data_y), set(out_data_y1))
        self.assertEquals(set(in_data_y), set(out_data_y2))
        self.assertEquals(set(in_data_e), set(out_data_e1))
        self.assertEquals(set(in_data_e), set(out_data_e2))


if __name__ == '__main__':
    unittest.main()
