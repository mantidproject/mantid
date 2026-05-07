# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import pyvista as pv

from pyvistaqt import BackgroundPlotter

from qtpy.QtWidgets import QHBoxLayout, QWidget
from qtpy.QtCore import QTimer

from mantidqt.utils.qt.qappthreadcall import run_on_qapp_thread
from instrumentview.ShapeOverlayManager import ShapeOverlayManager
from instrumentview.ShapeWidgets import RectangleSelectionShape


@run_on_qapp_thread()
class ReflectometryInstrumentViewView(QWidget):
    """A minimal instrument view for ISISReflectometry.

    Contains only a pyvista BackgroundPlotter for 3D instrument rendering.
    The BackgroundPlotter is created lazily via ``initialise()`` to avoid
    OpenGL context errors when VTK tries to render before the widget is
    embedded in its final layout.
    """

    def __init__(self, parent=None):
        super().__init__(parent)

        pv.global_theme.background = "black"
        pv.global_theme.font.color = "white"

        self._initialised = False
        self.main_plotter = None
        self._shape_overlay_manager: ShapeOverlayManager | None = None
        self._on_resize_callback = None

        self._resize_timer = QTimer(self)
        self._resize_timer.setSingleShot(True)
        self._resize_timer.setInterval(150)
        self._resize_timer.timeout.connect(self._on_resize_finished)

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)

    def initialise(self):
        """Create the BackgroundPlotter.

        Must be called after this widget is embedded in its final layout.
        """
        if self._initialised:
            return
        self._initialised = True

        self.main_plotter = BackgroundPlotter(show=False, menu_bar=False, toolbar=False, off_screen=False)
        self.layout().addWidget(self.main_plotter.app_window)

    def set_on_resize_callback(self, callback) -> None:
        """Set a callable invoked (deferred) on every resize, or None to clear."""
        self._on_resize_callback = callback

    def resizeEvent(self, event):
        """Start (or restart) the debounce timer on every resize event.

        During a drag-resize Qt fires many resize events in rapid succession.
        Restarting the timer each time means the actual fill-transform callback
        only fires once, 150 ms after the *last* resize event — i.e. when the
        user has finished resizing.  This prevents the mesh from flashing
        between its original aspect ratio and the filled state on every pixel
        of movement.
        """
        super().resizeEvent(event)
        if self.main_plotter is None:
            return
        self._resize_timer.start()

    def _on_resize_finished(self):
        """Invoke the resize callback once resizing has stopped.

        Called by the debounce timer.  By this point VTK has already applied
        the final render-window dimensions (via QVTKRenderWindowInteractor.
        resizeEvent inside super().resizeEvent()), so the fill transform can
        read the correct size from plotter.ren_win.GetSize().
        """
        if self.main_plotter is None:
            return
        callback = self._on_resize_callback if self._on_resize_callback is not None else self.main_plotter.reset_camera
        callback()

    @property
    def shape_overlay_manager(self) -> ShapeOverlayManager | None:
        """The active ShapeOverlayManager, or None if no shape is shown."""
        return self._shape_overlay_manager

    def overlay_rectangle(self, on_shape_changed=None) -> None:
        if self.main_plotter is None:
            return
        if self._shape_overlay_manager is None:
            self._shape_overlay_manager = ShapeOverlayManager(self.main_plotter)
        if on_shape_changed is not None:
            self._shape_overlay_manager.set_on_shape_changed(on_shape_changed)
        shape = RectangleSelectionShape(cx=0.5, cy=0.5, half_w=1.0 / 6.0, half_h=1.0 / 6.0)
        self._shape_overlay_manager.add_shape(shape)

    def remove_shape(self) -> None:
        """Remove the current selection shape overlay."""
        if self._shape_overlay_manager is not None:
            self._shape_overlay_manager.remove_shape()
            self._shape_overlay_manager = None

    def closeEvent(self, event):
        self.remove_shape()
        super().closeEvent(event)
        if self.main_plotter is not None:
            self.main_plotter.close()
