# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.alfview.ALFInstrumentViewView import ALFInstrumentViewView
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.FullInstrumentViewPresenter import FullInstrumentViewPresenter

from mantid.simpleapi import CreateSampleWorkspace


class ALFInstrumentViewPresenter(FullInstrumentViewPresenter):
    """Minimal presenter used by the C++ ALF python bridge.

    This keeps the import and construction path lightweight so the C++ side can
    always acquire a Qt widget from the `view` attribute.
    """

    def __init__(self, view=None):
        _placeholder_ws = CreateSampleWorkspace(InstrumentName="ALF", StoreInADS=False)
        super().__init__(ALFInstrumentViewView(), FullInstrumentViewModel(_placeholder_ws))

    def selected_detector_ids(self):
        return []
