# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy as np
from numpy.testing import assert_allclose

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import AddSampleLog, CloneWorkspace, ConvertToPointData, LoadEmptyInstrument, MaskDetectors, Rebin
from plugins.algorithms.poldi_calibration_utils import (
    BankModules,
    CalibrationFit,
    FoldedData,
    PixelGeometry,
    RigidBanks,
    TimingModel,
    apply_geometry,
    component_index,
    model_counts,
)
from plugins.algorithms.poldi_utils import exec_alg, get_tof_d1Ang, get_tths_and_l2s, get_instrument_settings_from_log

BANKS = ["SouthBank", "NorthBank"]
SI_D = np.array([3.1356, 1.9202, 1.6375, 1.3578, 1.2460, 1.1086, 1.0452, 0.9601, 0.9180, 0.8587])
TRUTH = dict(kappa=0.0035, srel=0.0009, A=np.linspace(1.0, 0.2, SI_D.size))


def poldi_workspace(speed, name, live_every=28):
    """Empty POLDI instrument on one folded cycle at `speed`, with all but every `live_every`-th spectrum masked."""
    cycle_time = 60e6 / (4 * speed)
    ws = LoadEmptyInstrument(Filename="POLDI_Definition.xml", OutputWorkspace=name)
    ws = Rebin(ws, Params=f"0,{cycle_time / 500},{cycle_time}", OutputWorkspace=name)
    ws = ConvertToPointData(ws, OutputWorkspace=name)
    AddSampleLog(ws, LogName="chopperspeed", LogText=str(speed), LogType="Number")
    nspec = ws.getNumberHistograms()
    MaskDetectors(ws, WorkspaceIndexList=[i for i in range(nspec) if i % live_every])
    return ws


def simulated_counts(geom, data, q_banks, tz, rng):
    """Model counts at a known geometry and tz, with per-spectrum scale and background, no noise."""
    c, sin_t = geom.difc(q_banks)
    m = model_counts(geom, data, c, sin_t, tz, TRUTH["kappa"], TRUTH["srel"], TRUTH["A"], SI_D)
    scale = rng.uniform(5e3, 1e4, (m.shape[0], 1))
    return np.where(geom.live[:, None], scale * m + rng.uniform(5, 20, (m.shape[0], 1)), 0.0)


class PoldiCalibrationUtilsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ws = poldi_workspace(5000, "poldi")
        cls.geom = PixelGeometry(cls.ws, BANKS)

    @classmethod
    def tearDownClass(cls):
        ADS.clear()

    def _data(self, speed):
        return FoldedData(np.zeros((self.geom.live.size, 500)), 60e6 / (4 * speed))

    def test_component_index_resolves_names_and_paths(self):
        ci = self.ws.componentInfo()

        block = component_index(ci, "NorthBank/detector-bank/reference-sphere-1/block1")

        self.assertEqual(ci.name(block), "block1")
        self.assertEqual(ci.name(ci.parent(ci.parent(ci.parent(block)))), "NorthBank")
        self.assertEqual(component_index(ci, "SouthBank"), ci.indexOfAny("SouthBank"))

    def test_bank_and_block_moves_match_mantid(self):
        q = self.geom.q0 + [0.004, -0.006, 0.4, -0.003, 0.005, -0.3]
        q_blocks = np.random.default_rng(2).uniform([-1e-3, -0.5], [1e-3, 0.5], (len(self.geom.blocks), 2)).ravel()
        for blocks in (None, q_blocks):
            with self.subTest(blocks=blocks is not None):
                ws = CloneWorkspace(self.ws, OutputWorkspace="moved")

                apply_geometry(ws, self.geom, q, exec_alg, blocks)

                live = np.flatnonzero(self.geom.live)
                si = ws.spectrumInfo()
                assert_allclose([si.position(int(i)) for i in live], self.geom.positions(q, blocks)[live], atol=1e-9)
                l1_chop = get_instrument_settings_from_log(ws)[3]
                c_mantid = get_tof_d1Ang(ws, get_tths_and_l2s(ws, self.geom.live)[1], l1_chop, self.geom.live)
                assert_allclose(self.geom.difc(q, blocks)[0][live], c_mantid[live], rtol=1e-12)

    def test_analytic_derivatives_match_finite_differences(self):
        data = self._data(5000)
        c, sin_t = self.geom.difc(self.geom.q0)
        args = dict(tz=160.0, kappa=TRUTH["kappa"], srel=TRUTH["srel"], A=TRUTH["A"])

        def model(**changes):
            kw = {**args, **changes}
            return model_counts(self.geom, data, kw.pop("c", c), sin_t, d_hkl=SI_D, **kw)

        _, dm_dc, dm_dtz, dm_dka, dm_dsr, dm_dA = model_counts(self.geom, data, c, sin_t, d_hkl=SI_D, deriv=True, **args)
        for name, analytic, value, step in (
            ("c", dm_dc, c, 1e-3),  # each pixel's counts depend only on its own c
            ("tz", dm_dtz, args["tz"], 1e-3),
            ("kappa", dm_dka, args["kappa"], 1e-7),
            ("srel", dm_dsr, args["srel"], 1e-7),
        ):
            with self.subTest(parameter=name):
                numeric = (model(**{name: value + step}) - model(**{name: value - step})) / (2 * step)
                assert_allclose(analytic, numeric, atol=1e-4 * np.abs(numeric).max())
        for k in (0, SI_D.size - 1):  # the model is linear in the intensities
            assert_allclose(dm_dA[k], model(A=np.eye(SI_D.size)[k]), atol=1e-12)

    def test_recovers_bank_geometry_and_timing(self):
        rng = np.random.default_rng(3)
        speeds = [5000, 10000]
        cycle_times = [60e6 / (4 * s) for s in speeds]
        q_true = self.geom.q0 + [0.003, 0.0, 0.3, 0.0, -0.002, -0.2]
        t0, t0_const = 0.05824, -12.68
        for kind in ("PerChopperSpeed", "Linear"):
            with self.subTest(timing=kind):
                timing = TimingModel(kind, cycle_times, speeds, t0, t0_const)
                tz_true = {
                    "PerChopperSpeed": [t0 * T + t0_const + 2.0 + 0.5 * i for i, T in enumerate(cycle_times)],  # not linear in T
                    "Linear": [(t0 + 1e-4) * T + t0_const + 2.0 for T in cycle_times],
                }[kind]
                datasets = [
                    FoldedData(simulated_counts(self.geom, self._data(s), q_true, tz, rng), T)
                    for s, T, tz in zip(speeds, cycle_times, tz_true)
                ]

                fit = CalibrationFit(self.geom, datasets, RigidBanks(self.geom), SI_D, timing).fit()

                assert_allclose(fit.q_banks, q_true, atol=1e-6)
                assert_allclose(timing.matrix @ fit.x_tz, tz_true, atol=1e-4)

    def test_recovers_block_moves_with_banks_fixed(self):
        ws = poldi_workspace(5000, "poldi_dense", live_every=8)  # several live pixels per block
        geom = PixelGeometry(ws, BANKS)
        q_blocks = np.random.default_rng(6).uniform([-1e-3, -0.5], [1e-3, 0.5], (len(geom.blocks), 2)).ravel()
        timing = TimingModel("PerChopperSpeed", [3000.0], [5000], 0.05824, -12.68)
        c, sin_t = geom.difc(geom.q0, q_blocks)
        m = model_counts(geom, self._data(5000), c, sin_t, 160.0, TRUTH["kappa"], TRUTH["srel"], TRUTH["A"], SI_D)
        y = np.where(geom.live[:, None], 1e4 * m + 10.0, 0.0)
        modules = BankModules(geom, geom.q0)
        fit = CalibrationFit(geom, [FoldedData(y, 3000.0)], modules, SI_D, timing)
        fit.free[: modules.n_bank_params] = False

        fit.fit()

        assert_allclose(fit.q_blocks, q_blocks, atol=1e-6)

    def test_masked_spectra_do_not_contribute(self):
        rng = np.random.default_rng(4)
        y = simulated_counts(self.geom, self._data(5000), self.geom.q0, 160.0, rng)
        masked = int(np.flatnonzero(~self.geom.live)[0])
        timing = TimingModel("PerChopperSpeed", [3000.0], [5000], 0.05824, -12.68)
        costs = []
        for extra in (0.0, 1e4):
            y_masked = y.copy()
            y_masked[masked] += extra
            fit = CalibrationFit(self.geom, [FoldedData(y_masked, 3000.0)], RigidBanks(self.geom), SI_D, timing)
            costs.append(fit.evaluate(fit.x))

        self.assertEqual(costs[0], costs[1])

    def test_unreachable_reflection_is_dropped(self):
        timing = TimingModel("PerChopperSpeed", [3000.0], [5000], 0.05824, -12.68)
        y = simulated_counts(self.geom, self._data(5000), self.geom.q0, 160.0, np.random.default_rng(5))

        fit = CalibrationFit(self.geom, [FoldedData(y, 3000.0)], RigidBanks(self.geom), np.append(SI_D, 20.0), timing)

        assert_allclose(fit.d_hkl, SI_D)


if __name__ == "__main__":
    unittest.main()
