# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy as np

from mantid.api import AlgorithmManager
from mantid.simpleapi import (
    AnalysisDataService,
    CreatePeaksWorkspace,
    DeleteWorkspace,
    mtd,
    ReorientUnitCell,
    SetUB,
)


class ReorientUnitCellTest(unittest.TestCase):
    R = np.array([[0, 1, 0], [0, 0, 1], [1, 0, 0]])

    def setUp(self):
        """Set up test fixtures."""
        # Create a simple peaks workspace for testing
        self.peaks_ws_name = mtd.unique_hidden_name()
        CreatePeaksWorkspace(NumberOfPeaks=0, OutputType="LeanElasticPeak", OutputWorkspace=self.peaks_ws_name)

    def tearDown(self):
        """Clean up after tests."""
        if AnalysisDataService.doesExist(self.peaks_ws_name):
            DeleteWorkspace(self.peaks_ws_name)

    @staticmethod
    def setup_algorithm():
        alg = AlgorithmManager.create("ReorientUnitCell")
        alg.setChild(True)
        alg.initialize()
        return alg

    def test_missing_ub_matrix(self):
        """Test that algorithm fails if workspace has no UB matrix."""
        # Create a workspace without UB
        test_ws = mtd.unique_hidden_name()
        CreatePeaksWorkspace(OutputType="LeanElasticPeak", OutputWorkspace=test_ws)
        try:
            # This should fail validation
            alg = self.setup_algorithm()
            alg.setProperty("PeaksWorkspace", test_ws)
            alg.setProperty("CrystalSystem", "Cubic")
            issues = alg.validateInputs()
            self.assertIn("PeaksWorkspace", issues)
        finally:
            DeleteWorkspace(test_ws)

    def assert_preferred_orientation(self):
        """Assert the orientation of the beam direction and the horizontal direction."""
        ol = mtd[self.peaks_ws_name].sample().getOrientedLattice()
        ol_u = np.array(ol.getuVector())
        # u vector has a positive component along a*
        self.assertTrue(ol_u[0] > 0)
        ol_u = np.abs(ol_u / np.linalg.norm(ol_u))
        # u vector is aligned with a*
        self.assertTrue(ol_u[0] > ol_u[1] and ol_u[0] > ol_u[2], "u vector not aligned with a*")

    def test_cubic_crystal_system(self):
        # Initially: beam direction along c*, horizontal direction along a*
        SetUB(self.peaks_ws_name, a=2.0, b=2.0, c=2.0, alpha=90, beta=90, gamma=90, u=[0, 0, 2], v=[2, 0, 0])
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Cubic", Tolerance=0.12)
        self.assert_preferred_orientation()

    def test_tetragonal_crystal_system(self):
        """Test reorientation with tetragonal crystal system."""
        # Initially: beam direction along b*, horizontal direction along -a*
        SetUB(self.peaks_ws_name, a=5.0, b=5.0, c=6.0, alpha=90, beta=90, gamma=90, u=[0, 5, 0], v=[-5, 0, 0])
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Tetragonal", Tolerance=0.12)
        self.assert_preferred_orientation()

    def test_trigonal_hexagonal(self):
        """Test reorientation with trigonal hexagonal lattice."""
        # Initially, vector `u` (Z axis) points along vector `v* = -a*/2 + b*`, and vector `v` (X axis) along `a*`
        SetUB(self.peaks_ws_name, a=1.0, b=1.0, c=2.0, alpha=90, beta=90, gamma=120, u=[-1 / 2, 1, 0], v=[1, 0, 0])
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Trigonal", LatticeSystem="Hexagonal", Tolerance=0.12)
        # Now `u` points along `a* - b*/2`, and `v` points along `+b`
        self.assert_preferred_orientation()

    def test_trigonal_rhombohedral(self):
        """Test reorientation with trigonal rhombohedral lattice."""
        # Direct lattice: a=1.0, b=1.0, c=1.0, alpha=60, beta=60, gamma=60
        # Reciprocal lattice: a*=1.22474, b*=1.22474, c*=1.22474, alpha*=109.471, beta*=109.471, gamma*=109.471
        # When U == I, Z axis points along `w* = 1/2 a* + 1/2 b* + c*`, and X-axis along `u* = a*`
        SetUB(self.peaks_ws_name, a=1.0, b=1.0, c=1.0, alpha=60, beta=60, gamma=60, u=[1, 1, 2], v=[1, 0, 0])
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Trigonal", LatticeSystem="Rhombohedral", Tolerance=0.12)
        self.assert_preferred_orientation()

    def test_orthorhombic_crystal_system(self):
        """Test reorientation with orthorhombic crystal system."""
        # Initially, Z axis points along `-a*`, X-axis along along `c*`
        SetUB(self.peaks_ws_name, a=1.0, b=2.0, c=3.0, alpha=90, beta=90, gamma=90, u=[-1, 0, 0], v=[0, 0, 1])
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Orthorhombic", Tolerance=0.12)
        # Now Z-axis points along `+a*`, X-axis along `-c*`
        self.assert_preferred_orientation()

    def test_monoclinic_crystal_system(self):
        # Reciprocal lattice is also monoclinic
        # Initially, Z axis points along `-a*`, X-axis along along `b*`
        SetUB(self.peaks_ws_name, a=1.0, b=2.0, c=3.0, alpha=90, beta=110, gamma=90, u=[-1, 0, 0], v=[0, 1, 0])
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Monoclinic", Tolerance=0.12)
        # now Z axis points along `+a*`, X-axis along `b*`
        self.assert_preferred_orientation()


if __name__ == "__main__":
    unittest.main()
