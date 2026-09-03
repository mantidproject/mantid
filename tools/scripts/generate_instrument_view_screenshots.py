# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Generate the screenshots used by the Instrument View documentation.

The user documentation at ``docs/source/workbench/instrumentviewer.rst`` needs a screenshot of
each part of the Instrument View. Taking them by hand is slow and they have to be retaken every
time the layout changes, so this script drives the window from a script and writes the images
straight into ``docs/source/images/Workbench/InstrumentViewer``.

Run it with a file, a run number or the name of a workspace already in the ADS::

    python generate_instrument_view_screenshots.py SXD23767.raw
    python generate_instrument_view_screenshots.py SXD23767
    python generate_instrument_view_screenshots.py SXD23767 --only LinePlot PeaksLinePlot

A run number is resolved through the usual data search path, so it will be fetched from the
archive if it is not held locally. Pointing at a local copy of the file avoids that.

``--list`` prints the shots that will be taken. ``ContextMenu.png`` is not among them: it shows
the Workbench workspace context menu, so it has to be taken by hand from a running Workbench.

The script needs a real display, because the 3D view is rendered by VTK. The window appears
while it runs and closes itself at the end.

There is no public API for driving the individual widgets, so this reaches into the view and the
presenter. If a screenshot comes out empty, check the private names used here still exist.
"""

import argparse
import sys
import time
from pathlib import Path

import numpy as np

# Screenshots are only useful if they match the documentation, so the names here are the file
# names the .rst refers to. Order matters: each shot leaves the window in a state the next one
# builds on, and re-running with --only reproduces the same state from scratch.
_SHOT_ORDER = [
    "Overview",
    "HomeTab",
    "ProjectionOptions",
    "Projection3D",
    "ProjectionSideBySide",
    "UnitsAndContourRange",
    "PickingInteraction",
    "DetectorInfo",
    "LinePlot",
    "PeaksOverlay",
    "PeaksLinePlot",
    "PeakAddDelete",
    "ShapeOverlay",
    "GroupingTab",
    "MaskingTab",
    "SettingsTab",
    "ComponentTree",
]

_SHOTS_NEEDING_PEAKS = {"PeaksOverlay", "PeaksLinePlot", "PeakAddDelete"}

_DEFAULT_OUTPUT_DIR = Path(__file__).resolve().parents[2] / "docs" / "source" / "images" / "Workbench" / "InstrumentViewer"

# Written to by the view whenever the render mode or a checkbox changes, so they are put back
# afterwards rather than leaving the user's settings altered by a documentation build.
_CONFIG_KEYS = ("InstrumentView.MaintainAspectRatio", "InstrumentView.RenderMode", "InstrumentView.FlipBeam")


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", nargs="?", help="File path, run number, or the name of a workspace already in the ADS.")
    parser.add_argument("-o", "--output-dir", type=Path, default=_DEFAULT_OUTPUT_DIR, help="Directory to write the PNGs into.")
    parser.add_argument("--only", nargs="+", metavar="NAME", help="Take only these screenshots. See --list for the names.")
    parser.add_argument("--list", action="store_true", help="Print the screenshot names and exit.")
    parser.add_argument(
        "--window-size",
        nargs=2,
        type=int,
        default=(1600, 1100),
        metavar=("WIDTH", "HEIGHT"),
        help="Size to open the window at. Clamped to the screen, which on a short screen crops the control column.",
    )
    parser.add_argument(
        "--render-mode",
        default="Approximated Shapes (Fast)",
        choices=["Points (Fastest)", "Approximated Shapes (Fast)", "Raw Shapes (Slowest)"],
        help="Render mode to draw the detectors with.",
    )
    parser.add_argument(
        "--count-scale",
        default="Logarithmic",
        choices=["Linear", "Logarithmic"],
        help="Scale to colour the integrated counts by. The logarithmic scale usually shows more of the instrument, "
        "because a handful of hot detectors otherwise take up the whole of the colour map.",
    )
    parser.add_argument("--peaks-workspace", help="Use this PeaksWorkspace instead of searching for peaks.")
    parser.add_argument("--no-peaks", action="store_true", help="Skip the peak overlay screenshots.")
    parser.add_argument(
        "--peak-threshold",
        type=float,
        default=30.0,
        help="Peak finding background threshold, in standard deviations above the median spectrum maximum.",
    )
    parser.add_argument("--timeout", type=float, default=300.0, help="Seconds to wait for any one update to finish.")
    args = parser.parse_args(argv)
    if not args.list and not args.source:
        parser.error("a file, run number or workspace name is required")
    return args


class Session:
    """The running window, plus the helpers each shot needs to drive and capture it."""

    def __init__(self, app, window, presenter, output_dir, timeout):
        self.app = app
        self.window = window
        self.view = window.get_instrument_view_widget()
        self.presenter = presenter
        self.model = presenter._model
        self.plotter = self.view.main_plotter
        self.output_dir = output_dir
        self.timeout = timeout
        self.peaks_workspace = None

    # --- waiting -----------------------------------------------------------------------------
    #
    # Most of the presenter's handlers only queue the real work onto its worker thread, and that
    # thread calls back into the view with blocking calls marshalled onto the Qt thread. Waiting
    # on the queue directly would deadlock, so the event loop has to be pumped while waiting.

    def settle(self, renders=6):
        queue = self.presenter._callback_queue
        deadline = time.monotonic() + self.timeout
        while queue.unfinished_tasks > 0:
            if time.monotonic() > deadline:
                print(f"  warning: timed out after {self.timeout}s waiting for the view to update", file=sys.stderr)
                break
            self.app.processEvents()
            time.sleep(0.02)
        for _ in range(renders):
            self.app.processEvents()
            time.sleep(0.05)

    # --- capturing ---------------------------------------------------------------------------

    def save_window(self, name):
        """Save the whole window, with the 3D view composited in.

        ``QWidget.grab`` renders the widget tree itself, which leaves the VTK area blank because
        it draws through a native window handle, so the 3D view is fetched from VTK separately
        and painted into place.
        """
        from qtpy.QtCore import QPoint, QRectF
        from qtpy.QtGui import QImage, QPainter

        pixmap = self.window.grab()
        image = self.plotter.screenshot(return_img=True)
        interactor = self.plotter.interactor
        top_left = interactor.mapTo(self.window, QPoint(0, 0))

        rendered = QImage(image.tobytes(), image.shape[1], image.shape[0], image.shape[1] * 3, QImage.Format.Format_RGB888)
        painter = QPainter(pixmap)
        painter.drawImage(QRectF(top_left.x(), top_left.y(), interactor.width(), interactor.height()), rendered)
        painter.end()
        self._write(pixmap, name)

    def save_widget(self, widget, name, rect=None, crop_to_content=False):
        """Save a single widget. Only valid for widgets Qt paints itself, i.e. not the 3D view."""
        if rect is None and crop_to_content:
            rect = _content_rect(widget)
        pixmap = widget.grab()
        if rect is not None:
            pixmap = pixmap.copy(rect)
        self._write(pixmap, name)

    def save_plotter(self, name):
        """Save just the 3D view, straight out of VTK."""
        path = self.output_dir / f"{name}.png"
        self.plotter.screenshot(str(path))
        print(f"  wrote {path}")

    def _write(self, pixmap, name):
        path = self.output_dir / f"{name}.png"
        if not pixmap.save(str(path)):
            raise RuntimeError(f"Could not save {path}")
        print(f"  wrote {path}")

    # --- driving -----------------------------------------------------------------------------

    def arrange_window(self):
        """Move the splitters off their default positions.

        The control column opens too narrow for its own buttons, so the ends of the labels are
        cut off, and the line plot opens short enough that matplotlib draws its x axis label off
        the bottom of the canvas. Both are easy enough to drag past in use but not in a
        screenshot.
        """
        wanted = self.view._left_column_tabs.sizeHint().width() + self.view._left_column_scroll.frameWidth() * 2
        total = sum(self.view._parent_hsplitter.sizes())
        self.view._parent_hsplitter.setSizes([wanted, max(total - wanted, 1)])

        graphics = sum(self.view._graphics_vsplitter.sizes())
        plotter_height = int(graphics * 0.55)
        self.view._graphics_vsplitter.setSizes([plotter_height, graphics - plotter_height])
        self.settle(renders=4)

    def reset_camera(self):
        """Fit whatever is currently drawn into the view.

        Redrawing keeps the camera where it was, so that changing an option does not throw away
        the zoom, which leaves the instrument off centre after anything that changes how much of
        it is drawn.
        """
        self.plotter.reset_camera()
        self.plotter.render()
        self.settle(renders=4)

    def set_count_scale(self, scale):
        self.view._count_scale_combo_box.setCurrentText(scale)
        self.settle()

    def set_projection(self, projection):
        self.view._projection_combo_box.setCurrentText(projection)
        self.settle()
        self.reset_camera()

    def show_left_tab(self, label):
        tabs = self.view._left_column_tabs
        for index in range(tabs.count()):
            if tabs.tabText(index) == label:
                tabs.setCurrentIndex(index)
                self.settle(renders=2)
                return
        raise RuntimeError(f"No '{label}' tab in the control column")

    def show_grouping_masking_tab(self, label):
        tabs = self.view._picking_masking_tab
        for index in range(tabs.count()):
            if tabs.tabText(index) == label:
                tabs.setCurrentIndex(index)
                self.settle(renders=2)
                return
        raise RuntimeError(f"No '{label}' tab in Grouping and Masking")

    def clear_picking(self):
        self.presenter.on_clear_point_picked_detectors_clicked()
        self.settle()

    def pick_detectors(self, indices):
        """Pick detectors by their index into the model's pickable detectors."""
        for index in indices:
            self.model.update_point_picked_detectors(int(index), False, False)
        self.presenter.update_picked_detectors_on_view()
        self.settle()

    def brightest_detector_indices(self, count):
        """Indices of the pickable detectors with the most counts, so the plots are not empty."""
        counts = self.model.detector_counts
        return np.argsort(counts)[-count:][::-1]

    def detector_indices_with_peaks(self, count):
        """Indices of pickable detectors that a peak sits on, so the peak overlays have something
        to annotate."""
        if self.peaks_workspace is None:
            return self.brightest_detector_indices(count)
        peak_ids = {peak.getDetectorID() for peak in self.peaks_workspace}
        pickable_ids = self.model.pickable_detector_ids
        matches = np.flatnonzero(np.isin(pickable_ids, list(peak_ids)))
        if len(matches) == 0:
            return self.brightest_detector_indices(count)
        # Prefer the brightest of them, otherwise the line plot is mostly noise
        counts = self.model.detector_counts[matches]
        return matches[np.argsort(counts)[-count:][::-1]]

    def add_shape(self, shape, centre=None):
        combo = self.view._shape_selector_combo_box
        if combo.findText(shape) < 0:
            raise RuntimeError(f"No '{shape}' shape in the shape combo box")
        combo.setCurrentText(shape)
        self.view._add_shape_button.setChecked(True)
        self.settle()
        if centre is not None:
            self.move_shape(*centre)

    def move_shape(self, cx, cy):
        manager = self.view._shape_overlay_manager
        if manager is None or manager.current_shape is None:
            raise RuntimeError("No shape is overlaid")
        manager.current_shape.cx = cx
        manager.current_shape.cy = cy
        manager.current_shape.update_plots()
        self.plotter.render()
        self.presenter.on_shape_changed()
        self.settle()

    def remove_shape(self):
        self.view._add_shape_button.setChecked(False)
        self.settle()

    def tick_peaks_workspace(self, name):
        from qtpy.QtCore import Qt

        peak_list = self.view._peak_ws_list
        for row in range(peak_list.count()):
            if peak_list.item(row).text() == name:
                peak_list.item(row).setCheckState(Qt.Checked)
                self.settle()
                return
        raise RuntimeError(f"'{name}' is not in the peaks workspace list")

    def untick_peaks_workspaces(self):
        """Take the peak overlays off again, so they do not clutter the shots that follow."""
        from qtpy.QtCore import Qt

        peak_list = self.view._peak_ws_list
        for row in range(peak_list.count()):
            peak_list.item(row).setCheckState(Qt.Unchecked)
        self.settle()


