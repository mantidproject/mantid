# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import WorkspaceNearestNeighbourInfo
from mantid.simpleapi import CreateSampleWorkspace


class WorkspaceNearestNeighbourInfoTest(unittest.TestCase):
    def test_out_of_range_numbers(self):
        ws = CreateSampleWorkspace(NumMonitors=0, NumBanks=10, BankPixelWidth=1, XMin=0, XMax=1, BinWidth=1)

        with self.assertRaises(ValueError):
            WorkspaceNearestNeighbourInfo(ws, True, 100)

        ni = WorkspaceNearestNeighbourInfo(ws, True, 6)
        with self.assertRaises(RuntimeError):
            ni.getNeighboursExact(0)
            ni.getNeighboursExact(100)
            ni.getNeighbours(100)

    def test_compare_neighbours_from_detector_exact_spectrum(self):
        ws = CreateSampleWorkspace(NumMonitors=0, NumBanks=10, BankPixelWidth=1, XMin=0, XMax=1, BinWidth=1)
        ni = WorkspaceNearestNeighbourInfo(ws, True, 6)

        for ind in range(ws.getNumberHistograms()):
            spec = ws.getSpectrum(ind)
            d = ws.getDetector(ind)
            neighbours_by_detector = ni.getNeighbours(d)
            neighbours_by_exact = ni.getNeighboursExact(spec.getSpectrumNo())
            self.assertEqual(neighbours_by_exact, neighbours_by_detector)

            neighbours_by_spectrum = ni.getNeighbours(spec.getSpectrumNo())
            self.assertEqual(neighbours_by_detector, neighbours_by_spectrum)


if __name__ == "__main__":
    unittest.main()
