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
        options_layout.addWidget(self._add_rectangle)
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