def _content_rect(widget, margin=6):
    """The area of *widget* its direct children actually occupy.

    Tab pages are as tall as the tab widget, so a short page like Settings is mostly empty
    space. Cropping to the children keeps the screenshot to the controls it is showing.
    """
    from qtpy.QtWidgets import QWidget

    rect = None
    for child in widget.findChildren(QWidget):
        if child.parentWidget() is not widget or not child.isVisible():
            continue
        rect = child.geometry() if rect is None else rect.united(child.geometry())
    if rect is None:
        return None
    return rect.adjusted(-margin, -margin, margin, margin).intersected(widget.rect())


# ---------------------------------------------------------------------------------------------
# The screenshots
# ---------------------------------------------------------------------------------------------


def shot_overview(session):
    """The whole window, on the instrument's default projection, with a spectrum plotted."""
    session.show_left_tab("Home")
    session.pick_detectors(session.brightest_detector_indices(3))
    session.save_window("Overview")
    session.clear_picking()


def shot_home_tab(session):
    """The Home tab of the control column on its own, including the Detector Info box.

    Detector Info hides itself when nothing is selected, so two detectors are picked to bring
    it into the shot.
    """
    session.show_left_tab("Home")
    session.pick_detectors(session.brightest_detector_indices(2))
    session.save_widget(session.view._left_column_home, "HomeTab")
    session.clear_picking()


