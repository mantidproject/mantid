# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from mantid.simpleapi import LoadCIF, CreateSingleValuedWorkspace
from mantid.geometry import CrystalStructure


class CrystalPhase:
    def __init__(self, crystal_structure: CrystalStructure):
        self.xtal = crystal_structure

    @classmethod
    def from_cif(cls, cif_file: str):
        ws = CreateSingleValuedWorkspace(StoreInADS=False, EnableLogging=False)
        LoadCIF(ws, cif_file, StoreInADS=False)
        return CrystalPhase(ws.sample().getCrystalStructure())

    @classmethod
    def from_alatt(cls, alatt: np.ndarray, space_group: str = "P 1", basis: str = ""):
        alatt_str = " ".join([str(par) for par in alatt])
        xtal = CrystalStructure(alatt_str, space_group, basis)
        return CrystalPhase(xtal)

    @classmethod
    def from_string(cls, lattice: str, space_group: str, basis: str):
        xtal = CrystalStructure(lattice, space_group, basis)
        return CrystalPhase(xtal)


def get_xtal_structure(input_method: str, *args, **kwargs) -> CrystalStructure:
    match input_method:
        case "cif":
            phase = CrystalPhase.from_cif(*args, **kwargs)
            return phase.xtal
        case "array":
            phase = CrystalPhase.from_alatt(*args, **kwargs)
            return phase.xtal
        case "string":
            phase = CrystalPhase.from_string(*args, **kwargs)
            return phase.xtal
        case _:
            raise ValueError(f"input_method must be: 'cif', 'array', or 'string', '{input_method}' was provided")
