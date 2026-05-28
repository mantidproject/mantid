# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.alfview.ALFInstrumentViewView import ALFInstrumentViewView
from qtpy.QtWidgets import QWidget


class ALFInstrumentViewPresenter:
    """Minimal presenter used by the C++ ALF python bridge.

    This keeps the import and construction path lightweight so the C++ side can
    always acquire a Qt widget from the `view` attribute.
    """

    def __init__(self, view=None):
        self.view = view or self._safe_create_view()

    @staticmethod
    def _safe_create_view():
        try:
            return ALFInstrumentViewView()
        except Exception:
            # Fallback keeps the C++ bridge alive if optional rendering deps fail.
            return QWidget()

    def initialise(self):
        initialise = getattr(self.view, "initialise", None)
        if callable(initialise):
            initialise()

    def selected_detector_ids(self):
        return []
