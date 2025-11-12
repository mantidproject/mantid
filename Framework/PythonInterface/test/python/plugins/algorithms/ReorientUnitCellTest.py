# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.simpleapi import (
    CreatePeaksWorkspace,
    SetUB,
    DeleteWorkspace,
    AnalysisDataService,
    ReorientUnitCell,
)


class ReorientUnitCellTest(unittest.TestCase):
    def setUp(self):
        """Set up test fixtures."""
        # Create a simple peaks workspace for testing
        self.peaks_ws_name = "test_peaks"
        CreatePeaksWorkspace(InstrumentWorkspace="", NumberOfPeaks=0, OutputWorkspace=self.peaks_ws_name)

        # Set a simple UB matrix
        # Using a simple cubic lattice with a=5.0
        SetUB(Workspace=self.peaks_ws_name, a=5.0, b=5.0, c=5.0, alpha=90, beta=90, gamma=90)

    def tearDown(self):
        """Clean up after tests."""
        if AnalysisDataService.doesExist(self.peaks_ws_name):
            DeleteWorkspace(self.peaks_ws_name)

    def test_algorithm_initialization(self):
        """Test that the algorithm can be initialized."""
        alg = ReorientUnitCell()
        self.assertIsNotNone(alg)

    def test_category(self):
        """Test algorithm category."""
        alg = ReorientUnitCell()
        self.assertEqual(alg.category(), "Crystal\\UBMatrix")

    def test_name(self):
        """Test algorithm name."""
        alg = ReorientUnitCell()
        self.assertEqual(alg.name(), "ReorientUnitCell")

    def test_summary(self):
        """Test algorithm summary."""
        alg = ReorientUnitCell()
        self.assertIsNotNone(alg.summary())
        self.assertGreater(len(alg.summary()), 0)

    def test_missing_ub_matrix(self):
        """Test that algorithm fails if workspace has no UB matrix."""
        # Create a workspace without UB
        test_ws = "test_no_ub"
        CreatePeaksWorkspace(InstrumentWorkspace="", NumberOfPeaks=0, OutputWorkspace=test_ws)

        try:
            # This should fail validation
            alg = ReorientUnitCell()
            alg.initialize()
            alg.setProperty("PeaksWorkspace", test_ws)
            alg.setProperty("CrystalSystem", "Cubic")
            issues = alg.validateInputs()
            self.assertIn("PeaksWorkspace", issues)
        finally:
            DeleteWorkspace(test_ws)

    def test_cubic_crystal_system(self):
        """Test reorientation with cubic crystal system."""
        # Add some peaks to the workspace
        ws = AnalysisDataService.retrieve(self.peaks_ws_name)

        # Run the algorithm
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Cubic", Tolerance=0.12)

        # Check that the workspace still has a UB matrix
        self.assertTrue(ws.sample().hasOrientedLattice())

    def test_trigonal_rhombohedral(self):
        """Test reorientation with trigonal rhombohedral lattice."""
        # Set appropriate lattice parameters for trigonal
        SetUB(Workspace=self.peaks_ws_name, a=5.0, b=5.0, c=5.0, alpha=85, beta=85, gamma=85)

        # Run the algorithm
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Trigonal", LatticeSystem="Rhombohedral", Tolerance=0.12)

        # Check that the workspace still has a UB matrix
        ws = AnalysisDataService.retrieve(self.peaks_ws_name)
        self.assertTrue(ws.sample().hasOrientedLattice())

    def test_trigonal_hexagonal(self):
        """Test reorientation with trigonal hexagonal lattice."""
        # Set appropriate lattice parameters for hexagonal
        SetUB(Workspace=self.peaks_ws_name, a=5.0, b=5.0, c=6.0, alpha=90, beta=90, gamma=120)

        # Run the algorithm
        ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Trigonal", LatticeSystem="Hexagonal", Tolerance=0.12)

        # Check that the workspace still has a UB matrix
        ws = AnalysisDataService.retrieve(self.peaks_ws_name)
        self.assertTrue(ws.sample().hasOrientedLattice())

    def test_all_crystal_systems(self):
        """Test that all crystal systems can be used."""
        crystal_systems = ["Triclinic", "Monoclinic", "Orthorhombic", "Tetragonal", "Hexagonal", "Cubic"]

        for crystal_system in crystal_systems:
            with self.subTest(crystal_system=crystal_system):
                # Reset UB
                SetUB(Workspace=self.peaks_ws_name, a=5.0, b=5.0, c=5.0, alpha=90, beta=90, gamma=90)

                # Run the algorithm
                ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem=crystal_system, Tolerance=0.12)

                # Check that the workspace still has a UB matrix
                ws = AnalysisDataService.retrieve(self.peaks_ws_name)
                self.assertTrue(ws.sample().hasOrientedLattice())

    def test_invalid_lattice_system_for_cubic(self):
        """Test that specifying LatticeSystem for non-Trigonal fails validation."""
        alg = ReorientUnitCell()
        alg.initialize()
        alg.setProperty("PeaksWorkspace", self.peaks_ws_name)
        alg.setProperty("CrystalSystem", "Cubic")
        alg.setProperty("LatticeSystem", "Hexagonal")

        issues = alg.validateInputs()
        self.assertIn("LatticeSystem", issues)

    def test_tolerance_parameter(self):
        """Test that tolerance parameter is properly used."""
        # Run with different tolerance values
        for tolerance in [0.05, 0.12, 0.20]:
            with self.subTest(tolerance=tolerance):
                # Reset UB
                SetUB(Workspace=self.peaks_ws_name, a=5.0, b=5.0, c=5.0, alpha=90, beta=90, gamma=90)

                # Run the algorithm
                ReorientUnitCell(PeaksWorkspace=self.peaks_ws_name, CrystalSystem="Cubic", Tolerance=tolerance)

                # Check that the workspace still has a UB matrix
                ws = AnalysisDataService.retrieve(self.peaks_ws_name)
                self.assertTrue(ws.sample().hasOrientedLattice())


if __name__ == "__main__":
    unittest.main()
