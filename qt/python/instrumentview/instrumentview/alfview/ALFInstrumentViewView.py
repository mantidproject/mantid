# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


from qtpy.QtWidgets import QHBoxLayout, QVBoxLayout, QWidget
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewView
from typing import override


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
        super().__init__(parent)
        self._detector_figure_canvas = NullWidget()

    @override
    def _set_layouts(self):
        parent_layout = QHBoxLayout(self)
        options_widget = QWidget()
        options_layout = QVBoxLayout(options_widget)
        options_layout.addWidget(self._select_bank_tube)
        options_layout.addWidget(self._add_rectangle)
        options_layout.addWidget(self._add_selection)
        options_layout.addWidget(self._show_shapes_check_box)
        options_layout.addWidget(self._projection_combo_box)
        options_layout.addWidget(self._spacer)
        parent_layout.addWidget(options_widget)
        parent_layout.addWidget(self.main_plotter)

    def closeEvent(self, event):
        super().close_view()
