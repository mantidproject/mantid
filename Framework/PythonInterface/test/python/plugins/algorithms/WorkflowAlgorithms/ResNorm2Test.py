# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import Load, ResNorm


class ResNorm2Test(unittest.TestCase):
    _res_ws = None
    _van_ws = None

    def setUp(self):
        self._res_ws = Load(Filename="irs26173_graphite002_res.nxs", OutputWorkspace="irs26173_graphite002_res")
        self._van_ws = Load(Filename="irs26173_graphite002_red.nxs", OutputWorkspace="irs26173_graphite002_red")

    def _validate_result(self, result):
        """
        Validates that the result workspace is of the correct type, units and shape.

        @param result Result workspace form ResNorm
        """

        self.assertTrue(isinstance(result, WorkspaceGroup))
        self.assertEqual(result.getNumberOfEntries(), 2)

        expected_names = [result.name() + "_" + n for n in ["Intensity", "Stretch"]]
        for name in expected_names:
            self.assertTrue(name in result.getNames())

        for idx in range(result.getNumberOfEntries()):
            sub_ws = result.getItem(idx)
            self.assertTrue(isinstance(sub_ws, MatrixWorkspace))
            self.assertEqual(sub_ws.blocksize(), self._van_ws.getNumberHistograms())
            self.assertEqual(sub_ws.getAxis(0).getUnit().unitID(), "MomentumTransfer")

    def test_basic(self):
        """
        Tests a basic run of ResNorm.
        """
        result = ResNorm(ResolutionWorkspace=self._res_ws, VanadiumWorkspace=self._van_ws, Version=2)
        self._validate_result(result)

    def test_with_limits(self):
        """
        Tests a basic run of ResNorm with energy limits.
        """
        result = ResNorm(ResolutionWorkspace=self._res_ws, VanadiumWorkspace=self._van_ws, EnergyMin=-0.1, EnergyMax=0.1, Version=2)
        self._validate_result(result)

    def test_with_bad_limits(self):
        """
        Tests validation for energy range.
        """
        self.assertRaisesRegex(
            RuntimeError,
            "Must be greater than EnergyMin",
            ResNorm,
            ResolutionWorkspace=self._res_ws,
            VanadiumWorkspace=self._van_ws,
            EnergyMin=0.1,
            EnergyMax=-0.1,
            Version=2,
            OutputWorkspace="wks",
        )


if __name__ == "__main__":
    unittest.main()
