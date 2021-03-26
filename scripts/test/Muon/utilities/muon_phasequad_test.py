# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.Common.muon_phasequad import MuonPhasequad
from unittest import mock


class MuonPhasequadTest(unittest.TestCase):
    def test_name(self):
        phasequad = MuonPhasequad("test", "table")
        self.assertEqual(phasequad.name, "test")

    def test_create_base_pairs(self):
        phasequad = MuonPhasequad("test", "table")
        self.assertEqual(phasequad.Re.name, "test_Re_")
        self.assertEqual(phasequad.Im.name, "test_Im_")

    def test_phase_table(self):
        phasequad = MuonPhasequad("test", "table")
        self.assertEqual(phasequad.phase_table, "table")

    def test_phase_table_swap(self):
        phasequad = MuonPhasequad("test", "table")
        self.assertEqual(phasequad.phase_table, "table")
        phasequad.phase_table = "new"
        self.assertEqual(phasequad.phase_table, "new")

    def test_update(self):
        phasequad = MuonPhasequad("test", "table")
        Re = mock.Mock()
        Im = mock.Mock()

        phasequad._Re.update_asymmetry_workspace = mock.Mock()
        phasequad._Im.update_asymmetry_workspace = mock.Mock()

        phasequad.update_asymmetry_workspaces([Re, Im], 42, False)
        phasequad._Re.update_asymmetry_workspace.assert_called_once_with(Re, 42, rebin=False)
        phasequad._Im.update_asymmetry_workspace.assert_called_once_with(Im, 42, rebin=False)

    def test_update_rebin(self):
        phasequad = MuonPhasequad("test", "table")
        Re = mock.Mock()
        Im = mock.Mock()

        phasequad._Re.update_asymmetry_workspace = mock.Mock()
        phasequad._Im.update_asymmetry_workspace = mock.Mock()

        phasequad.update_asymmetry_workspaces([Re, Im], 42, True)
        phasequad._Re.update_asymmetry_workspace.assert_called_once_with(Re, 42, rebin=True)
        phasequad._Im.update_asymmetry_workspace.assert_called_once_with(Im, 42, rebin=True)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
