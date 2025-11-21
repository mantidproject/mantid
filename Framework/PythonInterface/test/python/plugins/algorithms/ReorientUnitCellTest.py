# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy as np

from mantid.api import AlgorithmManager
from mantid.geometry import OrientedLattice
from mantid.simpleapi import (
    AnalysisDataService,
    CreatePeaksWorkspace,
    DeleteWorkspace,
    mtd,
    ReorientUnitCell,
    SetUB,
)


class ReorientUnitCellTest(unittest.TestCase):
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

    def rotation_angle_of_U(self):
        oriented_lattice = mtd[self.peaks_ws_name].sample().getOrientedLattice()
        trace = oriented_lattice.getU().diagonal().sum()
        cos_theta = (trace - 1) / 2.0
        return round(np.degrees(np.arccos(cos_theta)))

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

    def test_invalid_lattice_system(self):
        """Test that specifying LatticeSystem for non-Trigonal fails validation."""
        alg = self.setup_algorithm()
        alg.setProperty("PeaksWorkspace", self.peaks_ws_name)
        alg.setProperty("CrystalSystem", "Cubic")
        alg.setProperty("LatticeSystem", "Hexagonal")
        issues = alg.validateInputs()
        self.assertIn("LatticeSystem", issues)

    def test_cubic_crystal_system(self):
        """Test reorientation with cubic crystal system."""
        # U represents a 120 degree rotation around the cube's diagonal <111>
        # U = [[0, 0, 1],  B = [[1/2, 0,   0  ],
        #      [1, 0, 0],       [0,   1/2, 0  ],
        #      [0, 1, 0]]       [0,   0,   1/2]]
        SetUB(Workspace=self.peaks_ws_name, a=2.0, b=2.0, c=2.0, alpha=90, beta=90, gamma=90, u=[0, 1, 0], v=[0, 0, 1])
        self.assertEqual(self.rotation_angle_of_U(), 120)
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Cubic", Tolerance=0.12)
        self.assertEqual(self.rotation_angle_of_U(), 0)  # U becomes the identity matrix

    def test_tetragonal_crystal_system(self):
        """Test reorientation with tetragonal crystal system."""
        # U represents a 90 degree rotation around z-axis
        U = np.array([[0, -1, 0], [1, 0, 0], [0, 0, 1]], dtype=float)
        # a=5.0, b=5.0, c=6.0, alpha=90, beta=90, gamma=90
        ol = OrientedLattice(5.0, 5.0, 6.0, 90, 90, 90)
        SetUB(Workspace=self.peaks_ws_name, UB=(U @ ol.getB()).flatten().tolist())
        self.assertEqual(self.rotation_angle_of_U(), 90)
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Tetragonal", Tolerance=0.12)
        self.assertEqual(self.rotation_angle_of_U(), 0)

    def test_trigonal_rhombohedral(self):
        """Test reorientation with trigonal rhombohedral lattice."""
        # U represents a 120 degree rotation around diagonal <111>
        # U = [[0, 0, 1],
        #      [1, 0, 0],
        #      [0, 1, 0]]
        SetUB(Workspace=self.peaks_ws_name, a=5.0, b=5.0, c=5.0, alpha=88, beta=88, gamma=88, u=[0, 1, 0], v=[0, 0, 1])
        self.assertEqual(self.rotation_angle_of_U(), 120)
        # Run the algorithm
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Trigonal", LatticeSystem="Rhombohedral", Tolerance=0.12)
        # deformation from the cube (89 degrees vs 90) means we can't get the identity matrix for U
        # but we should get a small residual rotation ~ 1 degree
        self.assertLess(self.rotation_angle_of_U(), 4)

    def test_trigonal_hexagonal(self):
        """Test reorientation with trigonal hexagonal lattice."""
        # 120-degree rotation around c-axis
        U = np.array([[-0.5, -np.sqrt(3.0) / 2, 0.0], [np.sqrt(3.0) / 2, -0.5, 0.0], [0.0, 0.0, 1.0]])
        # a=4.0, b=4.0, c=6.0, alpha=90, beta=90, gamma=120
        ol = OrientedLattice(4.0, 4.0, 6.0, 90, 90, 120)
        SetUB(Workspace=self.peaks_ws_name, UB=(U @ ol.getB()).flatten().tolist())
        self.assertEqual(self.rotation_angle_of_U(), 120)
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Trigonal", LatticeSystem="Hexagonal", Tolerance=0.12)
        self.assertEqual(self.rotation_angle_of_U(), 0)

    def test_orthorhombic_crystal_system(self):
        """Test reorientation with orthorhombic crystal system."""
        # U represents a 180 degree rotation around x-axis
        U = np.array([[1, 0, 0], [0, -1, 0], [0, 0, -1]], dtype=float)
        # a=4.0, b=5.0, c=6.0, alpha=90, beta=90, gamma=90
        ol = OrientedLattice(4.0, 5.0, 6.0, 90, 90, 90)
        SetUB(Workspace=self.peaks_ws_name, UB=(U @ ol.getB()).flatten().tolist())
        self.assertEqual(self.rotation_angle_of_U(), 180)
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Orthorhombic", Tolerance=0.12)
        self.assertEqual(self.rotation_angle_of_U(), 0)

    def test_monoclinic_crystal_system(self):
        """Test reorientation with monoclinic crystal system."""
        # U represents a 180 degree rotation around y-axis
        U = np.array([[-1, 0, 0], [0, 1, 0], [0, 0, -1]], dtype=float)
        # a=4.0, b=5.0, c=6.0, alpha=90, beta=110, gamma=90
        ol = OrientedLattice(4.0, 5.0, 6.0, 90, 110, 90)
        SetUB(Workspace=self.peaks_ws_name, UB=(U @ ol.getB()).flatten().tolist())
        self.assertEqual(self.rotation_angle_of_U(), 180)
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Monoclinic", Tolerance=0.12)
        self.assertEqual(self.rotation_angle_of_U(), 0)


if __name__ == "__main__":
    unittest.main()