def shot_projection_options(session):
    """The projection combo box with its list dropped down, showing all eight projections."""
    combo = session.view._projection_combo_box
    combo.showPopup()
    session.settle(renders=4)
    popup = combo.view().window()
    session.save_widget(popup, "ProjectionOptions")
    combo.hidePopup()
    session.settle(renders=2)


def shot_projection_3d(session):
    """The same instrument drawn in 3D, turned so that the layout of the banks can be seen.

    The 3D view opens looking straight down the beam, which does not read as three dimensional
    at all, so the camera is moved to where the first drag of the right button would put it.
    """
    session.set_projection("3D")
    session.pick_detectors(session.brightest_detector_indices(3))
    session.plotter.camera_position = "iso"
    session.reset_camera()
    session.save_window("Projection3D")
    session.clear_picking()


def shot_projection_side_by_side(session):
    """The same instrument with each bank unrolled into its own panel."""
    session.set_projection("Side by Side")
    session.pick_detectors(session.brightest_detector_indices(3))
    session.save_window("ProjectionSideBySide")
    session.clear_picking()


def shot_units_and_contour_range(session):
    """The Units and Contour Range group boxes, with both sliders moved off their limits."""
    session.show_left_tab("Home")
    _narrow_slider(session.view._integration_limit_slider, session.presenter.on_integration_limits_updated)
    _narrow_slider(session.view._contour_range_slider, session.presenter.on_contour_limits_updated)
    session.settle()

    home = session.view._left_column_home
    rect = session.view._integration_limit_group_box.geometry().united(session.view._contour_range_group_box.geometry())
    session.save_widget(home, "UnitsAndContourRange", rect=rect.adjusted(-2, -2, 2, 2))

    session.presenter.on_integration_limits_reset_clicked()
    session.presenter.on_contour_range_reset_clicked()
    session.settle()


