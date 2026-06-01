# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os
import numpy as np

from mantid.simpleapi import (
    CopySample,
    LoadEmptyInstrument,
    RotateSampleShape,
    SaveNexus,
)
from mantid.api import AnalysisDataService as ADS
from mantid.kernel import logger
from Engineering.texture.TextureUtils import convert_to_sscanss_frame


class OrientationExporter:
    """Writes the currently-included orientations to disk in one of four formats.

    Holds a back-reference to the TexturePlannerModel so it can pull the live
    orientation table, init rotation, sample-mesh workspace, and instrument name
    at write time.
    """

    def __init__(self, model):
        self._model = model

    def _included(self):
        return (o for o in self._model.orientations.values() if o.include)

    def output_as_sscanss(self, save_dir, filename):
        header = ["xyz\n"]
        lines = []
        save_file = os.path.join(save_dir, filename + ".angles")
        for orientation in self._included():
            angs = convert_to_sscanss_frame(orientation.R.as_matrix())
            for _ in range(self._model.n_output_points):
                lines.append(f"{np.round(angs[0], 2)}\t{np.round(angs[1], 2)}\t{np.round(angs[2], 2)}\n")
        with open(save_file, "w") as f:
            f.writelines(header + lines)
        logger.notice(f"Orientation data written to '{save_file}' as Sscanss2 Angles")

    def output_as_matrix(self, save_dir, filename):
        lines = []
        save_file = os.path.join(save_dir, filename + ".txt")
        for orientation in self._included():
            rot_mat = orientation.R.as_matrix().reshape(-1)
            for _ in range(self._model.n_output_points):
                lines.append("\t".join(str(x) for x in rot_mat) + "\n")
        with open(save_file, "w") as f:
            f.writelines(lines)
        logger.notice(f"Orientation data written to '{save_file}' as Rotation Matrices")

    def output_as_euler(self, save_dir, filename):
        axes = self._model.orientation_kwargs["Axes"]
        senses = self._model.orientation_kwargs["Senses"].split(",")
        lines = []
        save_file = os.path.join(save_dir, filename + ".txt")
        for orientation in self._included():
            raw = orientation.R.as_euler(axes, degrees=True)
            angle_strs = [str(float(sense) * raw[i]) for i, sense in enumerate(senses)]
            for _ in range(self._model.n_output_points):
                lines.append("\t".join(angle_strs) + "\n")
        with open(save_file, "w") as f:
            f.writelines(lines)
        logger.notice(f"Orientation data written to '{save_file}' as Euler Angles with Scheme ({axes}) and Senses ({','.join(senses)})")

    def output_as_reference_workspace(self, save_dir, filename):
        ref_wsname = self._model.workspaces.WS_REFERENCE
        try:
            self._build_reference_ws(ref_wsname)
            self._bake_initial_rotation_if_csg(ref_wsname)
            self._reset_goniometer_to_identity(ref_wsname)
            save_file = os.path.join(save_dir, filename + ".nxs")
            SaveNexus(InputWorkspace=ref_wsname, Filename=save_file)
            logger.notice(f"Reference workspace saved to '{save_file}'")
        finally:
            if ADS.doesExist(ref_wsname):
                ADS.remove(ref_wsname)

    def _build_reference_ws(self, ref_wsname):
        LoadEmptyInstrument(InstrumentName=self._model.instr, OutputWorkspace=ref_wsname)
        # Source from updated_mesh_ws (always identity goniometer) rather than wsname
        # (which carries the current orientation R on its goniometer).
        CopySample(
            InputWorkspace=self._model.workspaces.updated_mesh_ws,
            OutputWorkspace=ref_wsname,
            CopyName=False,
            CopyEnvironment=False,
            CopyLattice=False,
        )

    def _bake_initial_rotation_if_csg(self, ref_wsname):
        # CopySample bakes the destination goniometer matrix into the shape XML for CSG
        # shapes, which strips the initial-orientation tag that RotateSampleShape adds in
        # update_initial_shape. Re-apply the initial rotation so it is baked into the
        # saved shape. Mesh shapes already have direct vertex transformations applied in
        # update_initial_shape, so no extra rotation is needed.
        shape = ADS.retrieve(ref_wsname).sample().getShape()
        if type(shape).__name__ != "CSGObject":
            return
        rotvec = self._model.workspaces.init_R.as_rotvec(degrees=True)
        ang = float(np.linalg.norm(rotvec))
        if ang > 0:
            vec = rotvec / ang
            RotateSampleShape(ref_wsname, f"{ang},{vec[0]},{vec[1]},{vec[2]},1")

    @staticmethod
    def _reset_goniometer_to_identity(ref_wsname):
        # ensure the saved sample is in its initial orientation, not under any current goniometer rotation
        ADS.retrieve(ref_wsname).run().getGoniometer().setR(np.eye(3))
