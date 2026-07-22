# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import AlgorithmManager
from mantid.simpleapi import CreateMDWorkspace, CreateSampleWorkspace, DeleteWorkspace, LoadEmptyInstrument, SetUB, mtd


class MDNormTest(unittest.TestCase):
    """Fast, data-free coverage of MDNorm::validateInputs() for the monochromatic-SCD
    (MonoSCDNormalizationWorkspace) input group. These tests never call execute(), so they
    do not need real instrument geometry or trajectories."""

    def setUp(self):
        self._workspace_names = []

    def tearDown(self):
        for name in self._workspace_names:
            if mtd.doesExist(name):
                DeleteWorkspace(name)

    def _track(self, ws):
        self._workspace_names.append(ws.name())
        return ws

    def _make_mde(self, ndims=3, frame="QSample", wavelength=None, mdnorm_logs=False, oriented_lattice=False):
        if ndims == 3:
            names = "Q_x,Q_y,Q_z"
            units = "A^-1,A^-1,A^-1"
            frames = ",".join([frame] * 3)
            extents = "-10,10,-10,10,-10,10"
        else:
            names = "Q_x,Q_y,Q_z,DeltaE"
            units = "A^-1,A^-1,A^-1,meV"
            frames = ",".join([frame] * 3) + ",General Frame"
            extents = "-10,10,-10,10,-10,10,-5,5"
        ws = self._track(
            CreateMDWorkspace(
                Dimensions=ndims,
                Extents=extents,
                Names=names,
                Units=units,
                Frames=frames,
                OutputWorkspace=mtd.unique_hidden_name(),
            )
        )
        dummy = self._track(LoadEmptyInstrument(InstrumentName="CORELLI", OutputWorkspace=mtd.unique_hidden_name()))
        ws.addExperimentInfo(dummy)
        if wavelength is not None:
            ws.getExperimentInfo(0).run().addProperty("wavelength", float(wavelength), True)
        if mdnorm_logs:
            ws.getExperimentInfo(0).run().addProperty("MDNorm_low", [0.0], True)
            ws.getExperimentInfo(0).run().addProperty("MDNorm_high", [10.0], True)
        if ndims > 3:
            ws.getExperimentInfo(0).run().addProperty("Ei", 20.0, True)
        if oriented_lattice:
            SetUB(ws, 5, 5, 5, 90, 90, 90)
        return ws

    @staticmethod
    def _setup_alg():
        alg = AlgorithmManager.create("MDNorm")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("RLU", False)
        return alg

    def test_valid_monochromatic_input_has_no_errors(self):
        data = self._make_mde(wavelength=1.5)
        norm = self._make_mde(wavelength=1.5)
        alg = self._setup_alg()
        alg.setProperty("InputWorkspace", data)
        alg.setProperty("MonoSCDNormalizationWorkspace", norm)
        issues = alg.validateInputs()
        self.assertEqual(issues, {})

    def test_mutually_exclusive_with_solid_angle_and_flux(self):
        data = self._make_mde(wavelength=1.5)
        norm = self._make_mde(wavelength=1.5)
        vanadium = self._track(CreateSampleWorkspace(OutputWorkspace=mtd.unique_hidden_name()))
        alg = self._setup_alg()
        alg.setProperty("InputWorkspace", data)
        alg.setProperty("MonoSCDNormalizationWorkspace", norm)
        alg.setProperty("SolidAngleWorkspace", vanadium)
        alg.setProperty("FluxWorkspace", vanadium)
        issues = alg.validateInputs()
        self.assertIn("MonoSCDNormalizationWorkspace", issues)

    def test_mutually_exclusive_with_background_workspace(self):
        data = self._make_mde(wavelength=1.5)
        norm = self._make_mde(wavelength=1.5)
        background = self._make_mde(frame="QLab", wavelength=1.5)
        alg = self._setup_alg()
        alg.setProperty("InputWorkspace", data)
        alg.setProperty("MonoSCDNormalizationWorkspace", norm)
        alg.setProperty("BackgroundWorkspace", background)
        issues = alg.validateInputs()
        self.assertIn("MonoSCDNormalizationWorkspace", issues)

    def test_rejects_inelastic_input(self):
        data = self._make_mde(ndims=4, wavelength=1.5)
        norm = self._make_mde(wavelength=1.5)
        alg = self._setup_alg()
        alg.setProperty("InputWorkspace", data)
        alg.setProperty("MonoSCDNormalizationWorkspace", norm)
        issues = alg.validateInputs()
        self.assertIn("MonoSCDNormalizationWorkspace", issues)

    def test_requires_wavelength_log(self):
        data = self._make_mde()  # no wavelength log
        norm = self._make_mde(wavelength=1.5)
        alg = self._setup_alg()
        alg.setProperty("InputWorkspace", data)
        alg.setProperty("MonoSCDNormalizationWorkspace", norm)
        issues = alg.validateInputs()
        self.assertIn("InputWorkspace", issues)

    def test_normalization_workspace_must_be_q_sample(self):
        data = self._make_mde(wavelength=1.5)
        norm = self._make_mde(frame="QLab", wavelength=1.5)
        alg = self._setup_alg()
        alg.setProperty("InputWorkspace", data)
        alg.setProperty("MonoSCDNormalizationWorkspace", norm)
        issues = alg.validateInputs()
        self.assertIn("MonoSCDNormalizationWorkspace", issues)

    def test_allows_rlu_true_for_monochromatic_input(self):
        data = self._make_mde(wavelength=1.5, oriented_lattice=True)
        norm = self._make_mde(wavelength=1.5, oriented_lattice=True)
        alg = self._setup_alg()
        alg.setProperty("InputWorkspace", data)
        alg.setProperty("MonoSCDNormalizationWorkspace", norm)
        alg.setProperty("RLU", True)
        issues = alg.validateInputs()
        self.assertEqual(issues, {})

    def test_normalization_workspace_dimension_mismatch(self):
        data = self._make_mde(ndims=3, wavelength=1.5)
        norm = self._make_mde(ndims=4, wavelength=1.5)
        alg = self._setup_alg()
        alg.setProperty("InputWorkspace", data)
        alg.setProperty("MonoSCDNormalizationWorkspace", norm)
        issues = alg.validateInputs()
        self.assertIn("MonoSCDNormalizationWorkspace", issues)


if __name__ == "__main__":
    unittest.main()