def _narrow_slider(slider, on_changed):
    """Pull a range slider's handles in to the middle half of its range."""
    low, high = slider.minimum(), slider.maximum()
    span = high - low
    if span <= 0:
        return
    slider.setValue((low + 0.25 * span, high - 0.25 * span))
    on_changed()


def shot_picking_interaction(session):
    """The Picking/Interaction buttons, with two of them toggled on.

    Rectangle Zoom and Select Bank/Tube are used rather than Hover Pick, which greys out half
    the row while it is on. The peak overlays are turned on for the shot because Select Peaks is
    disabled without them.
    """
    session.show_left_tab("Home")
    session.set_projection(session.model.get_default_projection().value)
    if session.peaks_workspace is not None:
        session.tick_peaks_workspace(session.peaks_workspace.name())
    session.view._rubberband_zoom.setChecked(True)
    session.view._select_bank_tube.setChecked(True)
    session.settle()
    session.save_widget(session.view._picking_group_box, "PickingInteraction")
    session.view._rubberband_zoom.setChecked(False)
    session.view._select_bank_tube.setChecked(False)
    session.untick_peaks_workspaces()


def shot_detector_info(session):
    """The Detector Info box for two detectors, which is when the relative angle is filled in."""
    session.clear_picking()
    session.pick_detectors(session.brightest_detector_indices(2))
    session.save_widget(session.view._detector_info_group_box, "DetectorInfo")


