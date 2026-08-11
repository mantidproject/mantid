# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABC, abstractmethod
from typing import Callable, Optional, ClassVar

import numpy as np
import pyvista as pv
from pyvistaqt import BackgroundPlotter


class InstrumentRenderer(ABC):
    """Abstract base class defining the interface for rendering detectors in the instrument view.

    Concrete implementations handle either point-cloud rendering (fast, approximate)
    or shape-based rendering (slower, geometrically accurate).
    """

    # Marker colour for picked detectors.  Deliberately distinct from the monitor
    # colour (red) and from the counts colour map, so a selection can never be
    # mistaken for either.
    _PICKED_HIGHLIGHT_COLOUR: ClassVar[str] = "magenta"

    # Opacity transfer function for the pickable overlay, as [unpicked, picked].
    # The picked entry is fully transparent: the magenta marker drawn by
    # update_picked_highlight marks the selection instead, and a tinted fill on
    # top of it only muddied the colour and hid the detector's counts.  The
    # overlay actor itself is still needed — it is the pick target.
    _PICKED_FILL_OPACITY: ClassVar[list[float]] = [0.0, 0.0]

    def __init__(self) -> None:
        super().__init__()
        self._mouse_move_observer_id = None
        self._left_button_observer_id = None
        self._picking_tolerance: float = 0.01
        self._picked_highlight_mesh: Optional[pv.PolyData] = None
        self._picked_highlight_actor = None

    def _clear_observers(self, plotter):
        style = plotter.iren.style
        if self._mouse_move_observer_id is not None:
            style.RemoveObserver(self._mouse_move_observer_id)
        if self._left_button_observer_id is not None:
            style.RemoveObserver(self._left_button_observer_id)
        self._mouse_move_observer_id = None
        self._left_button_observer_id = None

    @abstractmethod
    def build_detector_mesh(self, positions: np.ndarray, flip_beam: bool, model) -> pv.PolyData:
        """Build the visual mesh for unmasked detectors.

        Parameters
        ----------
        positions : np.ndarray
            (N, 3) detector centre positions (already projected if 2D).
        flip_beam : bool
            If True, mirror the projection along the beam axis, i.e. reflect the projection in the plane with the beam axis as its normal.
        model :
            The FullInstrumentViewModel, used to access workspace/shape data.

        Returns
        -------
        pv.PolyData
            The mesh to be displayed.
        """

    @abstractmethod
    def build_pickable_mesh(self, positions: np.ndarray, flip_beam: bool) -> pv.PolyData:
        """Build the mesh used for interactive picking / selection highlighting.

        Parameters
        ----------
        positions : np.ndarray
            (N, 3) detector centre positions.
        flip_beam : bool
            If True, mirror the projection along the beam axis, i.e. reflect the projection in the plane with the beam axis as its normal.

        Returns
        -------
        pv.PolyData
            The pickable mesh.
        """

    @abstractmethod
    def build_masked_mesh(self, positions: np.ndarray, flip_beam: bool, model) -> pv.PolyData:
        """Build the mesh for masked detectors.

        Parameters
        ----------
        positions : np.ndarray
            (N, 3) masked detector centre positions.
        flip_beam : bool
            If True, mirror the projection along the beam axis, i.e. reflect the projection in the plane with the beam axis as its normal.
        model :
            The FullInstrumentViewModel.

        Returns
        -------
        pv.PolyData
            The masked mesh.
        """

    @abstractmethod
    def add_detector_mesh_to_plotter(
        self, plotter: BackgroundPlotter, mesh: pv.PolyData, scalars: Optional[str] = None, show_scalar_bar: bool = True
    ) -> None:
        """Add the detector mesh to the plotter with appropriate visual settings."""

    @abstractmethod
    def add_pickable_mesh_to_plotter(self, plotter: BackgroundPlotter, mesh: pv.PolyData, scalars) -> None:
        """Add the pickable overlay mesh to the plotter."""

    @abstractmethod
    def add_masked_mesh_to_plotter(self, plotter: BackgroundPlotter, mesh: pv.PolyData) -> None:
        """Add the masked detector mesh to the plotter."""

    @abstractmethod
    def get_callback_tied_to_detector_index(
        self, plotter: BackgroundPlotter, callback: Callable[[int], None], hover: bool = False
    ) -> Callable:
        """Set up picking interaction on the plotter.

        Parameters
        ----------
        plotter : BackgroundPlotter
            The PyVista plotter.
        callback : Callable
            Function to call when a detector is picked; receives (detector_index: int).
        """

    @abstractmethod
    def set_detector_scalars(self, mesh: pv.PolyData, counts: np.ndarray, label: str) -> None:
        """Update the scalar data (e.g. integrated counts) on the detector mesh.

        Parameters
        ----------
        mesh : pv.PolyData
            The detector mesh.
        counts : np.ndarray
            Per-detector count values.
        label : str
            Scalar array name.
        """

    @abstractmethod
    def set_pickable_scalars(self, mesh: pv.PolyData, visibility: np.ndarray, label: str) -> None:
        """Update the visibility/pick scalars on the pickable mesh.

        Parameters
        ----------
        mesh : pv.PolyData
            The pickable mesh.
        visibility : np.ndarray
            Per-detector visibility flags.
        label : str
            Scalar array name.
        """

    # ------------------------------------------------------- picked highlight
    def create_picked_highlight_actor(self, plotter: BackgroundPlotter) -> None:
        """Create the (initially hidden) actor that marks picked detectors.

        Called once per full plotter rebuild, alongside every other actor.  The
        actor is created up front and then reused, rather than being added and
        removed as the selection changes: removing an actor makes VTK release
        its graphics resources, which needs the OpenGL context to be current.
        Selection changes can arrive on the presenter's callback worker thread,
        where grabbing the context fails with ``wglMakeCurrent`` errors because
        the Qt thread already holds it.  Updating this actor's data and
        visibility instead touches no graphics resources at all.
        """
        # A placeholder is needed because PyVista refuses to add an empty mesh.
        # The actor stays hidden until there is a real selection to show.
        self._picked_highlight_mesh = pv.PolyData(np.zeros((1, 3)))
        self._picked_highlight_actor = self._add_picked_highlight_actor(plotter, self._picked_highlight_mesh)
        if self._picked_highlight_actor is not None:
            self._picked_highlight_actor.SetVisibility(False)

    def update_picked_highlight(self, plotter: BackgroundPlotter, mesh: Optional[pv.PolyData], visibility: np.ndarray) -> None:
        """Update the high-visibility marker to match the current selection.

        The translucent fill driven by ``set_pickable_scalars`` scales with the
        detector's size on screen, so it disappears when zoomed out on a large
        instrument.  This marker is drawn in screen-space units instead (line
        width / point size), so it stays legible at any zoom level.

        Safe to call from any thread — see ``create_picked_highlight_actor``.

        Parameters
        ----------
        plotter : BackgroundPlotter
            The PyVista plotter holding the current actors.
        mesh : pv.PolyData or None
            The pickable mesh, already transformed into display coordinates.
        visibility : np.ndarray
            Per-pickable-detector flags; non-zero entries are picked.
        """
        if self._picked_highlight_actor is None or self._picked_highlight_mesh is None:
            return

        highlight = None
        if mesh is not None and visibility is not None:
            visibility = np.asarray(visibility)
            if visibility.size > 0 and np.any(visibility):
                highlight = self._build_picked_highlight_mesh(mesh, visibility)

        if highlight is None or highlight.number_of_points == 0:
            self._picked_highlight_actor.SetVisibility(False)
        else:
            self._picked_highlight_mesh.copy_from(highlight)
            self._picked_highlight_actor.SetVisibility(True)
        plotter.render()

    def _add_picked_highlight_actor(self, plotter: BackgroundPlotter, mesh: pv.PolyData):
        """Add the highlight actor for *mesh* to *plotter* and return it.

        The base implementation draws nothing; subclasses provide a marker
        appropriate to how they render detectors.
        """
        return None

    def _build_picked_highlight_mesh(self, mesh: pv.PolyData, visibility: np.ndarray) -> Optional[pv.PolyData]:
        """Return the marker geometry for the picked detectors, or None if there is nothing to draw."""
        return None

    def _effective_picking_tolerance(self, hover: bool) -> float:
        """Return the tolerance to pass to the VTK picker.

        Hover picking uses a 25 % larger tolerance so that moving the mouse
        over nearby detectors feels responsive without sacrificing click
        precision.
        """
        return self._picking_tolerance * 1.25 if hover else self._picking_tolerance
