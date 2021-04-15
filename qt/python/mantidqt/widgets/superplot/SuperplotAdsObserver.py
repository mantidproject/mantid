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
    wsDeleted = Signal(str)

    """
    Emitted when a workspace is renamed.
    Args:
        str: old name
        str: new name
    """
    wsRenamed = Signal(str, str)

    def __init__(self):
        super().__init__()


class SuperplotAdsObserver(AnalysisDataServiceObserver):

    def __init__(self):
        super().__init__()
        self.signals = SuperplotAdsObserverSignals()
        self.observeDelete(True)
        self.observeRename(True)

    def __del__(self):
        self.observeAll(False)

    def deleteHandle(self, wsName, ws):
        """
        Triggered when a workspace is deleted.

        Args:
            wsName (str): name of the workspace
            ws (workspace): reference to the workspace
        """
        self.signals.wsDeleted.emit(wsName)

    def renameHandle(self, oldName, newName):
        """
        Triggered when a workspace is renamed.

        Args:
            oldName (str): old name of the workspace
            newName (str): new name of the workspace
        """
        self.signals.wsRenamed.emit(oldName, newName)