def shot_line_plot(session):
    """The line plot with several spectra plotted separately, so the legend is shown."""
    session.clear_picking()
    session.view._sum_spectra_checkbox.setChecked(False)
    session.presenter.on_sum_spectra_checkbox_clicked()
    session.pick_detectors(session.brightest_detector_indices(4))
    session.save_widget(session.view._lineplot_widget, "LinePlot")
    session.view._sum_spectra_checkbox.setChecked(True)
    session.presenter.on_sum_spectra_checkbox_clicked()
    session.settle()


def shot_peaks_overlay(session):
    """The 3D view with a peaks workspace overlaid, showing the markers and their hkl labels."""
    session.clear_picking()
    session.tick_peaks_workspace(session.peaks_workspace.name())
    session.save_plotter("PeaksOverlay")


def shot_peaks_line_plot(session):
    """The line plot for detectors with peaks on them, showing the dashed peak lines."""
    session.tick_peaks_workspace(session.peaks_workspace.name())
    session.clear_picking()
    session.pick_detectors(session.detector_indices_with_peaks(2))
    session.save_widget(session.view._lineplot_widget, "PeaksLinePlot")


def shot_peak_add_delete(session):
    """The line plot in peak adding mode, with the red cursor drawn on it.

    Turning the mode on clears the selection, so the detector has to be picked afterwards or the
    plot comes out empty.
    """
    session.tick_peaks_workspace(session.peaks_workspace.name())
    session.clear_picking()
    session.view._start_adding_peaks_button.setChecked(True)
    session.settle()
    session.pick_detectors(session.detector_indices_with_peaks(1))
    _draw_peak_cursor(session)
    session.save_widget(session.view._lineplot_widget, "PeakAddDelete")
    session.view._start_adding_peaks_button.setChecked(False)
    session.settle()


def _draw_peak_cursor(session):
    """Put the peak cursor on the canvas.

    Matplotlib's Cursor only draws in response to mouse movement, and by default blits rather
    than repainting, neither of which happens on its own here. Turning blitting off and sending
    one synthetic move leaves the cursor in the canvas for the grab to pick up.
    """
    from matplotlib.backend_bases import MouseEvent

    cursor = session.view._lineplot_peak_cursor
    if cursor is None:
        return
    cursor.useblit = False
    canvas = session.view._detector_figure_canvas
    axes = session.view._detector_spectrum_axes
    # Two thirds of the way along the axes, which keeps the cursor clear of the y axis labels
    x_pixel, y_pixel = axes.transData.transform(_axes_fraction_to_data(axes, 0.66, 0.5))
    cursor.onmove(MouseEvent("motion_notify_event", canvas, x_pixel, y_pixel))
    canvas.draw()
    session.settle(renders=2)


def _axes_fraction_to_data(axes, x_fraction, y_fraction):
    x_low, x_high = axes.get_xlim()
    y_low, y_high = axes.get_ylim()
    return x_low + x_fraction * (x_high - x_low), y_low + y_fraction * (y_high - y_low)


