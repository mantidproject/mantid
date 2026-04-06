# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Renderer that draws detectors using their actual geometric shapes.

The shape data is obtained via ``componentInfo.shape(index).getMesh()``
which returns a triangle mesh in the component's local reference frame.
Vertices are then scaled, rotated and translated to world coordinates for
each detector.  All individual meshes are merged into a single
``pv.PolyData`` so that VTK issues only one GPU draw call, which is
critical for instruments with >100 k detectors.

A cell-to-detector index map is maintained so that VTK cell-picking on the
surface can be translated back to a logical detector index.
"""

import numpy as np
import pyvista as pv
from pyvistaqt import BackgroundPlotter
from scipy.spatial.transform import Rotation
from typing import Callable, Optional
from vtkmodules.vtkRenderingCore import vtkCellPicker

from instrumentview.Projections.Projection import Projection
from instrumentview.Projections.ProjectionType import ProjectionType
from instrumentview.renderers.base_renderer import InstrumentRenderer
from mantid.kernel import logger


class ShapeRenderer(InstrumentRenderer):
    """Renders detectors as their true geometric shapes (cuboids, cylinders, …).

    Shape data is fetched once from the workspace's ``ComponentInfo`` and
    cached.  The heavy work (mesh duplication, rotation, translation) is
    fully vectorised with NumPy/SciPy.
    """

    _MASKED_COLOUR = (0.25, 0.25, 0.25)
    _PICKING_TOLERANCE = 0.0001

    def __init__(self, workspace):
        self._workspace = workspace
        # Populated by ``precompute``.
        self._precomputed = False
        # Per-unique-shape: {xml_hash: (local_verts (V,3), local_faces (F,3))}
        self._shape_cache: dict[int, tuple[np.ndarray, np.ndarray]] = {}
        # Per-detector (all detectors, same order as _detector_ids in model):
        #   shape_key, position, rotation matrix (3x3), scale (3,)
        self._det_shape_keys: np.ndarray | None = None  # (N,) int64
        self._det_rotations: np.ndarray | None = None  # (N, 3, 3)
        self._det_scales: np.ndarray | None = None  # (N, 3)
        self._all_positions_3d: np.ndarray | None = None
        # Built during ``build_detector_mesh``:
        self._cell_to_detector: np.ndarray | None = None  # (total_cells,) → detector idx
        self._faces_per_detector: np.ndarray | None = None  # (N,)
        # Reference to the most recently built detector surface mesh
        self._detector_mesh_ref: pv.PolyData | None = None

    # -----------------------------------------------------------------
    # Pre-computation: fetch shape meshes and detector transforms once
    # -----------------------------------------------------------------
    def precompute(self) -> None:
        """Extract shape meshes and per-detector transforms from *workspace*.

        This should be called once when the workspace is first loaded or
        replaced, *before* any ``build_*`` calls.
        """
        comp_info = self._workspace.componentInfo()
        det_info = self._workspace.detectorInfo()
        n_det = det_info.size()

        shape_cache: dict[int, tuple[np.ndarray, np.ndarray]] = {}
        det_shape_keys = np.empty(n_det, dtype=np.int64)
        det_rotations = np.empty((n_det, 3, 3), dtype=np.float64)
        det_scales = np.empty((n_det, 3), dtype=np.float64)
        all_positions = np.empty((n_det, 3), dtype=np.float64)

        for i in range(n_det):
            # Position (V3D → array)
            pos = det_info.position(i)
            all_positions[i] = [pos.X(), pos.Y(), pos.Z()]

            # Rotation (Quat → scipy rotation matrix)
            q = comp_info.rotation(i)
            # Mantid Quat: (w, x, y, z); scipy wants (x, y, z, w)
            rot = Rotation.from_quat([q.imagI(), q.imagJ(), q.imagK(), q.real()])
            det_rotations[i] = rot.as_matrix()

            sc = comp_info.scaleFactor(i)
            det_scales[i] = [sc.X(), sc.Y(), sc.Z()]

            # Shape — hash the XML string for fast deduplication
            if not comp_info.hasValidShape(i):
                det_shape_keys[i] = 0
                if 0 not in shape_cache:
                    shape_cache[0] = _make_fallback_shape()
                continue

            shape_obj = comp_info.shape(i)
            xml = shape_obj.getShapeXML()
            key = hash(xml)
            det_shape_keys[i] = key

            if key not in shape_cache:
                try:
                    raw_mesh = shape_obj.getMesh()  # (N_tri, 3, 3)
                    if raw_mesh.size == 0:
                        shape_cache[key] = _make_fallback_shape()
                    else:
                        verts, faces = _triangles_to_verts_faces(raw_mesh)
                        shape_cache[key] = (verts, faces)
                except Exception:
                    shape_cache[key] = _make_fallback_shape()
                    logger.information(f"ShapeRenderer: failed to get mesh for shape for detector {i}, using fallback")

        self._shape_cache = shape_cache
        self._det_shape_keys = det_shape_keys
        self._det_rotations = det_rotations
        self._det_scales = det_scales
        self._all_positions_3d = all_positions
        self._precomputed = True
        logger.information(f"ShapeRenderer: precomputed {n_det} detectors, {len(shape_cache)} unique shapes")

    # -----------------------------------------------------------------
    # Build meshes
    # -----------------------------------------------------------------
    def build_detector_mesh(self, positions: np.ndarray, flip_z: bool, model) -> pv.PolyData:
        """Build a surface mesh for the unmasked detectors whose centres are *positions*.

        *positions* may already be projected to 2D (with z=0).  In that case
        the *model* must expose ``detector_positions_3d_pickable`` so we can
        compute the 3D→2D offset per detector and apply it to every vertex.
        """
        if not self._precomputed:
            self.precompute()

        indices = self._resolve_detector_indices(model.pickable_detector_ids)

        mesh, c2d, fpd = self._assemble_mesh(
            detector_indices=indices,
            detector_positions=positions,
            projection=model.active_projection,
            flip_z=flip_z,
        )
        self._cell_to_detector = c2d
        self._faces_per_detector = fpd
        self._detector_mesh_ref = mesh
        return mesh

    def build_pickable_mesh(self, positions: np.ndarray, flip_z: bool) -> pv.PolyData:
        """Return a copy of the detector shape mesh for picking and highlighting.

        Cell-based picking on this mesh lets the user click anywhere on
        a detector's surface to select it, using ``_cell_to_detector``
        to map the picked cell back to a detector index.
        """
        if self._detector_mesh_ref is not None and self._detector_mesh_ref.number_of_cells > 0:
            return self._detector_mesh_ref.copy(deep=True)
        if flip_z:
            positions = positions.copy()
            positions[:, 2] *= -1
        return pv.PolyData(positions)

    def build_masked_mesh(self, positions: np.ndarray, flip_z: bool, model) -> pv.PolyData:
        if len(positions) == 0:
            return pv.PolyData()
        indices = self._resolve_detector_indices(model.masked_detector_ids)

        mesh, _, _ = self._assemble_mesh(
            detector_indices=indices,
            detector_positions=positions,
            projection=model.active_projection,
            flip_z=flip_z,
        )
        return mesh

    def add_detector_mesh_to_plotter(
        self, plotter: BackgroundPlotter, mesh: pv.PolyData, is_projection: bool, scalars: Optional[str] = None
    ) -> None:
        if mesh.number_of_cells == 0:
            return
        scalar_bar_args = dict(interactive=True, vertical=False, title_font_size=15, label_font_size=12) if scalars is not None else None
        plotter.add_mesh(
            mesh,
            pickable=False,
            scalars=scalars,
            show_edges=False,
            scalar_bar_args=scalar_bar_args,
        )

        if plotter.off_screen:
            return

    def add_pickable_mesh_to_plotter(self, plotter: BackgroundPlotter, mesh: pv.PolyData, scalars) -> None:
        if mesh.number_of_cells == 0:
            return

        actor = plotter.add_mesh(
            mesh,
            scalars=scalars,
            opacity=[0.0, 0.3],
            clim=[0, 1],
            show_scalar_bar=False,
            pickable=True,
            cmap="Oranges",
            show_edges=False,
        )
        # Polygon offset so highlight renders in front of the detector surface
        mapper = actor.mapper
        mapper.SetResolveCoincidentTopologyToPolygonOffset()
        mapper.SetResolveCoincidentTopologyPolygonOffsetParameters(-1, -1)

    def add_masked_mesh_to_plotter(self, plotter: BackgroundPlotter, mesh: pv.PolyData) -> None:
        if mesh.number_of_cells == 0:
            return
        plotter.add_mesh(
            mesh,
            color=self._MASKED_COLOUR,
            pickable=False,
            show_edges=False,
        )

    def enable_picking(self, plotter: BackgroundPlotter, callback: Callable[[int], None]) -> None:
        """Set up left-click cell picking on the shape surface.  *callback* receives ``(detector_index: int)``."""
        plotter.disable_picking()

        if plotter.off_screen:
            return

        c2d = self._cell_to_detector
        picker = vtkCellPicker()
        picker.SetTolerance(self._PICKING_TOLERANCE)
        interactor = plotter.iren

        def _on_left_button_press(obj, event):
            """Handle left mouse button press for picking."""
            if c2d is None:
                return
            # Get the current mouse position from the interactor
            x, y = interactor.get_event_position()
            # Perform the pick operation
            pick_result = picker.Pick(x, y, 0, plotter.renderer)
            if pick_result > 0:
                # Get the picked cell ID
                cell_id = picker.GetCellId()
                if cell_id >= 0:
                    callback(int(c2d[cell_id]))

        # Register callback for left button press
        plotter.iren.style.AddObserver("LeftButtonPressEvent", _on_left_button_press)

    def set_detector_scalars(self, mesh: pv.PolyData, counts: np.ndarray, label: str) -> None:
        if self._cell_to_detector is not None and len(counts) > 0:
            # _cell_to_detector[c] gives the detector index for cell c,
            # accounting for the fact that cells are grouped by shape key
            # rather than following detector index order.
            mesh.cell_data[label] = counts[self._cell_to_detector]
        else:
            # Fallback: try assigning directly
            mesh.cell_data[label] = counts

    def set_pickable_scalars(self, mesh: pv.PolyData, visibility: np.ndarray, label: str) -> None:
        if self._cell_to_detector is not None and len(visibility) > 0:
            mesh.cell_data[label] = visibility[self._cell_to_detector]
        else:
            # No shape mesh available — fall back to point data
            mesh.point_data[label] = visibility

    def _resolve_detector_indices(self, detector_ids: np.ndarray) -> np.ndarray:
        """Return indices into ``self._all_positions_3d`` for the detectors
        represented by *positions*.

        The model stores ``_detector_ids`` for all detectors in the same order
        as the arrays in the ``CreateDetectorTable`` output.  We need to map
        those to the ``detectorInfo`` indices used during precomputation.

        We match by looking up the detector IDs that correspond to the
        pickable/masked subset and converting them via ``detectorInfo.indexOf``.
        """
        det_info = self._workspace.detectorInfo()

        # Convert detector IDs → detectorInfo indices (vectorised via the
        # C++ call).  ``detectorInfo().indexOf(id)`` works element-wise in C++
        # but not in Python, so we loop.  For large instruments this is fast
        # because it's just a dict lookup in C++.
        indices = np.empty(len(detector_ids), dtype=np.int64)
        for i, did in enumerate(detector_ids):
            indices[i] = det_info.indexOf(int(did))
        return indices

    def _assemble_mesh(
        self,
        detector_indices: np.ndarray,
        detector_positions: np.ndarray,
        projection: Projection | None = None,
        per_detector_scales: np.ndarray | None = None,
        per_detector_rotate: np.ndarray | None = None,
        flip_z: bool = False,
    ) -> tuple[pv.PolyData, np.ndarray, np.ndarray]:
        """Vectorised mesh assembly.

        For each unique shape (group of detectors sharing the same template
        shape), we:

        1. Tile the template vertices for each detector in the group.
        2. Apply per-detector scale, rotation and translation.
        3. Concatenate with face arrays, offsetting vertex indices.
        4. Return a single merged ``pv.PolyData`` plus a mapping from cell
           index to detector-in-group index.

        Parameters
        ----------
        detector_indices : np.ndarray
            Indices of the detectors matching detector positions.
        detector_positions: np.ndarray
            (N, 3) positions of detectors that may be in 2d, spherical, cylindrical or side-by-side projections.
            Offers a good shortcut to centres of detector shapes.
        per_detector_scales : np.ndarray or None
            If provided, (N,) per-detector scale factors to use instead of
            a single uniform projection_scale.  Only used for side-by-side projections
        per_detector_rotate : np.ndarray or None
            If provided, (N,) boolean array.  Detectors marked True have
            their 3D rotation applied; False detectors stay axis-aligned (grid banks).
            Only used for side-by-side projection

        Returns
        -------
        mesh : pv.PolyData
        cell_to_detector : np.ndarray   (total_cells,) → index in 0..N-1
        faces_per_detector : np.ndarray  (N,)
        """
        if (
            len(detector_indices) == 0
            or self._det_shape_keys is None
            or self._det_rotations is None
            or self._det_scales is None
            or self._all_positions_3d is None
        ):
            return pv.PolyData(), np.array([], dtype=np.int64), np.array([], dtype=np.int64)

        shape_keys = self._det_shape_keys[detector_indices]
        rotations = self._det_rotations[detector_indices]  # (N, 3, 3)
        scales = self._det_scales[detector_indices]  # (N, 3)

        all_verts_list: list[np.ndarray] = []
        all_faces_list: list[np.ndarray] = []
        cell_to_det_list: list[np.ndarray] = []
        faces_per_det = np.empty(len(detector_indices), dtype=np.int64)

        vertex_offset = 0

        # Centroid of all display positions (2D) — used for z-offset
        # so detectors farther from the mesh centre sit above closer ones.
        mesh_centre_2d = detector_positions[:, :2].mean(axis=0)

        # Group detectors by shape key for batch processing
        unique_keys = np.unique(shape_keys)

        for key in unique_keys:
            mask = shape_keys == key
            group_indices = np.where(mask)[0]  # indices into det_indices/positions arrays
            n_group = len(group_indices)

            template_verts, template_faces = self._shape_cache[key]
            n_verts = len(template_verts)
            n_faces = len(template_faces)

            if n_verts == 0 or n_faces == 0:
                faces_per_det[group_indices] = 0
                continue

            # Tile template: (n_group, n_verts, 3)
            tiled = np.tile(template_verts, (n_group, 1, 1))

            # Scale
            native_scales = scales[group_indices][:, np.newaxis, :]
            tiled = tiled * native_scales

            if projection is not None and projection.type is ProjectionType.SIDE_BY_SIDE:
                if per_detector_rotate is None or per_detector_scales is None:
                    return pv.PolyData(), np.array([], dtype=np.int64), np.array([], dtype=np.int64)

                projection_scales = per_detector_scales[group_indices][:, np.newaxis, np.newaxis]
                tiled = tiled * projection_scales

                rotate_mask = per_detector_rotate[group_indices]
                group_pos = detector_positions[group_indices][:, np.newaxis, :]  # (n_group, 1, 3)
            else:
                rotate_mask = np.ones(n_group).astype(bool)
                group_pos = self._all_positions_3d[detector_indices[group_indices]][:, np.newaxis, :]

            # Rotate
            group_rots = rotations[group_indices[rotate_mask]]
            tiled[rotate_mask] = np.einsum("nij,nvj->nvi", group_rots, tiled[rotate_mask])
            # Translate
            tiled = tiled + group_pos

            if projection is not None and projection.type is not ProjectionType.SIDE_BY_SIDE:
                if flip_z:
                    tiled[:, :, 2] *= -1

                projected_vertices = projection.project_points(tiled.reshape(-1, 3), apply_x_correction=False).reshape(n_group, n_verts, 2)

                u_period = projection.u_period
                if np.isfinite(u_period) and abs(u_period) > 0.0:
                    # Keep each detector polygon contiguous at the periodic seam
                    # by wrapping vertices near the projected detector center.
                    centre_x = detector_positions[group_indices, 0][:, np.newaxis]
                    projected_vertices[:, :, 0] += np.round((centre_x - projected_vertices[:, :, 0]) / u_period) * u_period

                tiled[:, :, :2] = projected_vertices

                # Assign tiny z offsets so detectors farther from the mesh
                # centre sit above those closer, preventing picking
                # ambiguity on overlapping coplanar cells.
                group_center_dist = np.linalg.norm(detector_positions[group_indices, :2] - mesh_centre_2d, axis=1)
                tiled[:, :, 2] = group_center_dist[:, np.newaxis] * 1e-4

            # Flatten to (n_group * n_verts, 3)
            flat_verts = tiled.reshape(-1, 3)

            # --- Build VTK face array with offset vertex indices -------------
            # template_faces is (n_faces, 3).  We add vertex_offset + k*n_verts
            # for the k-th detector in this group.
            offsets = np.arange(n_group, dtype=np.int64) * n_verts + vertex_offset
            # (n_group, n_faces, 3) with broadcasting
            offset_faces = template_faces[np.newaxis, :, :] + offsets[:, np.newaxis, np.newaxis]
            # VTK format: prepend 3 to each face → (n_group*n_faces, 4)
            flat_faces = offset_faces.reshape(-1, 3)
            vtk_faces = np.hstack([np.full((len(flat_faces), 1), 3, dtype=np.int64), flat_faces])

            all_verts_list.append(flat_verts)
            all_faces_list.append(vtk_faces.ravel())

            # Cell-to-detector map: each of the n_faces cells for detector k
            # maps to group_indices[k]
            cell_map = np.repeat(group_indices, n_faces)
            cell_to_det_list.append(cell_map)

            faces_per_det[group_indices] = n_faces
            vertex_offset += n_group * n_verts

        if len(all_verts_list) == 0:
            return pv.PolyData(), np.array([], dtype=np.int64), faces_per_det

        all_verts = np.concatenate(all_verts_list, axis=0)
        all_faces = np.concatenate(all_faces_list, axis=0)
        cell_to_det = np.concatenate(cell_to_det_list, axis=0)

        mesh = pv.PolyData(all_verts, all_faces)
        return mesh, cell_to_det, faces_per_det


def _triangles_to_verts_faces(raw_mesh: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """Convert the (N_tri, 3, 3) array from ``CSGObject.getMesh()`` to
    deduplicated vertices ``(V, 3)`` and face indices ``(F, 3)``.

    Deduplication reduces memory and improves rendering quality (shared
    normals at shared vertices give smooth shading).
    """
    n_tri = raw_mesh.shape[0]
    # Flatten to (N_tri*3, 3)
    all_corners = raw_mesh.reshape(-1, 3)

    # Deduplicate vertices: round to avoid floating-point near-misses,
    # then use np.unique to find unique rows and an inverse index map.
    # inv_idx[k] gives the index into the unique (sorted) array for corner k.
    rounded = np.round(all_corners, decimals=10)
    _, inv_idx = np.unique(rounded, axis=0, return_inverse=True)

    # Recover original-precision coordinates for each unique vertex.
    # first_occ[j] = first position in all_corners that maps to unique vertex j,
    # preserving the same ordering as inv_idx so face indices remain valid.
    _, first_occ = np.unique(inv_idx, return_index=True)
    unique_verts = all_corners[first_occ]

    faces = inv_idx.reshape(n_tri, 3)
    return unique_verts, faces


def _make_fallback_shape() -> tuple[np.ndarray, np.ndarray]:
    """A tiny tetrahedron used when a detector has no valid shape."""
    s = 0.002
    verts = np.array(
        [
            [s, 0, -s / np.sqrt(2)],
            [-s, 0, -s / np.sqrt(2)],
            [0, s, s / np.sqrt(2)],
            [0, -s, s / np.sqrt(2)],
        ],
        dtype=np.float64,
    )
    faces = np.array([[0, 1, 2], [0, 1, 3], [0, 2, 3], [1, 2, 3]], dtype=np.int64)
    return verts, faces
