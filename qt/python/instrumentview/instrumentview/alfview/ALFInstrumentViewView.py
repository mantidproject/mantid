# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


from qtpy.QtWidgets import QHBoxLayout, QVBoxLayout, QWidget
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewView
from typing import override
import re
from qtpy.QtWidgets import QLineEdit, QPushButton


class NullWidget:
    """Placeholder for optional widgets that are absent in this view variant."""

    def __getattr__(self, name):
        return lambda *args, **kwargs: None


# TODO: figure out if @run_on_qapp_thread() needed
class ALFInstrumentViewView(FullInstrumentViewView):
    """A minimal instrument view for ALFView.

    Contains only a pyvista BackgroundPlotter for 3D instrument rendering.
    The BackgroundPlotter is created lazily via ``initialise()`` to avoid
    OpenGL context errors when VTK tries to render before the widget is
    embedded in its final layout.
    """

    def __init__(self, parent=None):
        self._detector_figure_canvas = NullWidget()

        self.rebin_input = QLineEdit()
        self.rebin_input.setPlaceholderText("5.5,0.01,6")
        self.rebin_input.textChanged.connect(self._update_rebin_button_state)

        self.rebin_btn = QPushButton("Rebin")
        self.rebin_btn.setEnabled(False)  # disabled until input is valid
        self.rebin_btn.clicked.connect(self._on_rebin_clicked)
        super().__init__(parent)

    def _parse_rebin_args(self, text: str):
        pattern = r"^\s*([+-]?\d*\.?\d+)\s*,\s*([+-]?\d*\.?\d+)\s*,\s*([+-]?\d*\.?\d+)\s*$"
        match = re.match(pattern, text)
        if match:
            return text
        return None

    def _update_rebin_button_state(self, text: str):
        self.rebin_btn.setEnabled(self._parse_rebin_args(text) is not None)

    def _on_rebin_clicked(self):
        params = self._parse_rebin_args(self.rebin_input.text())
        if params is None:
            return
        self._presenter.rebin_button_clicked(params)

    @override
    def _set_layouts(self):
        parent_layout = QHBoxLayout(self)
        options_widget = QWidget()
        options_layout = QVBoxLayout(options_widget)
        options_layout.addWidget(self._add_rectangle)
        options_layout.addWidget(self._add_selection)
        options_layout.addWidget(self.rebin_btn)
        options_layout.addWidget(self.rebin_input)
        # NOTE: Widgets in the full view can be added as needed
        # options_layout.addWidget(self._select_bank_tube)
        # options_layout.addWidget(self._show_shapes_check_box)
        # options_layout.addWidget(self._projection_combo_box)
        options_layout.addWidget(self._spacer)
        options_widget.setFixedWidth(options_layout.sizeHint().width())
        parent_layout.addWidget(options_widget)
        parent_layout.addWidget(self.main_plotter)

    def closeEvent(self, event):
        super().close_view()
