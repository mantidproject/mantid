# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


from qtpy.QtWidgets import QHBoxLayout, QVBoxLayout, QWidget, QLineEdit, QPushButton, QSizePolicy, QSplitter
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewView
from mantidqt.utils.qt.qappthreadcall import run_on_qapp_thread
from typing import override
import re


# TODO: figure out if @run_on_qapp_thread() needed
@run_on_qapp_thread()
class ALFInstrumentViewView(FullInstrumentViewView):
    """A minimal instrument view for ALFView.

    Contains only a pyvista BackgroundPlotter for 3D instrument rendering.
    The BackgroundPlotter is created lazily via ``initialise()`` to avoid
    OpenGL context errors when VTK tries to render before the widget is
    embedded in its final layout.
    """

    def __init__(self, parent=None):
        self.rebin_input = QLineEdit()
        self.rebin_input.setPlaceholderText("5.5,0.01,6")
        self.rebin_input.textChanged.connect(self._update_rebin_button_state)

        self.rebin_btn = QPushButton("Rebin")
        self.rebin_btn.setEnabled(False)  # disabled until input is valid
        self.rebin_btn.clicked.connect(self._on_rebin_clicked)

        super().__init__(parent)

        # NOTE: After __init__ to overwride lineplot with placeholder
        self._detector_info_group_box.setVisible(False)

    def _parse_rebin_args(self, text: str):
        pattern = r"^\s*([+-]?\d*\.?\d+)\s*,\s*([+-]?\d*\.?\d+)\s*,\s*([+-]?\d*\.?\d+)\s*$"
        match = re.match(pattern, text)
        if not match:
            return None
        start, step, end = map(float, match.groups())
        if step == 0 or (end - start) * step <= 0:
            return None
        return text

    def _update_rebin_button_state(self, text: str):
        self.rebin_btn.setEnabled(self._parse_rebin_args(text) is not None)

    def _on_rebin_clicked(self):
        params = self._parse_rebin_args(self.rebin_input.text())
        if params is None:
            return
        self._presenter.rebin_button_clicked(params)

    @override
    def setup_connections_to_presenter(self):
        super().setup_connections_to_presenter()
        # ALF has no shape selector, so "Add ROI" is the rectangle control itself rather than the
        # base class button that commits an already drawn shape to the (hidden) ROI list.
        self._add_selection.clicked.disconnect()
        self._add_selection.setCheckable(True)
        self._add_selection.setEnabled(True)
        self._add_selection.setToolTip("Overlay a rectangle on the projection to select a region of detectors.")
        self._add_selection.toggled.connect(self._on_add_roi_toggled)

    def _on_add_roi_toggled(self, checked: bool):
        if not checked:
            self.delete_current_overlaid_shape()
            return
        self.add_rectangular_widget()

    # ALF drives its tube selection from the rectangle rather than showing the base class's
    # line-plot-only preview, so it registers its own callback here instead.
    @override
    def _register_shape_changed_callback(self) -> None:
        if self._shape_overlay_manager is None:
            return
        self._shape_overlay_manager.set_on_shape_changed(self._presenter.on_roi_shape_changed)
        # Select whatever the rectangle covers where it is first drawn
        self._presenter.on_roi_shape_changed()

    # NOTE: "Add ROI" creates the rectangle rather than acting on an existing one, so unlike in the
    # full view it must stay enabled when no shape is overlaid.
    @override
    def set_add_selection_and_mask_buttons_enabled(self, enabled: bool):
        return

    @override
    def set_overlaid_shape_controls_enabled(self, enabled: bool):
        if not enabled:
            self._add_selection.setChecked(False)
        self._add_selection.setEnabled(enabled)

    @override
    def set_overlaid_shape_controls_checked(self, checked: bool):
        self._add_selection.setChecked(checked)

    @override
    def set_selected_detector_info(self, detector_infos):
        return

    @override
    def set_relative_detector_angle(self, angle):
        return

    # NOTE: Ignore setting camera position because any update to the view should reset it
    @override
    def set_camera_to_cached_state(self) -> None:
        return

    @override
    def _set_layouts(self):
        parent_layout = QHBoxLayout(self)
        options_widget = QWidget()
        options_layout = QVBoxLayout(options_widget)
        options_layout.addWidget(self._add_selection)
        options_layout.addWidget(self._hover_pick)
        options_layout.addWidget(self.rebin_btn)
        options_layout.addWidget(self.rebin_input)
        self._detector_figure_canvas.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        options_layout.addWidget(self._detector_figure_canvas, 1)
        options_layout.addWidget(self._spacer)
        options_widget.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Expanding)
        splitter = QSplitter()
        splitter.addWidget(options_widget)
        splitter.addWidget(self.main_plotter.app_window)
        splitter.setSizes([300, 700])  # Initial split proportions
        parent_layout.addWidget(splitter)
