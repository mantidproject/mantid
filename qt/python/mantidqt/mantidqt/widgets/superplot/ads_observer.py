# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


from mantid.api import AnalysisDataServiceObserver

from qtpy.QtCore import QObject, Signal


class SuperplotAdsObserverSignals(QObject):
    """
    Emitted when a workspace is deleted.
    Args:
        str: name of the deleted workspace
    """

    sig_ws_deleted = Signal(str)

    """
    Emitted when a workspace is renamed.
    Args:
        str: old name
        str: new name
    """
    sig_ws_renamed = Signal(str, str)

    """
    Emitted when a workspace is replaced.
    Args:
        str: name of the workspace
    """
    sig_ws_replaced = Signal(str)

    def __init__(self):
        super().__init__()


class SuperplotAdsObserver(AnalysisDataServiceObserver):
    def __init__(self):
        super().__init__()
        self.signals = SuperplotAdsObserverSignals()
        self.observeDelete(True)
        self.observeRename(True)
        self.observeReplace(True)

    def __del__(self):
        self.observeAll(False)

    def deleteHandle(self, ws_name, ws):
        """
        Triggered when a workspace is deleted.

        Args:
            ws_name (str): name of the workspace
            ws (workspace): reference to the workspace
        """
        self.signals.sig_ws_deleted.emit(ws_name)

    def renameHandle(self, old_name, new_name):
        """
        Triggered when a workspace is renamed.

        Args:
            old_name (str): old name of the workspace
            new_name (str): new name of the workspace
        """
        self.signals.sig_ws_renamed.emit(old_name, new_name)

    def replacedHandle(self, ws_name, ws):
        """
        Triggered when a workspace is replaces.

        Args:
            ws_name (str): name of the workspace
            ws (workspace): reference to the workspace
        """
        self.signals.sig_ws_replaced.emit(ws_name)
