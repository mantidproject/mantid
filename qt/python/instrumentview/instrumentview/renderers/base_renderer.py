# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABC, abstractmethod
from typing import Callable, Optional

import numpy as np
import pyvista as pv
from pyvistaqt import BackgroundPlotter


class InstrumentRenderer(ABC):
    """Abstract base class defining the interface for rendering detectors in the instrument view.

    Concrete implementations handle either point-cloud rendering (fast, approximate)
    or shape-based rendering (slower, geometrically accurate).
    """

    @abstractmethod
    def build_detector_mesh(self, positions: np.ndarray, model) -> pv.PolyData:
        """Build the visual mesh for unmasked detectors.

        Parameters
        ----------
        positions : np.ndarray
            (N, 3) detector centre positions (already projected if 2D).
        model :
            The FullInstrumentViewModel, used to access workspace/shape data.

        Returns
        -------
        pv.PolyData
            The mesh to be displayed.
        """

    @abstractmethod
    def build_pickable_mesh(self, positions: np.ndarray, model) -> pv.PolyData:
        """Build the mesh used for interactive picking / selection highlighting.

        Parameters
        ----------
        positions : np.ndarray
            (N, 3) detector centre positions.
        model :
            The FullInstrumentViewModel.

        Returns
        -------
        pv.PolyData
            The pickable mesh.
        """

    @abstractmethod
    def build_masked_mesh(self, positions: np.ndarray, model) -> pv.PolyData:
        """Build the mesh for masked detectors.

        Parameters
        ----------
        positions : np.ndarray
            (N, 3) masked detector centre positions.
        model :
            The FullInstrumentViewModel.

        Returns
        -------
        pv.PolyData
            The masked mesh.
        """

    @abstractmethod
    def add_detector_mesh_to_plotter(
        self, plotter: BackgroundPlotter, mesh: pv.PolyData, is_projection: bool, scalars: Optional[str] = None
    ) -> None:
        """Add the detector mesh to the plotter with appropriate visual settings."""

    @abstractmethod
    def add_pickable_mesh_to_plotter(self, plotter: BackgroundPlotter, mesh: pv.PolyData, scalars) -> None:
        """Add the pickable overlay mesh to the plotter."""

    @abstractmethod
    def add_masked_mesh_to_plotter(self, plotter: BackgroundPlotter, mesh: pv.PolyData) -> None:
        """Add the masked detector mesh to the plotter."""

    @abstractmethod
    def enable_picking(self, plotter: BackgroundPlotter, is_2d: bool, callback: Callable[[int], None]) -> None:
        """Set up picking interaction on the plotter.

        Parameters
        ----------
        plotter : BackgroundPlotter
            The PyVista plotter.
        is_2d : bool
            Whether the current view is a 2D projection.
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

    def transform_internal_meshes(self, transform: np.ndarray) -> None:
        """Apply a 4×4 affine transform to any renderer-internal meshes.

        Override in subclasses that maintain additional meshes (e.g. the
        surface highlight overlay in ShapeRenderer).  The default is a no-op.
        """
