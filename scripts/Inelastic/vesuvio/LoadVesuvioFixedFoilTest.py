# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright (c) 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX-License-Identifier: GPL-3.0+
"""Run with Mantid's Python, keeping LoadVesuvioFixedFoil.py beside this file."""

import unittest
from unittest.mock import patch

import numpy as np
from mantid.api import AlgorithmFactory, AlgorithmManager, mtd
import mantid.simpleapi as ms

import LoadVesuvioFixedFoil


class LoadVesuvioFixedFoilTest(unittest.TestCase):
    spectra = [135, 136, 143]
    x = np.array([0.0, 100, 200, 300, 400, 450, 500, 600, 700, 800])
    shapes = np.array(
        [
            [4, 5, 6, 8, 10, 12, 14, 16, 18],
            [7, 8, 9, 10, 12, 14, 16, 18, 20],
            [10, 12, 14, 16, 18, 20, 23, 24, 26],
        ],
        dtype=float,
    )

    def tearDown(self):
        for name in ("fixed_test", "fixed_test_monitors"):
            if name in mtd:
                ms.DeleteWorkspace(name, EnableLogging=False)

    def load_synthetic(self, order, *, OutputWorkspace, **kwargs):
        data_names, monitor_names = [], []
        for index, period in enumerate(order):
            data_name = f"{OutputWorkspace}_data_{index}"
            mon_name = f"{OutputWorkspace}_mon_{index}"
            weights = np.array([period + 1, 2 * period + 1, 3 * period + 1])[:, None]
            data = ms.CreateWorkspace(
                DataX=np.tile(self.x, 3),
                DataY=(self.shapes * weights).ravel(),
                NSpec=3,
                UnitX="TOF",
                OutputWorkspace=data_name,
                EnableLogging=False,
            )
            ms.LoadInstrument(Workspace=data, InstrumentName="VESUVIO", RewriteSpectraMap=False, EnableLogging=False)
            for i, (spectrum, detector) in enumerate(zip(self.spectra, (3101, 3102, 3109))):
                data.getSpectrum(i).setSpectrumNo(spectrum)
                data.getSpectrum(i).clearDetectorIDs()
                data.getSpectrum(i).addDetectorID(detector)
            ms.CreateWorkspace(
                DataX=np.tile(self.x, 2),
                DataY=np.full(18, (period + 1) * 100.0),
                NSpec=2,
                UnitX="TOF",
                OutputWorkspace=mon_name,
                EnableLogging=False,
            )
            data_names.append(data_name)
            monitor_names.append(mon_name)
        ms.GroupWorkspaces(InputWorkspaces=data_names, OutputWorkspace=OutputWorkspace, EnableLogging=False)
        ms.GroupWorkspaces(InputWorkspaces=monitor_names, OutputWorkspace=OutputWorkspace + "_monitors", EnableLogging=False)

    def run_algorithm(self, order, **properties):
        alg = AlgorithmManager.create("LoadVesuvioFixedFoil")
        alg.initialize()
        alg.setRethrows(True)
        for name, value in {
            "Filename": "14188",
            "FoilInSpectra": "135-136",
            "FoilOutSpectra": "143",
            "OutputWorkspace": "fixed_test",
            **properties,
        }.items():
            alg.setPropertyValue(name, str(value))
        with patch.object(LoadVesuvioFixedFoil.ms, "Load", side_effect=lambda **kwargs: self.load_synthetic(order, **kwargs)):
            alg.execute()
        return mtd["fixed_test"]

    def test_factory_registration_and_original_algorithm_are_available(self):
        self.assertTrue(AlgorithmFactory.exists("LoadVesuvioFixedFoil"))
        original = AlgorithmManager.create("LoadVesuvio")
        original.initialize()
        self.assertFalse(original.existsProperty("FoilInSpectra"))

    def test_fixed_states_use_all_periods_independent_of_order(self):
        # Four independently counted periods, with one fixed state per detector.
        order = [0, 1, 2, 3]
        result = self.run_algorithm(order)
        actual_y = result.y(0).copy()
        actual_e = result.e(0).copy()
        counts = sum(
            (self.shapes * np.array([p + 1, 2 * p + 1, 3 * p + 1])[:, None] for p in order),
            np.zeros_like(self.shapes),
        )
        widths = np.diff(self.x)
        times = self.x[1:] - (self.x[2] - self.x[1])
        monitor = sum((p + 1) * 100.0 for p in order)
        mon_norm = np.sum((np.full(len(widths), monitor) / widths)[(times >= 600) & (times < 700)])
        scale = 1000.0 / mon_norm
        foil_in = (counts[0] + counts[1]) * scale / widths
        foil_out = counts[2] * scale / widths
        reference = (times >= 400) & (times < 450)
        ratio = np.sum(foil_out[reference]) / np.sum(foil_in[reference])
        expected_y = ratio * foil_in - foil_out
        expected_e = np.sqrt(ratio**2 * (counts[0] + counts[1]) + counts[2]) * scale / widths
        n = len(actual_y)
        np.testing.assert_allclose(actual_y, expected_y[:n], rtol=1e-12, atol=1e-12)
        np.testing.assert_allclose(actual_e, expected_e[:n], rtol=1e-12, atol=1e-12)
        self.assertEqual(set(result.getSpectrum(0).getDetectorIDs()), {3101, 3102, 3109})
        shuffled = self.run_algorithm([3, 0, 2, 1])
        np.testing.assert_allclose(shuffled.y(0), actual_y, rtol=1e-12, atol=1e-12)
        np.testing.assert_allclose(shuffled.e(0), actual_e, rtol=1e-12, atol=1e-12)

    def test_multiple_runs_and_monitor_output(self):
        single = self.run_algorithm([0, 1, 2])
        expected_y, expected_e = single.y(0).copy(), single.e(0).copy()
        combined = self.run_algorithm([0, 1, 2], Filename="14188,14188", LoadMonitors="1")
        np.testing.assert_allclose(combined.y(0), expected_y, rtol=1e-12, atol=1e-12)
        np.testing.assert_allclose(combined.e(0), expected_e / np.sqrt(2), rtol=1e-12, atol=1e-12)
        self.assertIn("fixed_test_monitors", mtd)
        np.testing.assert_allclose(mtd["fixed_test_monitors"].y(0), np.full(9, 1200.0))

    def test_single_period_does_not_require_a_foil_cycling_pattern(self):
        result = self.run_algorithm([0])
        self.assertEqual(result.getNumberHistograms(), 1)
        self.assertTrue(np.all(np.isfinite(result.y(0))))

    def test_invalid_assignments_are_rejected(self):
        for properties in (
            {"FoilInSpectra": ""},
            {"FoilOutSpectra": ""},
            {"FoilOutSpectra": "135"},
            {"FoilInSpectra": "134"},
            {"FoilInSpectra": "135,135"},
        ):
            with self.subTest(properties=properties), self.assertRaises(RuntimeError):
                self.run_algorithm([0, 1], **properties)

    def test_zero_monitor_fails_instead_of_returning_invalid_data(self):
        def load_with_zero_monitor(**kwargs):
            self.load_synthetic([0, 1], **kwargs)
            for workspace in mtd[kwargs["OutputWorkspace"] + "_monitors"]:
                workspace.mutableY(0)[:] = 0

        alg = AlgorithmManager.create("LoadVesuvioFixedFoil")
        alg.initialize()
        alg.setRethrows(True)
        for name, value in {
            "Filename": "14188",
            "FoilInSpectra": "135-136",
            "FoilOutSpectra": "143",
            "OutputWorkspace": "fixed_test",
        }.items():
            alg.setPropertyValue(name, value)
        with patch.object(LoadVesuvioFixedFoil.ms, "Load", side_effect=load_with_zero_monitor):
            with self.assertRaisesRegex(RuntimeError, "monitor signal must be positive"):
                alg.execute()


if __name__ == "__main__":
    unittest.main()