def shot_shape_overlay(session):
    """The 3D view with a selection shape overlaid, including its rotation handle."""
    session.untick_peaks_workspaces()
    session.clear_picking()
    session.set_projection("Side by Side")
    session.add_shape("Ellipse")
    session.save_plotter("ShapeOverlay")
    session.remove_shape()


def shot_grouping_tab(session):
    """The Grouping and Masking box on the Grouping tab, with two regions of interest added."""
    session.untick_peaks_workspaces()
    session.show_left_tab("Home")
    session.set_projection("Side by Side")
    session.show_grouping_masking_tab("Grouping")
    session.presenter.on_clear_list_clicked()
    session.settle()
    for centre in ((0.35, 0.5), (0.62, 0.5)):
        session.add_shape("Rectangle", centre=centre)
        session.presenter.on_add_item_clicked()
        session.settle()
    session.remove_shape()
    session.save_widget(session.view._grouping_masking_group_box, "GroupingTab")


def shot_masking_tab(session):
    """The whole window on the Masking tab, with a mask applied so the detectors are drawn grey."""
    session.untick_peaks_workspaces()
    session.show_left_tab("Home")
    session.set_projection("Side by Side")
    session.show_grouping_masking_tab("Masking")
    session.presenter.on_clear_list_clicked()
    session.settle()
    session.add_shape("Circle", centre=(0.5, 0.5))
    session.presenter.on_add_item_clicked()
    session.settle()
    session.remove_shape()
    session.save_window("MaskingTab")
    session.presenter.on_clear_list_clicked()
    session.settle()


def shot_settings_tab(session):
    """The Settings tab, with Monitors and Sample ticked so their indicator dots are coloured."""
    session.show_left_tab("Settings")
    for check_box in (session.view._show_monitors_check_box, session.view._show_sample_position_check_box):
        check_box.setChecked(True)
        check_box.clicked.emit(True)
        check_box.toggled.emit(True)
    session.settle()
    session.save_widget(session.view._left_column_settings, "SettingsTab", crop_to_content=True)
    session.show_left_tab("Home")


def shot_component_tree(session):
    """The Component Tree tab with a bank selected, so the rest of the instrument is greyed out."""
    from qtpy.QtCore import QItemSelectionModel

    session.untick_peaks_workspaces()
    session.clear_picking()
    session.set_projection(session.model.get_default_projection().value)
    session.show_left_tab("Component Tree")
    tree = session.view.component_tree
    tree_model = tree.model()

    root = tree_model.index(0, 0)
    tree.expand(root)
    session.settle(renders=2)

    # Expand the smallest branch rather than the selected one: a bank has a row per tube, so
    # opening it fills the whole column with identical rows and nothing else can be seen
    smallest = _child_by_subtree_size(session, tree_model, root, largest=False)
    if smallest is not None:
        tree.expand(smallest)
        session.settle(renders=2)

    bank = _child_by_subtree_size(session, tree_model, root, largest=True)
    if bank is not None:
        tree.selectionModel().select(bank, QItemSelectionModel.SelectionFlag.ClearAndSelect | QItemSelectionModel.SelectionFlag.Rows)
        session.settle()
    tree.scrollToTop()

    # Only the selected bank is drawn now, so the camera has to be refitted around it, and the
    # detectors picked afterwards so that they come from the bank that is left
    session.reset_camera()
    session.pick_detectors(session.brightest_detector_indices(2))
    session.save_window("ComponentTree")

    session.clear_picking()
    tree.selectionModel().clearSelection()
    session.settle()
    session.reset_camera()
    session.show_left_tab("Home")


