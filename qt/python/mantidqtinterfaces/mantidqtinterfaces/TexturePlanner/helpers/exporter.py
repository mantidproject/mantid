# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os
import numpy as np

from mantid.simpleapi import (
    LoadEmptyInstrument,
    SaveNexus,
)
from mantid.api import AnalysisDataService as ADS
from mantid.kernel import logger
from Engineering.texture.texture_helper import convert_to_sscanss_frame
from typing import Protocol, ValuesView, Generator
from abc import abstractmethod
from mantid.api import MatrixWorkspace
from scipy.spatial.transform import Rotation


class _WorkspaceManagerType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    updated_mesh_ws: MatrixWorkspace
    WS_REFERENCE: str

    @abstractmethod
    def copy_sample_preserving_initial_rotation(self, source_ws: MatrixWorkspace, dest_ws: MatrixWorkspace) -> None:
        pass


class _InstrumentType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    @abstractmethod
    def get_instrument(self) -> str:
        pass


class _OrientationType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    include: bool
    select: bool
    R: Rotation
    transmission: np.ndarray | None


class _OrientationTableType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    orientation_kwargs: dict

    @abstractmethod
    def values(self) -> ValuesView[_OrientationType]:
        pass


class _BaseModelType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    workspaces: _WorkspaceManagerType
    instrument: _InstrumentType
    orientations: _OrientationTableType


class OrientationExporter:
    """Writes the currently-included orientations to disk in one of four formats.

    Holds a back-reference to the TexturePlannerModel so it can pull the live
    orientation table, init rotation, sample-mesh workspace, and instrument name
    at write time.
    """

    def __init__(self, model: _BaseModelType):
        self._model = model

    def _included(self) -> Generator[_OrientationType, None, None]:
        return (o for o in self._model.orientations.values() if o.include)

    def output_as_sscanss(self, save_dir: str, filename: str) -> None:
        header = ["xyz\n"]
        lines = []
        save_file = os.path.join(save_dir, filename + ".angles")
        for orientation in self._included():
            angs = convert_to_sscanss_frame(orientation.R.as_matrix())
            lines.append(f"{np.round(angs[0], 2)}\t{np.round(angs[1], 2)}\t{np.round(angs[2], 2)}\n")
        with open(save_file, "w") as f:
            f.writelines(header + lines)
        logger.notice(f"Orientation data written to '{save_file}' as Sscanss2 Angles")

    def output_as_matrix(self, save_dir: str, filename: str) -> None:
        lines = []
        save_file = os.path.join(save_dir, filename + ".txt")
        for orientation in self._included():
            rot_mat = orientation.R.as_matrix().reshape(-1)
            lines.append("\t".join(str(x) for x in rot_mat) + "\n")
        with open(save_file, "w") as f:
            f.writelines(lines)
        logger.notice(f"Orientation data written to '{save_file}' as Rotation Matrices")

    def output_as_euler(self, save_dir: str, filename: str) -> None:
        axes = self._model.orientations.orientation_kwargs["Axes"]
        senses = self._model.orientations.orientation_kwargs["Senses"].split(",")
        lines = []
        save_file = os.path.join(save_dir, filename + ".txt")
        for orientation in self._included():
            raw = orientation.R.as_euler(axes, degrees=True)
            angle_strs = [str(np.round(float(sense) * raw[i], 2)) for i, sense in enumerate(senses)]
            lines.append("\t".join(angle_strs) + "\n")
        with open(save_file, "w") as f:
            f.writelines(lines)
        logger.notice(f"Orientation data written to '{save_file}' as Euler Angles with Scheme ({axes}) and Senses ({','.join(senses)})")

    def output_transmission_weighting(self, save_dir: str, filename: str) -> None:
        """Write one transmission-weighting value per included orientation.

        For each included orientation we take the smallest transmission factor it produced, then
        normalise those per-orientation minima against the largest value across all orientations,
        so the least strongly absorbing orientation is 1.0 and the more absorbing ones are > 1.0
        (i.e. they would need proportionally longer counting). Requires the transmission factors to
        have been estimated (Estimate Transmission Values).
        """
        included = list(self._included())
        per_orientation_min = []
        for orientation in included:
            transmission = orientation.transmission
            if transmission is None or len(transmission) == 0:
                logger.warning(
                    "Transmission weighting export skipped: transmission values have not been estimated for all included orientations"
                )
                return
            per_orientation_min.append(float(np.min(transmission)))
        if not per_orientation_min:
            logger.warning("Transmission weighting export skipped: no orientations are included")
            return
        if min(per_orientation_min) <= 0:
            logger.warning(
                "Transmission weighting export skipped: at least one included orientation has zero "
                "transmission (sample likely outside the gauge volume)"
            )
            return
        global_max = max(per_orientation_min)
        save_file = os.path.join(save_dir, filename + "_transmission_weighting.txt")
        with open(save_file, "w") as f:
            f.writelines(f"{global_max / per_min}\n" for per_min in per_orientation_min)
        logger.notice(f"Transmission weighting written to '{save_file}'")

    def output_as_reference_workspace(self, save_dir, filename):
        ref_wsname = self._model.workspaces.WS_REFERENCE
        try:
            self._build_reference_ws(ref_wsname)
            save_file = os.path.join(save_dir, filename + ".nxs")
            SaveNexus(InputWorkspace=ref_wsname, Filename=save_file)
            logger.notice(f"Reference workspace saved to '{save_file}'")
        finally:
            if ADS.doesExist(ref_wsname):
                ADS.remove(ref_wsname)

    def _build_reference_ws(self, ref_wsname):
        ref_ws = LoadEmptyInstrument(InstrumentName=self._model.instrument.get_instrument(), OutputWorkspace=ref_wsname)
        # Source from updated_mesh_ws (always identity goniometer) rather than wsname (which carries
        # the current orientation R on its goniometer). copy_sample_preserving_initial_rotation keeps
        # the baked-in initial rotation - which a plain CopySample would strip from a CSG shape - and
        # leaves the reference ws at an identity goniometer, i.e. already in its initial orientation
        # ready to save.
        self._model.workspaces.copy_sample_preserving_initial_rotation(self._model.workspaces.updated_mesh_ws, ref_ws)
