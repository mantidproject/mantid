# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np

from mantid.api import PythonAlgorithm, AlgorithmFactory, IPeaksWorkspaceProperty
from mantid.geometry import PointGroupFactory
from mantid.kernel import Direction, FloatBoundedValidator, PropertyMode, StringListValidator
from mantid.simpleapi import mtd, TransformHKL


class ReorientUnitCell(PythonAlgorithm):
    """
    Reorients a unit cell based on crystal symmetry by finding the transformation
    that maximizes trace(U @ B @ inv(M)) across all symmetry operations M.
    """

    @staticmethod
    def get_point_group_symbol(crystal_system, lattice_system):
        """
        Get the point group symbol based on crystal system and lattice system.

        :param str crystal_system: Crystal system (Cubic, Monoclinic, etc.)
        :param str lattice_system: Lattice system (for Trigonal: Rhombohedral or Hexagonal)
        :return: Point group symbol string
        :rtype: str
        """
        point_group_map = {
            "Cubic": "m-3m",
            "Hexagonal": "6/mmm",
            "Tetragonal": "4/mmm",
            "Orthorhombic": "mmm",
            "Monoclinic": "2/m",
        }

        # Special case for Trigonal
        if crystal_system == "Trigonal":
            if lattice_system == "Rhombohedral":
                return "-3m r"
            else:  # Hexagonal
                return "-3m"

        return point_group_map[crystal_system]

    def name(self):
        return "ReorientUnitCell"

    def category(self):
        return "Crystal\\UBMatrix"

    def seeAlso(self):
        return ["TransformHKL", "FindUBUsingIndexedPeaks"]

    def summary(self):
        return "Select the unit cell most aligned to goniometer axes."

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
            doc="Indexing tolerance for algorithm TransformHKL.",
        )

        # Crystal system
        crystal_systems = ["Cubic", "Hexagonal", "Tetragonal", "Trigonal", "Orthorhombic", "Monoclinic"]
        self.declareProperty(
            name="CrystalSystem",
            defaultValue="Monoclinic",
            direction=Direction.Input,
            optional=PropertyMode.Optional,
            validator=StringListValidator(crystal_systems),
            doc="Crystal system.",
        )

        # Lattice system (optional, for Trigonal)
        lattice_systems = ["Rhombohedral", "Hexagonal"]
        self.declareProperty(
            name="LatticeSystem",
            direction=Direction.Input,
            validator=StringListValidator(lattice_systems),
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

        if crystal_system == "Trigonal" and lattice_system == "":
            issues["LatticeSystem"] = "For Trigonal system, LatticeSystem must be passed."

        return issues

    def validate_lattice(self, lattice_system):
        wsname: str = self.getPropertyValue("PeaksWorkspace")
        oriented_lattice = mtd[wsname].sample().getOrientedLattice()

        a = oriented_lattice.a()
        b = oriented_lattice.b()
        c = oriented_lattice.c()
        alpha = oriented_lattice.alpha()
        beta = oriented_lattice.beta()
        gamma = oriented_lattice.gamma()

        def all_same(x, y, z=None, value=None, tol=1e-5):
            """Check if all provided values are the same (or equal to a specified value)."""
            if z is None:
                z = x
            if value is None:
                value = x
            return np.allclose([x, y, z], value, atol=tol)

        def not_equal(x, y, tol=1e-5):
            """Check if two values are not equal."""
            return abs(x - y) > tol

        def all_different(x, y, z, tol=1e-5):
            """Check if all provided values are different from each other."""
            return not_equal(x, y, tol) and not_equal(y, z, tol) and not_equal(x, z, tol)

        if all_same(a, b, c, tol=0.01) and all_same(alpha, beta, gamma, 90, tol=1):
            lattice = "Cubic"
        elif all_same(a, b, c, tol=0.01) and all_same(alpha, beta, gamma, tol=1) and not_equal(gamma, 90, tol=1):
            lattice = "Rhombohedral"
        elif all_same(a, b, tol=0.01) and not_equal(b, c, tol=0.01) and all_same(alpha, beta, gamma, 90, tol=1):
            lattice = "Tetragonal"
        elif all_same(a, b, tol=0.01) and not_equal(b, c, tol=0.01) and all_same(alpha, beta, 90, tol=1) and all_same(gamma, 120, tol=1):
            lattice = "Hexagonal"
        elif all_different(a, b, c, tol=0.01) and all_same(alpha, beta, gamma, 90, tol=1):
            lattice = "Orthorhombic"
        elif all_different(a, b, c, tol=0.01) and all_same(alpha, gamma, 90, tol=1) and not_equal(beta, 90, tol=1):
            lattice = "Monoclinic"
        else:
            lattice = "Triclinic"

        if lattice_system != lattice:
            params = f"a={a}, b={b}, c={c}, alpha={alpha}, beta={beta}, gamma={gamma}"
            raise ValueError(
                f"Lattice system encoded in the input peaks workspace '{lattice}' does not match"
                f"input CrystalSystem '{lattice_system}'.\nLattice parameters: {params}"
            )

    def PyExec(self):
        # Get input properties
        wsname: str = self.getPropertyValue("PeaksWorkspace")
        crystal_system: str = self.getProperty("CrystalSystem").value
        lattice_system: str = self.getProperty("LatticeSystem").value
        print("DEBUG: lattice_system =", lattice_system)
        # If lattice system not specified, use crystal system
        if not lattice_system:
            lattice_system = crystal_system
        self.validate_lattice(lattice_system)

        # Step 1: Fetch U and B matrices
        oriented_lattice = mtd[wsname].sample().getOrientedLattice()
        U, B = oriented_lattice.getU(), oriented_lattice.getB()

        # Step 4: Generate cell symmetry transformation matrices
        point_group_symbol = self.get_point_group_symbol(crystal_system, lattice_system)
        point_group = PointGroupFactory.createPointGroup(point_group_symbol)

        # Get symmetry operations with positive determinant to preserve handedness
        transforms = {}
        coords = np.eye(3).astype(int)  # 3x3 identity matrix representing hkl coordinates for the lattice vectors
        for sym_op in point_group.getSymmetryOperations():
            transform = np.column_stack([sym_op.transformHKL(vec) for vec in coords])
            if np.linalg.det(transform) > 0:
                name = "{}: ".format(sym_op.getOrder()) + sym_op.getIdentifier()
                transforms[name] = transform

        # Step 5: Find the symmetry operation that maximizes trace(U @ B @ inv(M)) = trace(U' @ B)
        max_trace = -np.inf
        optimal_transform = None
        transform_name = None
        for name, transform in transforms.items():
            try:
                transform_inv = np.linalg.inv(transform)
                trace = np.trace(U @ B @ transform_inv)
                if trace > max_trace:
                    max_trace = trace
                    optimal_transform = transform
                    transform_name = name
            except np.linalg.LinAlgError:  # Skip matrices that are not invertible due to numerical issues
                continue

        # Step 6: Apply the optimal transformation
        if optimal_transform is not None:
            self.log().notice(f"Aligning symmetry operation: {transform_name}")
            # Determine whether to use FindError based on number of peaks
            find_error = mtd[wsname].getNumberPeaks() > 3
            (
                TransformHKL(
                    PeaksWorkspace=wsname,
                    Tolerance=self.getProperty("Tolerance").value,
                    HKLTransform=optimal_transform,
                    FindError=find_error,
                ),
            )

        self.setProperty("PeaksWorkspace", mtd[wsname])


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ReorientUnitCell)
