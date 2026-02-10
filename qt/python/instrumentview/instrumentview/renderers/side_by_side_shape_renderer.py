# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Shape renderer specialised for the side-by-side projection.

Extends :class:`ShapeRenderer` with per-bank scaling and selective
rotation so that detector shapes tile correctly in the flattened
side-by-side view without overlapping.
"""

import numpy as np
import pyvista as pv
from scipy.spatial import cKDTree

from mantid.dataobjects import Workspace2D
from instrumentview.renderers.shape_renderer import ShapeRenderer


class SideBySideShapeRenderer(ShapeRenderer):
    """ShapeRenderer with vertex-based overlap prevention for side-by-side.

    For each bank of detectors:

    * **Grid banks** - shapes stay axis-aligned (no 3D rotation) so
      cuboids appear as rectangles.
    * **Tube banks** - shapes are rotated into their 3D orientation
      before z-flattening so cylinders orient along the tube axis.

    Scaling is computed per-bank by projecting each detector's shape
    vertices onto the direction toward its nearest neighbours, ensuring
    no shape extends past half the inter-detector spacing.
    """

    def precompute(self, workspace: Workspace2D, bank_groups_by_detector_id: list[tuple[list[int], str]] | None) -> None:
        super().precompute(workspace)
        self._bank_groups_by_detector_id = [] if bank_groups_by_detector_id is None else bank_groups_by_detector_id

    def build_detector_mesh(self, positions: np.ndarray, model=None) -> pv.PolyData:
        if not self._precomputed:
            raise RuntimeError("SideBySideShapeRenderer.precompute() must be called before build_detector_mesh().")

        flatten_2d = model is not None and model.is_2d_projection
        indices = self._resolve_detector_indices(positions, model, masked=False)

        per_detector_scales = None
        per_detector_rotate = None
        if flatten_2d and model is not None:
            per_detector_scales, per_detector_rotate = self._compute_bank_projection_scales(indices, positions, model)

        mesh, c2d, fpd = self._assemble_mesh(
            indices,
            positions,
            flatten_2d=flatten_2d,
            per_detector_scales=per_detector_scales,
            per_detector_rotate=per_detector_rotate,
        )
        self._cell_to_detector = c2d
        self._faces_per_detector = fpd
        self._detector_mesh_ref = mesh
        return mesh

    def build_masked_mesh(self, positions: np.ndarray, model=None) -> pv.PolyData:
        if len(positions) == 0:
            return pv.PolyData()
        flatten_2d = model is not None and model.is_2d_projection
        indices = self._resolve_detector_indices(positions, model, masked=True)

        per_detector_scales = None
        per_detector_rotate = None
        if flatten_2d and model is not None:
            per_detector_scales, per_detector_rotate = self._compute_bank_projection_scales(indices, positions, model)

        mesh, _, _ = self._assemble_mesh(
            indices,
            positions,
            flatten_2d=flatten_2d,
            per_detector_scales=per_detector_scales,
            per_detector_rotate=per_detector_rotate,
        )
        return mesh

    # ------------------------------------------------------------------
    # Bank-aware scaling
    # ------------------------------------------------------------------

    def _compute_bank_projection_scales(
        self,
        det_indices: np.ndarray,
        projected_positions: np.ndarray,
        model,
    ) -> tuple[np.ndarray, np.ndarray]:
        """Compute per-detector scale factors using vertex-based overlap detection.

        For each bank of detectors this method:

        1. Computes each detector's shape extent along the direction toward
           each nearest neighbour in 2D, after applying component scale,
           optional 3D rotation, and z-flattening.
        2. Sets the scale so that the shape's furthest vertex in that
           direction does not exceed half the nearest-neighbour distance,
           preventing overlap.
        3. Takes the minimum scale across the bank so all detectors are
           drawn at the same size.

        Returns
        -------
        per_det_scale : np.ndarray
            Per-detector scale factor, shape ``(len(det_indices),)``.
        per_det_rotate : np.ndarray
            Per-detector boolean flag, True if the 3D rotation should be
            applied (tube banks), False otherwise (grid banks).
        """
        ws = model.workspace
        det_info = ws.detectorInfo()
        all_det_ids = np.array(det_info.detectorIDs())

        # Build mapping: global_det_index -> local_index in `det_indices`
        global_to_local = {int(gi): li for li, gi in enumerate(det_indices)}

        # Convert detector-ID-based bank groups to local-index-based groups
        bank_local_groups: list[tuple[list[int], str]] = []
        for bank_det_ids, bank_type in self._bank_groups_by_detector_id:
            local_indices = []
            for did in bank_det_ids:
                matches = np.where(all_det_ids == did)[0]
                if len(matches) > 0:
                    global_idx = int(matches[0])
                    if global_idx in global_to_local:
                        local_indices.append(global_to_local[global_idx])
            if local_indices:
                bank_local_groups.append((local_indices, bank_type))

        per_det_scale = np.ones(len(det_indices), dtype=np.float64)
        per_det_rotate = np.zeros(len(det_indices), dtype=bool)

        if not bank_local_groups:
            uniform = self._compute_projection_scale(det_indices, projected_positions)
            per_det_scale[:] = uniform
            return per_det_scale, per_det_rotate

        for local_indices, bank_type in bank_local_groups:
            arr_local = np.array(local_indices)

            if bank_type == "tube":
                per_det_rotate[arr_local] = True

            apply_rot = bank_type == "tube"

            if len(arr_local) < 2:
                ext = self._shape_radial_extent_2d(det_indices[arr_local[0]], apply_rotation=apply_rot)
                per_det_scale[arr_local] = 1.0 if ext < 1e-12 else 0.01 / ext
                continue

            # Nearest-neighbour distances and directions in projected 2D space.
            bank_proj_pos = projected_positions[arr_local]
            pos_2d = bank_proj_pos[:, :2]
            tree_2d = cKDTree(pos_2d)
            k = min(5, len(pos_2d))
            nn_dists, nn_idx = tree_2d.query(pos_2d, k=k)
            nn_dists = nn_dists[:, 1:]
            nn_idx = nn_idx[:, 1:]

            min_scales = np.full(len(arr_local), np.inf)

            for j in range(k - 1):
                nn_dirs = pos_2d[nn_idx[:, j]] - pos_2d
                dists = nn_dists[:, j]
                safe_dists = np.maximum(dists, 1e-12)
                nn_unit = nn_dirs / safe_dists[:, np.newaxis]

                dir_extents = np.array(
                    [
                        self._shape_extent_along_direction_2d(det_indices[li], nn_unit[i], apply_rotation=apply_rot)
                        for i, li in enumerate(arr_local)
                    ]
                )

                safe_ext = np.maximum(dir_extents, 1e-12)
                scales_j = dists / (2.0 * safe_ext)
                min_scales = np.minimum(min_scales, scales_j)

            bank_scale = float(np.min(min_scales))
            per_det_scale[arr_local] = bank_scale

        return per_det_scale, per_det_rotate

    def _shape_extent_along_direction_2d(self, det_index: int, direction_2d: np.ndarray, apply_rotation: bool = False) -> float:
        """Return the half-extent of a detector's shape along *direction_2d*.

        Vertices are scaled, optionally rotated, then projected to XY.
        The returned value is ``max(|v · direction_2d|)`` over all vertices.
        """
        shape_key = self._det_shape_keys[det_index]
        template_verts, _ = self._shape_cache[shape_key]
        if len(template_verts) == 0:
            return 0.0

        v = template_verts * self._det_scales[det_index]
        if apply_rotation:
            v = (self._det_rotations[det_index] @ v.T).T
        projections = v[:, :2] @ direction_2d
        return float(np.max(np.abs(projections)))

    def _shape_radial_extent_2d(self, det_index: int, apply_rotation: bool = False) -> float:
        """Return the maximum radial distance from origin to any vertex in 2D.

        Used as a fallback for single-detector banks.
        """
        shape_key = self._det_shape_keys[det_index]
        template_verts, _ = self._shape_cache[shape_key]
        if len(template_verts) == 0:
            return 0.0

        v = template_verts * self._det_scales[det_index]
        if apply_rotation:
            v = (self._det_rotations[det_index] @ v.T).T
        return float(np.max(np.linalg.norm(v[:, :2], axis=1)))
