# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import LoadEmptyVesuvio


class LoadEmptyVesuvioTest(unittest.TestCase):
    def test_load_empty_no_par_file(self):
        """
        Tests loading the instrument with no PAR file.
        """
        evs_ws = LoadEmptyVesuvio()

        self.assertEqual(evs_ws.getNumberHistograms(), 198)
        self.assertEqual(evs_ws.blocksize(), 1)

        evs = evs_ws.getInstrument()
        sample_pos = evs.getSample().getPos()

        bs_det_1 = evs_ws.getDetector(2)
        bs_det_1_l1 = sample_pos.distance(bs_det_1.getPos())

        self.assertAlmostEqual(bs_det_1_l1, 0.6747705, places=7)

    def test_load_empty_with_par_file(self):
        """
        Tests loading the instrument with no PAR file.
        """
        evs_ws = LoadEmptyVesuvio(InstrumentParFile="IP0005.dat")

        self.assertEqual(evs_ws.getNumberHistograms(), 198)
        self.assertEqual(evs_ws.blocksize(), 1)

        evs = evs_ws.getInstrument()
        sample_pos = evs.getSample().getPos()

        bs_det_1 = evs_ws.getDetector(2)
        bs_det_1_l1 = sample_pos.distance(bs_det_1.getPos())

        self.assertAlmostEqual(bs_det_1_l1, 0.6707999706268, places=7)


if __name__ == "__main__":
    unittest.main()
