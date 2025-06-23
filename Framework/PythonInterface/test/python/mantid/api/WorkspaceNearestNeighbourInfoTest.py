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
        self.assertRaises(RuntimeError, ni.getNeighboursExact, 0)
        self.assertRaises(RuntimeError, ni.getNeighboursExact, 100)
        self.assertRaises(RuntimeError, ni.getNeighbours, 100, 1)

        self.assertRaises(ValueError, ni.getNeighbours, 1, 20)
        self.assertRaises(ValueError, ni.getNeighbours, 1, -5)

    def test_compare_neighbours_from_detector_exact_spectrum(self):
        ws = CreateSampleWorkspace(NumMonitors=0, NumBanks=10, BankPixelWidth=1, XMin=0, XMax=1, BinWidth=1)
        ni = WorkspaceNearestNeighbourInfo(ws, True, 6)

        for ind in range(ws.getNumberHistograms()):
            spec = ws.getSpectrum(ind)
            d = ws.getDetector(ind)
            neighbours_by_detector = ni.getNeighbours(d)
            neighbours_by_exact = ni.getNeighboursExact(spec.getSpectrumNo())
            self.assertEqual(neighbours_by_exact, neighbours_by_detector)
            self.assertEqual(len(neighbours_by_exact), 8)

            neighbours_by_spectrum = ni.getNeighbours(spec.getSpectrumNo(), 0)
            self.assertEqual(neighbours_by_detector, neighbours_by_spectrum)
            self.assertEqual(len(neighbours_by_spectrum), 8)

    def test_get_neighbours_with_radius(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4)
        ni = WorkspaceNearestNeighbourInfo(ws, True, 4)

        neigh_closer = ni.getNeighbours(1, 0.020)
        self.assertEqual(len(neigh_closer), 3)
        neigh_distance = ni.getNeighbours(1, 0.025)
        self.assertEqual(len(neigh_distance), 6)

        self.assertTrue(set(neigh_closer.values()).issubset(set(neigh_distance.values())))

        d = ws.getDetector(0)
        neighbours_by_det_radius_closer = ni.getNeighbours(d, 0.01)
        neighbours_by_det_radius_distant = ni.getNeighbours(d, 0.02)
        self.assertTrue(set(neighbours_by_det_radius_closer.values()).issubset(set(neighbours_by_det_radius_distant.values())))


if __name__ == "__main__":
    unittest.main()