def _child_by_subtree_size(session, tree_model, parent_index, largest):
    """The child of *parent_index* with the most, or fewest, components under it.

    The largest is the biggest bank, which is the useful thing to select. The smallest branch
    that still has children is the one to expand, because it shows the nesting without filling
    the column.
    """
    from instrumentview.ComponentTreePresenter import _COMPONENT_INDEX_ROLE

    component_info = session.model.workspace.componentInfo()
    best_index, best_size = None, None
    for row in range(tree_model.rowCount(parent_index)):
        index = tree_model.index(row, 0, parent_index)
        component_index = tree_model.itemFromIndex(index).data(_COMPONENT_INDEX_ROLE)
        if component_index is None or component_index < 0:
            continue
        size = len(component_info.componentsInSubtree(int(component_index)))
        if size <= 1:
            # A leaf: nothing to expand, and not worth selecting
            continue
        if best_size is None or (size > best_size if largest else size < best_size):
            best_index, best_size = index, size
    return best_index


_SHOT_FUNCTIONS = {
    "Overview": shot_overview,
    "HomeTab": shot_home_tab,
    "ProjectionOptions": shot_projection_options,
    "Projection3D": shot_projection_3d,
    "ProjectionSideBySide": shot_projection_side_by_side,
    "UnitsAndContourRange": shot_units_and_contour_range,
    "PickingInteraction": shot_picking_interaction,
    "DetectorInfo": shot_detector_info,
    "LinePlot": shot_line_plot,
    "PeaksOverlay": shot_peaks_overlay,
    "PeaksLinePlot": shot_peaks_line_plot,
    "PeakAddDelete": shot_peak_add_delete,
    "ShapeOverlay": shot_shape_overlay,
    "GroupingTab": shot_grouping_tab,
    "MaskingTab": shot_masking_tab,
    "SettingsTab": shot_settings_tab,
    "ComponentTree": shot_component_tree,
}


# ---------------------------------------------------------------------------------------------
# Workspace setup
# ---------------------------------------------------------------------------------------------


def load_workspace(source):
    from mantid.simpleapi import AnalysisDataService, Load

    if AnalysisDataService.doesExist(source):
        print(f"Using workspace '{source}' from the ADS")
        return AnalysisDataService.retrieve(source)

    name = Path(source).stem or source
    print(f"Loading {source}")
    return Load(Filename=source, OutputWorkspace=name)


def check_workspace_has_instrument(workspace):
    if (
        not workspace.getInstrument()
        or not workspace.getInstrumentName()
        or not workspace.getAxis(1).isSpectra()
        or workspace.detectorInfo().detectorIDs().size == 0
    ):
        raise RuntimeError(f"'{workspace.name()}' has no instrument with detectors attached, so it cannot be displayed")


def find_peaks(workspace, threshold):
    """Find peaks and index them, so the overlays are labelled with real Miller indices.

    Peak finding needs no more than a rough answer here; the screenshots only have to look like
    a real result. If the UB cannot be found the peaks are still usable, they are just all
    labelled (0, 0, 0).
    """
    from mantid.kernel import logger
    from mantid.simpleapi import FindSXPeaks, FindUBUsingFFT, IndexPeaks

    counts = np.max(workspace.extractY(), axis=1)
    median = np.median(counts)
    # 1.2815 is the 90th percentile of a standard normal, so this reads the spread of the
    # spectrum maxima off the top decile without being skewed by the peaks themselves
    spread = (np.percentile(counts, 90) - median) / 1.2815
    background = median + threshold * spread

    name = f"{workspace.name()}_peaks"
    print(f"Finding peaks in '{workspace.name()}' above a background of {background:.1f}")
    peaks = FindSXPeaks(
        InputWorkspace=workspace,
        PeakFindingStrategy="AllPeaks",
        AbsoluteBackground=background,
        ResolutionStrategy="AbsoluteResolution",
        XResolution=200,
        PhiResolution=2,
        TwoThetaResolution=2,
        OutputWorkspace=name,
    )
    if peaks.getNumberPeaks() == 0:
        logger.warning("No peaks found, so the peak screenshots will be skipped")
        return None

    try:
        FindUBUsingFFT(PeaksWorkspace=peaks, MinD=1, MaxD=10, Tolerance=0.15)
        indexed, *_ = IndexPeaks(PeaksWorkspace=peaks, Tolerance=0.15, CommonUBForAll=True)
        print(f"Found {peaks.getNumberPeaks()} peaks, {indexed} of them indexed")
    except Exception as exception:
        # Unindexed peaks still overlay, they are just all labelled (0, 0, 0), which is a better
        # outcome than losing the three peak screenshots because the UB search did not converge
        logger.warning(f"Could not index the peaks, so they will all be labelled (0, 0, 0): {exception}")
    return peaks


