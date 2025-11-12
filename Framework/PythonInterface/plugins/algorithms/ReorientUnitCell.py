# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import PythonAlgorithm, AlgorithmFactory, IPeaksWorkspaceProperty
from mantid.kernel import Direction, FloatBoundedValidator, StringListValidator
from mantid.simpleapi import IndexPeaks, TransformHKL
from mantid.geometry import PointGroupFactory
import numpy as np


class ReorientUnitCell(PythonAlgorithm):
    """
    Reorients a unit cell based on crystal symmetry by finding the transformation
    that maximizes trace(U @ B @ inv(M)) across all symmetry operations.
    """

    def name(self):
        return "ReorientUnitCell"

    def category(self):
        return "Crystal\\UBMatrix"

    def seeAlso(self):
        return ["IndexPeaks", "TransformHKL", "FindUBUsingIndexedPeaks"]

    def summary(self):
        return "Reorients a unit cell based on crystal symmetry."

    def PyInit(self):
        # Input/Output properties
        self.declareProperty(
            IPeaksWorkspaceProperty(name="PeaksWorkspace", defaultValue="", direction=Direction.InOut),
            doc="The peaks workspace to reorient (modified in place).",
        )

        # Tolerance
        self.declareProperty(
            name="Tolerance",
            defaultValue=0.12,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Indexing tolerance for IndexPeaks.",
        )

        # Crystal system
        crystal_systems = ["Triclinic", "Monoclinic", "Orthorhombic", "Tetragonal", "Trigonal", "Hexagonal", "Cubic"]
        self.declareProperty(
            name="CrystalSystem",
            defaultValue="Triclinic",
            direction=Direction.Input,
            validator=StringListValidator(crystal_systems),
            doc="Crystal system.",
        )

        # Lattice system (optional, for Trigonal)
        self.declareProperty(
            name="LatticeSystem",
            defaultValue="",
            direction=Direction.Input,
            doc="Lattice system (for Trigonal: Rhombohedral or Hexagonal). Defaults to CrystalSystem value.",
        )

    def validateInputs(self):
        issues = dict()

        # Check that peaks workspace has oriented lattice
        peaks_ws = self.getProperty("PeaksWorkspace").value
        if not peaks_ws.sample().hasOrientedLattice():
            issues["PeaksWorkspace"] = "PeaksWorkspace must have an oriented lattice (UB matrix)."

        # Check lattice system is appropriate
        crystal_system = self.getProperty("CrystalSystem").value
        lattice_system = self.getProperty("LatticeSystem").value
        if lattice_system and crystal_system != "Trigonal":
            issues["LatticeSystem"] = "LatticeSystem should only be specified for Trigonal crystal system."

        if crystal_system == "Trigonal" and lattice_system:
            if lattice_system not in ["Rhombohedral", "Hexagonal"]:
                issues["LatticeSystem"] = "For Trigonal system, LatticeSystem must be Rhombohedral or Hexagonal."

        return issues

    def PyExec(self):
        # Get input properties
        peaks_ws_name = self.getPropertyValue("PeaksWorkspace")
        peaks_ws = self.getProperty("PeaksWorkspace").value
        tolerance = self.getProperty("Tolerance").value
        crystal_system = self.getProperty("CrystalSystem").value
        lattice_system = self.getProperty("LatticeSystem").value

        # If lattice system not specified, use crystal system
        if not lattice_system:
            lattice_system = crystal_system

        # Step 1: Get the oriented lattice
        ol = peaks_ws.sample().getOrientedLattice()
        U = ol.getU()
        B = ol.getB()

        # Step 2: Index peaks with the given tolerance
        IndexPeaks(PeaksWorkspace=peaks_ws_name, Tolerance=tolerance, CommonUBForAll=False)

        # Step 3: Get unique run numbers
        run_numbers = set()
        for i in range(peaks_ws.getNumberPeaks()):
            run_numbers.add(peaks_ws.getPeak(i).getRunNumber())

        # Step 4: Generate cell symmetry transformation matrices
        point_group_symbol = self._get_point_group_symbol(crystal_system, lattice_system)
        point_group = PointGroupFactory.createPointGroup(point_group_symbol)

        # Get symmetry operations with positive determinant
        sym_ops = point_group.getSymmetryOperations()
        transformation_matrices = []
        for sym_op in sym_ops:
            M = np.array(sym_op.transformationMatrix()).reshape(3, 3)
            if np.linalg.det(M) > 0:
                transformation_matrices.append(M)

        # Step 5: Minimize by finding transformation that maximizes trace(U @ B @ inv(M))
        best_trace = -np.inf
        best_M = None

        for M in transformation_matrices:
            try:
                M_inv = np.linalg.inv(M)
                trace_val = np.trace(U @ B @ M_inv)
                if trace_val > best_trace:
                    best_trace = trace_val
                    best_M = M
            except np.linalg.LinAlgError:
                # Skip singular matrices
                continue

        # Step 6: Apply the optimal transformation
        if best_M is not None:
            # Determine whether to use FindError based on number of peaks
            find_error = peaks_ws.getNumberPeaks() > 3
            TransformHKL(PeaksWorkspace=peaks_ws_name, HKLTransform=best_M, FindError=find_error)

        self.setProperty("PeaksWorkspace", peaks_ws)

    def _get_point_group_symbol(self, crystal_system, lattice_system):
        """
        Get the point group symbol based on crystal system and lattice system.

        Args:
            crystal_system: Crystal system (Triclinic, Monoclinic, etc.)
            lattice_system: Lattice system (for Trigonal: Rhombohedral or Hexagonal)

        Returns:
            Point group symbol string
        """
        point_group_map = {
            "Cubic": "m-3m",
            "Hexagonal": "6/mmm",
            "Tetragonal": "4/mmm",
            "Orthorhombic": "mmm",
            "Monoclinic": "2/m",
            "Triclinic": "-1",
        }

        # Special case for Trigonal
        if crystal_system == "Trigonal":
            if lattice_system == "Rhombohedral":
                return "-3m r"
            else:  # Hexagonal
                return "-3m"

        return point_group_map[crystal_system]


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ReorientUnitCell)
