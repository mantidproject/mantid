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

from typing import Callable, Optional

import numpy as np
import pyvista as pv
from pyvistaqt import BackgroundPlotter
from scipy.spatial import cKDTree
from scipy.spatial.transform import Rotation

from instrumentview.renderers.base_renderer import InstrumentRenderer
from mantid.kernel import logger


class ShapeRenderer(InstrumentRenderer):
    """Renders detectors as their true geometric shapes (cuboids, cylinders, …).

    Shape data is fetched once from the workspace's ``ComponentInfo`` and
    cached.  The heavy work (mesh duplication, rotation, translation) is
    fully vectorised with NumPy/SciPy.
    """

    _MASKED_COLOUR = (0.25, 0.25, 0.25)

    def __init__(self):
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
        # Surface highlight overlay for picked detectors
        self._detector_mesh_ref: pv.PolyData | None = None
        self._highlight_mesh: pv.PolyData | None = None
        # Cache visibility data to restore after plotter is cleared
        self._cached_visibility: np.ndarray | None = None
        self._cached_visibility_label: str | None = None

    # -----------------------------------------------------------------
    # Pre-computation: fetch shape meshes and detector transforms once
    # -----------------------------------------------------------------
    def precompute(self, workspace) -> None:
        """Extract shape meshes and per-detector transforms from *workspace*.

        This should be called once when the workspace is first loaded or
        replaced, *before* any ``build_*`` calls.
        """
        comp_info = workspace.componentInfo()
        det_info = workspace.detectorInfo()
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

            # Scale
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
    def build_detector_mesh(self, positions: np.ndarray, model=None) -> pv.PolyData:
        """Build a surface mesh for the unmasked detectors whose centres are *positions*.

        *positions* may already be projected to 2D (with z=0).  In that case
        the *model* must expose ``detector_positions_3d_pickable`` so we can
        compute the 3D→2D offset per detector and apply it to every vertex.
        """
        if not self._precomputed:
            raise RuntimeError("ShapeRenderer.precompute() must be called before build_detector_mesh().")

        flatten_2d = model is not None and model.is_2d_projection
        indices = self._resolve_detector_indices(positions, model, masked=False)

        mesh, c2d, fpd = self._assemble_mesh(indices, positions, flatten_2d=flatten_2d)
        self._cell_to_detector = c2d
        self._faces_per_detector = fpd
        self._detector_mesh_ref = mesh
        return mesh

    def build_pickable_mesh(self, positions: np.ndarray) -> pv.PolyData:
        # Use a simple point cloud for the pickable overlay — point-based
        # picking is reliable in both 2D and 3D, unlike cell picking on
        # coplanar shape surfaces.
        return pv.PolyData(positions)

    def build_masked_mesh(self, positions: np.ndarray, model=None) -> pv.PolyData:
        if len(positions) == 0:
            return pv.PolyData()
        flatten_2d = model is not None and model.is_2d_projection
        indices = self._resolve_detector_indices(positions, model, masked=True)

        mesh, _, _ = self._assemble_mesh(indices, positions, flatten_2d=flatten_2d)
        return mesh

    # -----------------------------------------------------------------
    # Add to plotter
    # -----------------------------------------------------------------
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

        if not is_projection:
            plotter.enable_trackball_style()
            return

        plotter.view_xy()
        plotter.enable_parallel_projection()
        plotter.enable_zoom_style()

    def add_pickable_mesh_to_plotter(self, plotter: BackgroundPlotter, mesh: pv.PolyData, scalars) -> None:
        if mesh.number_of_points == 0:
            return

        # --- Surface highlight overlay (shows full shape for picked detectors) ---
        if self._detector_mesh_ref is not None and self._detector_mesh_ref.number_of_cells > 0:
            self._highlight_mesh = self._detector_mesh_ref.copy(deep=True)

            # Apply cached visibility if available, otherwise initialize to zero
            if self._cached_visibility is not None and self._cached_visibility_label == scalars and self._cell_to_detector is not None:
                self._highlight_mesh.cell_data[scalars] = self._cached_visibility[self._cell_to_detector]
            else:
                self._highlight_mesh.cell_data[scalars] = np.zeros(self._highlight_mesh.number_of_cells, dtype=np.float64)

            actor = plotter.add_mesh(
                self._highlight_mesh,
                scalars=scalars,
                opacity=[0.0, 0.3],
                clim=[0, 1],
                show_scalar_bar=False,
                pickable=False,
                cmap="Oranges",
                show_edges=False,
            )
            # Polygon offset so highlight renders in front of the detector surface
            mapper = actor.mapper
            mapper.SetResolveCoincidentTopologyToPolygonOffset()
            mapper.SetResolveCoincidentTopologyPolygonOffsetParameters(-1, -1)

        # --- Invisible point cloud used only for picking ---
        plotter.add_mesh(
            mesh,
            scalars=scalars,
            opacity=[0.0, 0.0],
            clim=[0, 1],
            show_scalar_bar=False,
            pickable=True,
            cmap="Oranges",
            point_size=30,
            render_points_as_spheres=True,
        )

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
        """Set up point picking on the pickable overlay.  *callback* receives ``(detector_index: int)``."""
        plotter.disable_picking()

        if plotter.off_screen:
            return

        def _point_picked(point_position, picker):
            if point_position is None:
                return
            point_index = picker.GetPointId()
            callback(point_index)

        plotter.enable_surface_point_picking(
            show_message=False,
            use_picker=True,
            callback=_point_picked,
            show_point=False,
            pickable_window=False,
            picker="point",
            tolerance=0.01,
        )

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
        mesh.point_data[label] = visibility
        # Cache visibility for when highlight mesh is recreated
        self._cached_visibility = visibility
        self._cached_visibility_label = label
        # Propagate per-detector visibility to the surface highlight overlay
        if self._highlight_mesh is not None and self._cell_to_detector is not None and len(visibility) > 0:
            self._highlight_mesh.cell_data[label] = visibility[self._cell_to_detector]

    def transform_internal_meshes(self, transform: np.ndarray) -> None:
        """Apply *transform* to the highlight overlay so it stays aligned
        with the detector surface after the presenter stretches meshes."""
        if self._highlight_mesh is not None:
            self._highlight_mesh.transform(transform, inplace=True)

    def _resolve_detector_indices(self, positions: np.ndarray, model, masked: bool) -> np.ndarray:
        """Return indices into ``self._all_positions_3d`` for the detectors
        represented by *positions*.

        The model stores ``_detector_ids`` for all detectors in the same order
        as the arrays in the ``CreateDetectorTable`` output.  We need to map
        those to the ``detectorInfo`` indices used during precomputation.

        We match by looking up the detector IDs that correspond to the
        pickable/masked subset and converting them via ``detectorInfo.indexOf``.
        """
        if model is None:
            # Fallback: assume positions indexing matches precomputed
            return np.arange(len(positions))

        ws = model.workspace
        det_info = ws.detectorInfo()

        if masked:
            det_ids = model.masked_detector_ids
        else:
            det_ids = model.pickable_detector_ids

        # Convert detector IDs → detectorInfo indices (vectorised via the
        # C++ call).  ``detectorInfo().indexOf(id)`` works element-wise in C++
        # but not in Python, so we loop.  For large instruments this is fast
        # because it's just a dict lookup in C++.
        indices = np.empty(len(det_ids), dtype=np.int64)
        for i, did in enumerate(det_ids):
            indices[i] = det_info.indexOf(int(did))
        return indices

    def _compute_projection_scale(self, det_indices: np.ndarray, projected_positions: np.ndarray) -> float:
        """Compute a uniform scale factor so shapes in 2D projections are
        proportional to the inter-detector spacing.

        Compares median nearest-neighbour distance in 3D (metres) to the
        median nearest-neighbour distance in the projected coordinate space.
        """
        n = len(det_indices)
        if n < 2:
            return 1.0

        # Sample to avoid O(n log n) overhead on very large instruments
        max_sample = 2000
        if n > max_sample:
            rng = np.random.default_rng(42)
            sample = rng.choice(n, max_sample, replace=False)
        else:
            sample = np.arange(n)

        pos_3d = self._all_positions_3d[det_indices[sample]]
        pos_2d = projected_positions[sample, :2]

        tree_3d = cKDTree(pos_3d)
        nnd_3d = tree_3d.query(pos_3d, k=2)[0][:, 1]

        tree_2d = cKDTree(pos_2d)
        nnd_2d = tree_2d.query(pos_2d, k=2)[0][:, 1]

        med_3d = np.median(nnd_3d)
        med_2d = np.median(nnd_2d)

        if med_3d < 1e-12:
            return 1.0

        return med_2d / med_3d

    def _assemble_mesh(
        self,
        det_indices: np.ndarray,
        display_positions: np.ndarray,
        flatten_2d: bool = False,
        per_detector_scales: np.ndarray | None = None,
        per_detector_rotate: np.ndarray | None = None,
    ) -> tuple[pv.PolyData, np.ndarray, np.ndarray]:
        """Vectorised mesh assembly.

        For each unique shape (group of detectors sharing the same template
        mesh), we:

        1. Tile the template vertices for each detector in the group.
        2. Apply per-detector scale, rotation and translation (via
           ``np.einsum``).
        3. Concatenate with face arrays, offsetting vertex indices.
        4. Return a single merged ``pv.PolyData`` plus a mapping from cell
           index to detector-in-group index.

        Parameters
        ----------
        det_indices : np.ndarray
            Indices into the precomputed arrays (``_det_shape_keys``, etc.)
            for the detectors to render.
        display_positions : np.ndarray
            (N, 3) display positions (may be 2D projected).  These are used
            as the centre of each shape.
        flatten_2d : bool
            If True, skip 3D rotations and rescale shapes so they are
            proportional to the projected inter-detector spacing.
        per_detector_scales : np.ndarray or None
            If provided, (N,) per-detector scale factors to use instead of
            a single uniform projection_scale.  Only used when *flatten_2d*
            is True.
        per_detector_rotate : np.ndarray or None
            If provided, (N,) boolean array.  Detectors marked True have
            their 3D rotation applied before flattening (tube banks);
            False detectors stay axis-aligned (grid banks).

        Returns
        -------
        mesh : pv.PolyData
        cell_to_detector : np.ndarray   (total_cells,) → index in 0..N-1
        faces_per_detector : np.ndarray  (N,)
        """
        if len(det_indices) == 0:
            return pv.PolyData(), np.array([], dtype=np.int64), np.array([], dtype=np.int64)

        shape_keys = self._det_shape_keys[det_indices]
        rotations = self._det_rotations[det_indices]  # (N, 3, 3)
        scales = self._det_scales[det_indices]  # (N, 3)

        # In 2D projection mode, compute a scale factor so that shapes
        # are proportional to the projected inter-detector spacing.
        projection_scale = 1.0
        if flatten_2d and per_detector_scales is None and len(det_indices) > 1:
            projection_scale = self._compute_projection_scale(det_indices, display_positions)

        # Use display_positions as centres (they might be projected)
        positions = display_positions

        all_verts_list: list[np.ndarray] = []
        all_faces_list: list[np.ndarray] = []
        cell_to_det_list: list[np.ndarray] = []
        faces_per_det = np.empty(len(det_indices), dtype=np.int64)

        vertex_offset = 0

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

            # --- Vectorised transform: scale → rotate → translate -----------
            # Tile template: (n_group, n_verts, 3)
            tiled = np.tile(template_verts, (n_group, 1, 1))

            # Scale: (n_group, 1, 3) * (n_group, n_verts, 3)
            group_scales = scales[group_indices][:, np.newaxis, :]
            tiled = tiled * group_scales

            if flatten_2d:
                if per_detector_scales is not None:
                    group_rotate = per_detector_rotate[group_indices] if per_detector_rotate is not None else np.zeros(n_group, dtype=bool)
                    if np.all(group_rotate):
                        group_rots = rotations[group_indices]
                        tiled = np.einsum("nij,nvj->nvi", group_rots, tiled)
                    elif np.any(group_rotate):
                        rot_mask = group_rotate
                        group_rots = rotations[group_indices[rot_mask]]
                        tiled[rot_mask] = np.einsum("nij,nvj->nvi", group_rots, tiled[rot_mask])
                    # else: no rotation for this entire shape group
                    group_proj_scales = per_detector_scales[group_indices][:, np.newaxis, np.newaxis]
                    tiled = tiled * group_proj_scales
                else:
                    tiled = tiled * projection_scale
                # Flatten z so shapes lie in the XY plane.
                tiled[:, :, 2] = 0.0
            else:
                # Rotate: R @ v  →  einsum('nij,nvj->nvi', R, V)
                group_rots = rotations[group_indices]  # (n_group, 3, 3)
                tiled = np.einsum("nij,nvj->nvi", group_rots, tiled)

            # Translate
            group_pos = positions[group_indices][:, np.newaxis, :]  # (n_group, 1, 3)
            tiled = tiled + group_pos

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


# =====================================================================
# Module-level helpers
# =====================================================================


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
    s = 0.002  # 2 mm
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
