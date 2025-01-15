# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, FileProperty, FileAction, WorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction, V3D
from mantid.geometry import SymmetryOperationFactory, SpaceGroupFactory
from os import path, makedirs
import numpy as np


class SaveINS(PythonAlgorithm):
    LATT_TYPE_MAP = {type: itype + 1 for itype, type in enumerate(["P", "I", "R", "F", "A", "B", "C"])}
    IDENTIY_OP = SymmetryOperationFactory.createSymOp("x,y,z")
    INVERSION_OP = SymmetryOperationFactory.createSymOp("-x,-y,-z")
    ROTATION_OPS = {1: [IDENTIY_OP, INVERSION_OP], -1: [IDENTIY_OP]}
    CENTERING_OPS = {
        1: [IDENTIY_OP],
        2: [
            SymmetryOperationFactory.createSymOp("x+1/2,y+1/2,z+1/2"),
        ],
        3: [
            SymmetryOperationFactory.createSymOp("x+1/3,y+2/3,z+2/3"),
            SymmetryOperationFactory.createSymOp("x+2/3,y+1/3,z+1/3"),
        ],
        4: [
            SymmetryOperationFactory.createSymOp("x,y+1/2,z+1/2"),
            SymmetryOperationFactory.createSymOp("x+1/2,y,z+1/2"),
            SymmetryOperationFactory.createSymOp("x+1/2,y+1/2,z"),
        ],
        5: [
            SymmetryOperationFactory.createSymOp("x,y+1/2,z+1/2"),
        ],
        6: [
            SymmetryOperationFactory.createSymOp("x,y+1/2,z"),
        ],
        7: [
            SymmetryOperationFactory.createSymOp("x,y,z"),
        ],
    }
    DUMMY_WAVELENGTH = 1.0

    def category(self):
        return "DataHandling\\Text;Crystal\\DataHandling"

    def summary(self):
        return "Saves .ins input file for SHELX single-crystal refinement."

    def PyInit(self):
        """Initilize the algorithms properties"""

        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", Direction.Input),
            doc="The name of the workspace from which to extract the information required for the "
            ".ins file. Note the workspace must have an oriented lattice/UB and a sample material"
            "set (see SetSample for details).",
        )

        self.declareProperty(
            FileProperty("Filename", "", action=FileAction.Save, direction=Direction.Input),
            doc="File with the data from a phonon calculation.",
        )

        self.declareProperty(
            name="Spacegroup",
            defaultValue="",
            direction=Direction.Input,
            doc="Spacegroup Hermann–Mauguin symbol - if not specified then the spacegroup will be "
            "taken from the CrystalStructure stored in the workspace. If a spacegroup is provided "
            "it will be used in preference to the spacegroup in the CrystalStructre.",
        )

        self.declareProperty(
            name="UseNaturalIsotopicAbundances",
            defaultValue=True,
            direction=Direction.Input,
            doc="If True the scattering lengths will not be explicitly output and SHELX will use "
            "the weighted mean values for natural isotopic abundances. If False the scattering "
            "lengths stored in the sample material will be output - in this case mantid will set "
            "the covalent radii to be 0, the user will need to edit the file before performing a "
            "refinement in SHELX.",
        )

    def validateInputs(self):
        issues = dict()
        # check workspace has a crystal structure stored
        ws = self.getProperty("InputWorkspace").value
        sample = ws.sample()
        spgr_sym = self.getProperty("Spacegroup").value
        if spgr_sym:
            if not SpaceGroupFactory.isSubscribedSymbol(spgr_sym):
                issues["Spacegroup"] = "Not a valid spacegroup symbol."
        elif not ws.sample().hasCrystalStructure():
            issues["InputWorkspace"] = "The workspace does not have a crystal structure defined, a spacegroup must " "be provided as input."
        # check the workspace has a UB defined
        if not sample.hasOrientedLattice():
            issues["InputWorkspace"] = "Workspace must have an oriented lattice defined."
        if sample.getMaterial().numberDensity < 1e-15:
            issues["InputWorkspace"] = "Workspace must have a sample material set."
        return issues

    def PyExec(self):
        """Execute the algorithm"""
        ws = self.getProperty("InputWorkspace").value
        filename = self.getPropertyValue("Filename")
        spgr_sym = self.getProperty("Spacegroup").value
        use_natural_abundances = self.getProperty("UseNaturalIsotopicAbundances").value

        # get wavelength (for CW instruments) if set on workspace otherwise use 1 Ang
        run = ws.run()
        if run.hasProperty("wavelength"):
            wl = run.getPropertyAsSingleValue("wavelength")
        else:
            wl = self.DUMMY_WAVELENGTH
        sample = ws.sample()
        cell = sample.getOrientedLattice()
        spgr = SpaceGroupFactory.createSpaceGroup(spgr_sym) if spgr_sym else sample.getCrystalStructure().getSpaceGroup()
        material = sample.getMaterial()
        dirname = path.dirname(filename)
        if not path.exists(dirname):
            makedirs(dirname)
        with open(filename, "w") as f_handle:
            f_handle.write(f"TITL {ws.name()}\n")  # title
            f_handle.write("REM This file was produced by mantid using SaveINS\n")  # comment line

            # cell parameters
            alatt = [cell.a(), cell.b(), cell.c(), cell.alpha(), cell.beta(), cell.gamma()]
            f_handle.write(f"CELL {wl:.1f} {' '.join([f'{param:.4f}' for param in alatt])}\n")
            # n formula units and cell parameter errors
            nfu = material.numberDensity * cell.volume()
            errors = [cell.errora(), cell.errorb(), cell.errorc(), cell.erroralpha(), cell.errorbeta(), cell.errorgamma()]
            f_handle.write(f"ZERR {nfu:.0f} {' '.join([f'{err:.4f}' for err in errors])}\n")
            # lattice type
            latt_type = self.LATT_TYPE_MAP[spgr.getHMSymbol()[0]]
            # check if not centrosymmetric
            if not spgr.containsOperation(self.INVERSION_OP):
                latt_type = -latt_type
            f_handle.write(f"LATT {latt_type}\n")

            # print sym operations
            for sym_str in self.get_shelx_symmetry_operators(spgr, latt_type):
                f_handle.write(f"SYMM {sym_str}\n")

            # print atom info
            f_handle.write("NEUT\n")
            atoms, natoms = material.chemicalFormula()
            if use_natural_abundances:
                f_handle.write(f"SFAC {' '.join([atom.symbol for atom in atoms])}\n")
            else:
                for atom in atoms:
                    label = atom.symbol
                    xs_info = atom.neutron()
                    b = xs_info["coh_scatt_length"]
                    mf = atom.mass
                    if label != "H":
                        mu = xs_info["tot_scatt_xs"] + wl * xs_info["abs_xs"] / 1.798
                    else:
                        # use empirical formula from Howard et al. J.Appl.Cryst. (1987). 20, 120 - 122
                        # https: // doi.org / 10.1107 / S0021889887087028
                        mu = 20.6 + wl * 19.2
                    f_handle.write(f"SFAC {label} 0 0 0 0 0 0 0 0 {b:.4f} 0 0 {mu:.4f} 1.0 {mf:.4f}\n")
            f_handle.write(f"UNIT {' '.join([f'{nfu*natom:.0f}' for natom in natoms])}\n")  # total num in unit cell
            # Neutron TOF flags
            f_handle.write("MERG 0\n")  # do not merge same reflection at different lambda
            f_handle.write("HKLF 2\n")  # tells SHELX the columns saved in the reflection file
            f_handle.write("END")

    def symmetry_operation_key(self, W1, w1, W2=np.eye(3), w2=np.zeros(3)):
        """Symmetry element key for comparison"""
        W = W1 @ W2
        w = W1 @ w2 + w1
        w[w < 0] += 1
        w[w > 1] -= 1
        return tuple(np.round(W, 0).astype(int).flatten().tolist() + np.round(w, 3).tolist())

    def symmetry_matrix_vector(self, symop):
        W = np.linalg.inv(np.column_stack([symop.transformHKL(V3D(*vec)) for vec in np.eye(3)]))
        w = np.array(symop.transformCoordinates(V3D(0, 0, 0)))
        return W, w

    def get_shelx_symmetry_operators(self, spgr, latt_type):
        """Get SHELX SYMM records https://cci.lbl.gov/cctbx/shelx.html"""
        indentity = self.IDENTIY_OP.getIdentifier()
        inversion = self.INVERSION_OP.getIdentifier()
        latt_numb = abs(latt_type)
        latt_sign = 1 if latt_type > 0 else -1
        sym_ops = spgr.getSymmetryOperations()
        sym_ops_list = []
        sym_ops_set = set()
        for sym_op in sym_ops:
            W1, w1 = self.symmetry_matrix_vector(sym_op)
            sym_key = sym_op.getIdentifier()
            if sym_key != indentity and sym_key != inversion:
                S1 = self.symmetry_operation_key(W1, w1)
                if S1 not in sym_ops_set:
                    sym_ops_list.append(sym_key)
                for rot in self.ROTATION_OPS[latt_sign]:
                    W2, _ = self.symmetry_matrix_vector(rot)
                    for cent in self.CENTERING_OPS[latt_numb]:
                        _, w2 = self.symmetry_matrix_vector(cent)
                        S3 = self.symmetry_operation_key(W1, w1, W2, w2)
                        sym_ops_set.add(S3)
        return sym_ops_list


AlgorithmFactory.subscribe(SaveINS)