# ---------------------------------------------------------------------------------------------


def main(argv=None):
    args = parse_args(argv)

    if args.list:
        for name in _SHOT_ORDER:
            print(name)
        print("\nContextMenu is not generated here: it shows the Workbench workspace context menu.")
        return 0

    requested = args.only or _SHOT_ORDER
    unknown = [name for name in requested if name not in _SHOT_FUNCTIONS]
    if unknown:
        raise SystemExit(f"Unknown screenshot(s): {', '.join(unknown)}. Use --list to see the names.")
    # Keep the documented order however they were given on the command line, so that the state
    # each shot leaves behind is the state the next one expects
    requested = [name for name in _SHOT_ORDER if name in requested]

    args.output_dir.mkdir(parents=True, exist_ok=True)

    from qtpy.QtWidgets import QApplication

    app = QApplication.instance() or QApplication(sys.argv[:1])

    from mantid import ConfigService
    from mantid.simpleapi import AnalysisDataService
    from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
    from instrumentview.FullInstrumentViewPresenter import FullInstrumentViewPresenter
    from instrumentview.FullInstrumentViewWindow import FullInstrumentViewWindow

    workspace = load_workspace(args.source)
    check_workspace_has_instrument(workspace)

    peaks = None
    if args.no_peaks:
        requested = [name for name in requested if name not in _SHOTS_NEEDING_PEAKS]
    elif _SHOTS_NEEDING_PEAKS.intersection(requested):
        if args.peaks_workspace:
            peaks = AnalysisDataService.retrieve(args.peaks_workspace)
        else:
            peaks = find_peaks(workspace, args.peak_threshold)
        if peaks is None:
            requested = [name for name in requested if name not in _SHOTS_NEEDING_PEAKS]

    config = ConfigService.Instance()
    saved_config = {key: config[key] for key in _CONFIG_KEYS}
    # The view reads these when it is built, so they have to be set before the window is made
    config["InstrumentView.MaintainAspectRatio"] = "No"
    config["InstrumentView.FlipBeam"] = "No"
    config["InstrumentView.RenderMode"] = args.render_mode

    window = FullInstrumentViewWindow()
    try:
        # The whole window has to be on screen: VTK renders what the graphics card is showing, so
        # anything hanging off the edge of the screen comes back missing or garbled
        available = app.primaryScreen().availableGeometry()
        size = (min(args.window_size[0], available.width()), min(args.window_size[1], available.height()))
        if size != tuple(args.window_size):
            print(f"note: window clamped to {size[0]}x{size[1]} to fit the screen", file=sys.stderr)
        window.resize(*size)
        window.move(available.topLeft())
        presenter = FullInstrumentViewPresenter(window.get_instrument_view_widget(), FullInstrumentViewModel(workspace))
        window.show()

        session = Session(app, window, presenter, args.output_dir, args.timeout)
        session.peaks_workspace = peaks
        session.settle(renders=10)
        session.arrange_window()
        session.set_count_scale(args.count_scale)

        failures = []
        for name in requested:
            print(f"{name}:")
            try:
                _SHOT_FUNCTIONS[name](session)
            except Exception as exception:
                # A shot that fails is usually one whose widget has been renamed. Carry on so the
                # rest are still taken, and report which ones need looking at at the end.
                failures.append(name)
                print(f"  failed: {exception}", file=sys.stderr)
    finally:
        window.close()
        app.processEvents()
        for key, value in saved_config.items():
            config[key] = value

    if failures:
        print(f"\n{len(failures)} screenshot(s) failed: {', '.join(failures)}", file=sys.stderr)
        return 1
    print(f"\nWrote {len(requested)} screenshot(s) to {args.output_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
