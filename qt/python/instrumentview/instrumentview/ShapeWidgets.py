# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Shape widgets for instrument view selection tools.

This module provides custom VTK widgets for interactive shape selection:
- CylinderWidgetNoRotation: Cylinder selection without rotation
- RectangleWidgetNoRotation: Rectangle selection without rotation
- ImplicitEllipse: Custom implicit function for rotated ellipse
- EllipseWidgetNoRotation: Ellipse selection with rotation handle
"""

from mantid.kernel import logger

import numpy as np
import pyvista as pv
from vtkmodules.vtkCommonCore import vtkCommand
from vtkmodules.vtkInteractionWidgets import vtkImplicitCylinderWidget, vtkBoxWidget2
from vtkmodules.vtkRenderingCore import vtkPropPicker


class CylinderWidgetNoRotation(vtkImplicitCylinderWidget):
    def __init__(self):
        super().__init__()
        self.AddObserver(vtkCommand.StartInteractionEvent, lambda *_: self._on_interaction())

    def _on_interaction(self):
        # Replace rotation state (integer 4) with translation state (integer 3)
        if self.GetCylinderRepresentation().GetInteractionState() == 4:
            self.GetCylinderRepresentation().SetInteractionState(3)
            return


class RectangleWidgetNoRotation(vtkBoxWidget2):
    def __init__(self):
        super().__init__()
        self.AddObserver(vtkCommand.StartInteractionEvent, lambda *_: self._on_interaction())

    def _on_interaction(self):
        # Replace rotation state (integer 8) with translation state (integer 7)
        if self.GetRepresentation().GetInteractionState() == 8:
            self.GetRepresentation().SetInteractionState(7)
            return


class ImplicitEllipse:
    """Custom implicit function for a rotated ellipse in 2D.
    Supports center, semi-axes and rotation angle (radians). The implicit
    function evaluates using the rotated coordinates so rotations are
    correctly handled for point-in-ellipse tests.
    """

    def __init__(self):
        self.cx = 0.0
        self.cy = 0.0
        self.rx = 1.0
        self.ry = 1.0
        self.angle = 0.0  # Rotation angle in radians (counter-clockwise)

    def EvaluateFunction(self, x):
        """Evaluate the implicit function. Returns negative inside, positive outside."""
        px, py = x[0], x[1]

        dx = px - self.cx
        dy = py - self.cy
        # Rotate point by -angle to bring ellipse to axis-aligned coordinates
        ca = np.cos(-self.angle)
        sa = np.sin(-self.angle)
        xr = dx * ca - dy * sa
        yr = dx * sa + dy * ca

        nx = xr / self.rx if self.rx > 0 else 0
        ny = yr / self.ry if self.ry > 0 else 0
        return nx * nx + ny * ny - 1.0

    def EvaluateGradient(self, x, gradient):
        if isinstance(x, (list, tuple)):
            px, py = x[0], x[1]
        else:
            px, py = x[0], x[1]

        dx = px - self.cx
        dy = py - self.cy
        ca = np.cos(-self.angle)
        sa = np.sin(-self.angle)
        xr = dx * ca - dy * sa
        yr = dx * sa + dy * ca

        if self.rx > 0 and self.ry > 0:
            gx = 2.0 * xr / (self.rx**2)
            gy = 2.0 * yr / (self.ry**2)
            ca_f = np.cos(self.angle)
            sa_f = np.sin(self.angle)
            gradient[0] = gx * ca_f - gy * sa_f
            gradient[1] = gx * sa_f + gy * ca_f
            gradient[2] = 0.0
        else:
            gradient[0] = gradient[1] = gradient[2] = 0.0

    def set_center(self, cx, cy):
        self.cx = cx
        self.cy = cy

    def set_radii(self, rx, ry):
        self.rx = max(rx, 0.001)
        self.ry = max(ry, 0.001)

    def set_angle(self, angle_radians: float):
        self.angle = float(angle_radians or 0.0)


class EllipseWidgetNoRotation(vtkBoxWidget2):
    """Box-based widget that supports a rotated ellipse visual and implicit test.

    This widget still uses a box representation for translation and scaling of
    the ellipse, but it also provides a rotation handle that the user can drag
    to rotate the ellipse in the 2D projection. Rotation is computed using
    display-space angles which makes the interaction intuitive in the 2D
    projection view.
    """

    def __init__(self, plotter=None):
        super().__init__()
        self._ellipse = ImplicitEllipse()
        self._plotter = plotter
        self._ellipse_actor = None
        self._ellipse_mesh = None
        self._rotation_handle_actor = None
        self._rotation_handle_mesh = None
        self._angle = 0.0
        self._rotating = False
        self._center_display = None
        self._handle_display = None

        self._vtk_interactor = None
        self._saved_interactor_style = None  # stashed while rotating
        self._rotation_observers_installed = False
        self._handle_picker = vtkPropPicker()
        try:
            # Prefer only picking from an explicit list (the handle actor).
            self._handle_picker.PickFromListOn()
        except Exception:
            logger.debug("Failed to configure VTK picker - rotation handle picking may be unreliable.")

        self._press_id = None
        self._move_id = None
        self._release_id = None
        self._end_interaction_id = None
        self._leave_id = None
        self._exit_id = None
        self._render_id = None
        self._event_filter = None
        self._last_button_down = False

        # Keep VTK observers for widget interaction to update parameters
        self.AddObserver(vtkCommand.StartInteractionEvent, lambda *_: self._on_interaction())
        self.AddObserver(vtkCommand.InteractionEvent, lambda *_: self._update_ellipse_from_bounds())
        self.AddObserver(vtkCommand.EndInteractionEvent, lambda *_: self._update_ellipse_from_bounds())

    def SetInteractor(self, interactor):
        """Override to hook rotation observers onto the real VTK interactor.

        Mouse events are invoked on the `vtkRenderWindowInteractor`, not on the widget
        instance itself, so we install observers here.
        """
        super().SetInteractor(interactor)
        self._vtk_interactor = interactor
        self._install_rotation_observers_if_needed()

    def _install_rotation_observers_if_needed(self):
        if self._rotation_observers_installed:
            return
        if self._vtk_interactor is None:
            return

        # Use a high priority so we can abort event propagation when rotating.
        self._press_id = self._vtk_interactor.AddObserver(vtkCommand.LeftButtonPressEvent, self._on_vtk_button_press, 1.0)
        self._move_id = self._vtk_interactor.AddObserver(vtkCommand.MouseMoveEvent, self._on_vtk_mouse_move, 1.0)
        self._release_id = self._vtk_interactor.AddObserver(vtkCommand.LeftButtonReleaseEvent, self._on_vtk_button_release, 1.0)
        # Extra safety: if the release happens outside the render window, we may not
        # receive LeftButtonReleaseEvent. These events let us reliably cancel.
        self._end_interaction_id = self._vtk_interactor.AddObserver(vtkCommand.EndInteractionEvent, self._on_vtk_end_interaction, 1.0)
        self._leave_id = self._vtk_interactor.AddObserver(vtkCommand.LeaveEvent, self._on_vtk_cancel_rotation, 1.0)
        self._exit_id = self._vtk_interactor.AddObserver(vtkCommand.ExitEvent, self._on_vtk_cancel_rotation, 1.0)
        self._rotation_observers_installed = True

    def _cancel_rotation(self, interactor=None):
        if not self._rotating:
            return
        self._rotating = False
        # Restore the interactor style that was disabled when rotation started.
        self._restore_interactor_style(interactor)

    def _disable_interactor_style(self, interactor):
        """Remove the interactor style so it doesn't draw a rubber-band zoom box,
        and disable the box widget's own event processing so it doesn't move/resize."""
        if interactor is None:
            return
        try:
            style = interactor.GetInteractorStyle()
            if style is not None:
                self._saved_interactor_style = style
                interactor.SetInteractorStyle(None)
        except Exception:
            logger.debug("Failed to disable VTK interactor style - rotation may also pan/zoom and have visual artifacts.")
        try:
            self.SetProcessEvents(0)
        except Exception:
            logger.debug("Failed to disable box widget event processing - box may also move/resize during rotation.")

    def _restore_interactor_style(self, interactor):
        """Put the interactor style and box widget event processing back after rotation ends."""
        try:
            self.SetProcessEvents(1)
        except Exception:
            logger.debug("Failed to re-enable box widget event processing after rotation.")
        if self._saved_interactor_style is None:
            return
        try:
            if interactor is not None:
                interactor.SetInteractorStyle(self._saved_interactor_style)
        except Exception:
            logger.debug("Failed to restore VTK interactor style after rotation - interactor may be left without a style.")
        self._saved_interactor_style = None

    def _on_vtk_cancel_rotation(self, obj, event):
        interactor = self.GetInteractor()
        self._cancel_rotation(interactor)

    def _on_vtk_end_interaction(self, obj, event):
        # EndInteractionEvent can occur when mouse is released outside the window.
        interactor = self.GetInteractor()
        self._cancel_rotation(interactor)

    def _on_interaction(self):
        # Replace rotation state (integer 8) with translation state (integer 7)
        if self.GetRepresentation().GetInteractionState() == 8:
            self.GetRepresentation().SetInteractionState(7)
            return

    def _on_vtk_button_press(self, obj, event):
        interactor = obj if hasattr(obj, "GetEventPosition") else self.GetInteractor()
        if interactor is None or self._handle_display is None:
            return

        pos = interactor.GetEventPosition()

        # First try an actual VTK pick against the handle actor.
        # This is much more reliable than a small fixed pixel distance.
        try:
            renderer = None
            if self._plotter is not None:
                renderer = getattr(self._plotter, "renderer", None)
            if renderer is None:
                renderer = self.GetCurrentRenderer()

            if renderer is not None and self._rotation_handle_actor is not None:
                self._handle_picker.Pick(pos[0], pos[1], 0.0, renderer)
                picked_actor = self._handle_picker.GetActor()
                # NOTE: VTK often returns a new Python wrapper for the same underlying
                # C++ actor, so use `==` rather than `is`.
                if picked_actor is not None and picked_actor == self._rotation_handle_actor:
                    self._rotating = True
                    self._disable_interactor_style(interactor)
                    return
        except Exception:
            logger.debug("VTK picking of rotation handle failed - falling back to distance-based hit test, which may be unreliable.")

        dx = pos[0] - self._handle_display[0]
        dy = pos[1] - self._handle_display[1]
        dist_sq = dx * dx + dy * dy

        # Fallback: allow a generous grab radius, scaled for HiDPI.
        grab_px = 20.0
        try:
            if self._plotter is not None and hasattr(self._plotter, "app_window"):
                grab_px *= float(self._plotter.app_window.devicePixelRatioF())
        except Exception:
            logger.debug("Failed to scale grab radius for HiDPI - grab area may be too small on high-resolution displays.")

        if dist_sq <= grab_px * grab_px:
            self._rotating = True
            self._disable_interactor_style(interactor)
            # Consume the event so the box widget/camera doesn't also handle it.

    def _on_vtk_mouse_move(self, obj, event):
        """Handle VTK MouseMoveEvent."""
        try:
            if not self._rotating or self._center_display is None:
                return

            interactor = obj if hasattr(obj, "GetEventPosition") else self.GetInteractor()
            if interactor is None:
                return

            # Hard guard: if left button is no longer down, stop rotating.
            # (This covers missed LeftButtonRelease events.)
            try:
                if hasattr(interactor, "GetLeftButton") and not interactor.GetLeftButton():
                    self._cancel_rotation(interactor)
                    return
            except Exception:
                logger.debug(
                    "Failed to check left button state during rotation - relying on VTK events to end rotation, "
                    "which may be unreliable if events are missed."
                )

            # If the interactor became disabled mid-drag, stop rotating.
            try:
                if hasattr(interactor, "GetEnabled") and not interactor.GetEnabled():
                    self._cancel_rotation(interactor)
                    return
            except Exception:
                logger.debug(
                    "Failed to check interactor enabled state during rotation - relying on VTK events to end rotation, "
                    "which may be unreliable if events are missed."
                )

            pos = interactor.GetEventPosition()
            cx_disp, cy_disp = self._center_display
            ang = np.arctan2(pos[1] - cy_disp, pos[0] - cx_disp)
            self._angle = ang
            self._ellipse.set_angle(self._angle)
            self._update_ellipse_from_bounds()
        except Exception:
            logger.debug("Exception in rotation mouse move handler - rotation may be inconsistent.", exc_info=True)

    def _on_vtk_button_release(self, obj, event):
        try:
            interactor = self.GetInteractor()
            self._cancel_rotation(interactor)
        except Exception:
            logger.debug("Failed to handle button release event during rotation - interaction may be inconsistent.", exc_info=True)

    def _update_ellipse_from_bounds(self):
        """Update ellipse parameters from the widget's current bounds and refresh visuals."""
        try:
            bounds = self.GetRepresentation().GetBounds()
            xmin, xmax, ymin, ymax, zmin, zmax = bounds

            cx = (xmin + xmax) / 2.0
            cy = (ymin + ymax) / 2.0
            rx = max((xmax - xmin) / 2.0, 0.001)
            ry = max((ymax - ymin) / 2.0, 0.001)
            z = (zmin + zmax) / 2.0

            self._ellipse.set_center(cx, cy)
            self._ellipse.set_radii(rx, ry)
            self._ellipse.set_angle(self._angle)

            if self._plotter is not None:
                self._update_visual_ellipse(cx, cy, rx, ry, z)
        except Exception:
            logger.debug("Failed to update ellipse from widget bounds - visuals may be out of sync with parameters.", exc_info=True)

    def _create_ellipse_mesh(self, cx, cy, rx, ry, z, num_points=64, angle=0.0):
        theta = np.linspace(0, 2 * np.pi, num_points)
        x = rx * np.cos(theta)
        y = ry * np.sin(theta)
        ca = np.cos(angle)
        sa = np.sin(angle)
        xr = x * ca - y * sa + cx
        yr = x * sa + y * ca + cy
        z_arr = np.full_like(xr, z)

        points = np.column_stack([xr, yr, z_arr])
        lines = np.hstack(([num_points], np.arange(num_points, dtype=np.int32))).astype(np.int32)
        mesh = pv.PolyData(points)
        mesh.lines = lines
        return mesh

    def _update_visual_ellipse(self, cx, cy, rx, ry, z):
        """Create or update the ellipse polyline and the rotation handle."""
        try:
            if self._ellipse_mesh is None:
                self._ellipse_mesh = self._create_ellipse_mesh(cx, cy, rx, ry, z, angle=self._angle)
                self._ellipse_actor = self._plotter.add_mesh(
                    self._ellipse_mesh,
                    color="yellow",
                    line_width=3,
                    render_lines_as_tubes=False,
                    pickable=False,
                    show_edges=False,
                    name=f"ellipse_widget_{id(self)}",
                )
            else:
                num_points = int(self._ellipse_mesh.n_points)
                theta = np.linspace(0, 2 * np.pi, num_points)
                x = rx * np.cos(theta)
                y = ry * np.sin(theta)
                ca = np.cos(self._angle)
                sa = np.sin(self._angle)
                xr = x * ca - y * sa + cx
                yr = x * sa + y * ca + cy
                z_arr = np.full_like(xr, z)
                new_points = np.column_stack([xr, yr, z_arr])
                self._ellipse_mesh.points = new_points
                try:
                    self._plotter.update_coordinates(self._ellipse_mesh.points, mesh=self._ellipse_mesh, render=True)
                except Exception:
                    try:
                        self._plotter.render()
                    except Exception:
                        logger.debug(
                            "Failed to update plotter coordinates or render after ellipse update - "
                            "visuals may not reflect current parameters."
                        )

            # Rotation handle: place a small sphere at angle 0 on the ellipse (major-axis positive x direction)
            handle_radius = min(rx, ry) * 0.08
            hx = cx + rx * np.cos(self._angle)
            hy = cy + rx * np.sin(self._angle)
            hz = z

            if self._rotation_handle_mesh is None:
                sph = pv.Sphere(radius=handle_radius, center=(hx, hy, hz), theta_resolution=12, phi_resolution=12)
                self._rotation_handle_mesh = sph
                self._rotation_handle_actor = self._plotter.add_mesh(
                    sph,
                    color="orange",
                    smooth_shading=True,
                    pickable=True,
                    name=f"ellipse_handle_{id(self)}",
                )
                try:
                    # Ensure VTK picker can hit it reliably.
                    self._rotation_handle_actor.SetPickable(True)
                except Exception:
                    logger.debug("Failed to set rotation handle actor pickable - picking may be unreliable.")
                try:
                    # Restrict picking to just this actor.
                    self._handle_picker.InitializePickList()
                    self._handle_picker.AddPickList(self._rotation_handle_actor)
                except Exception:
                    logger.debug("Failed to configure VTK picker for rotation handle - picking may be unreliable.")
            else:
                try:
                    num_points = int(self._rotation_handle_mesh.n_points)
                    # Recreate a small sphere at the handle location for simplicity
                    sph = pv.Sphere(radius=handle_radius, center=(hx, hy, hz), theta_resolution=12, phi_resolution=12)
                    self._rotation_handle_mesh.points = sph.points
                    try:
                        self._plotter.update_coordinates(self._rotation_handle_mesh.points, mesh=self._rotation_handle_mesh, render=True)
                    except Exception:
                        try:
                            self._plotter.render()
                        except Exception:
                            logger.debug(
                                "Failed to update plotter coordinates or render after rotation handle update - "
                                "visuals may not reflect current parameters."
                            )
                except Exception:
                    logger.debug("Failed to update rotation handle position - handle may not reflect current ellipse angle.")

            # Store display positions for quick hit-testing
            try:
                renderer = self._plotter.renderer
                # Center display
                renderer.SetWorldPoint(cx, cy, z, 1.0)
                renderer.WorldToDisplay()
                cdisp = renderer.GetDisplayPoint()
                # Handle display
                renderer.SetWorldPoint(hx, hy, hz, 1.0)
                renderer.WorldToDisplay()
                hdisp = renderer.GetDisplayPoint()
                self._center_display = (cdisp[0], cdisp[1])
                self._handle_display = (hdisp[0], hdisp[1])
            except Exception:
                self._center_display = None
                self._handle_display = None
        except Exception:
            logger.debug("Failed to update visual ellipse - visuals may not reflect current parameters.", exc_info=True)
            return

    def set_ellipse_parameters(self, cx, cy, rx, ry, angle=None):
        self._ellipse.set_center(cx, cy)
        self._ellipse.set_radii(rx, ry)
        if angle is not None:
            self._angle = angle
            self._ellipse.set_angle(angle)

    def get_implicit_ellipse(self):
        self._update_ellipse_from_bounds()
        return self._ellipse

    def cleanup(self):
        # Remove VTK interactor observers (rotation)
        try:
            if self._vtk_interactor is not None:
                for oid in (
                    self._press_id,
                    self._move_id,
                    self._release_id,
                    self._end_interaction_id,
                    self._leave_id,
                    self._exit_id,
                ):
                    if oid is not None:
                        self._vtk_interactor.RemoveObserver(oid)
        except Exception:
            logger.debug(
                "Failed to remove VTK interactor observers during cleanup - may cause issues if widget is deleted but interactor persists."
            )

        if self._ellipse_actor is not None:
            try:
                self._plotter.remove_actor(self._ellipse_actor)
            except Exception:
                logger.debug(
                    "Failed to remove ellipse actor during cleanup - may cause visual artifacts if widget is deleted but plotter persists."
                )
            self._ellipse_actor = None
        if self._rotation_handle_actor is not None:
            try:
                self._plotter.remove_actor(self._rotation_handle_actor)
            except Exception:
                logger.debug(
                    "Failed to remove rotation handle actor during cleanup - may cause visual artifacts if "
                    "widget is deleted but plotter persists."
                )
            self._rotation_handle_actor = None
        # Remove render observer and event filter
        try:
            if getattr(self, "_render_id", None) is not None and self._vtk_interactor is not None:
                try:
                    rw = self._vtk_interactor.GetRenderWindow()
                    if rw is not None:
                        rw.RemoveObserver(self._render_id)
                except Exception:
                    logger.debug(
                        "Failed to remove render observer during cleanup - may cause issues if widget is "
                        "deleted but render window persists."
                    )

            # Remove Qt event filter if it was installed
            if getattr(self, "_event_filter", None) is not None and hasattr(self._plotter, "app_window"):
                widget = self._plotter.app_window
                try:
                    widget.removeEventFilter(self._event_filter)
                    self._event_filter = None
                except Exception:
                    logger.debug(
                        "Failed to remove Qt event filter during cleanup - may cause issues if widget is "
                        "deleted but application window persists."
                    )
        except Exception:
            logger.debug(
                "Exception during EllipseWidgetNoRotation cleanup - may cause issues if widget is deleted but VTK/Qt objects persist.",
                exc_info=True,
            )
        return self._ellipse
