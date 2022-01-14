# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtCore import *

from mantid.simpleapi import mtd, DeleteWorkspace
from mantid.kernel import logger

from ...utils.asynchronous import set_interval
from ..memorywidget.memoryinfo import get_memory_info


class MemoryManager(QObject):
    """
    Gets system memory usage information and manages the caching of the workspaces loaded by the raw data explorer.
    It keeps track of the loaded workspaces and delete the oldest ones if the memory goes above MEMORY_THRESHOLD percent
    Update is made every TIME_INTERVAL_MEMORY_USAGE_UPDATE (s) using threading.Timer;
    """

    """
    Interval between each memory update, in seconds.
    """
    TIME_INTERVAL_MEMORY_USAGE_UPDATE = 5.000  # in s

    """
    Limit at which the memory manager starts to free memory.
    """
    MEMORY_THRESHOLD = 80  # in percentage of the total

    """
    Signal sent when freeing memory is needed.
    """
    sig_free_memory = Signal()

    """
    List containing all the workspaces loaded by the raw data explorer, sorted from most ancient to most recent
    """
    _current_workspaces = None

    def __init__(self, rdexp_model):
        super().__init__()

        self.rdexp_model = rdexp_model
        self._current_workspaces = []

        self.update_allowed = True
        self.update_memory_usage()
        self.thread_stopper = self.update_memory_usage_threaded()
        self.sig_free_memory.connect(self.on_free_requested)

    def __del__(self):
        self.cancel_memory_update()

    @set_interval(TIME_INTERVAL_MEMORY_USAGE_UPDATE)
    def update_memory_usage_threaded(self):
        """
        Calls update_memory_usage once every TIME_INTERVAL_MEMORY_USAGE_UPDATE
        If the memory is too high, ask for workspace deletion.
        """
        mem_used_percent = self.update_memory_usage()
        if mem_used_percent is None:
            return

        if mem_used_percent > self.MEMORY_THRESHOLD:
            self.sig_free_memory.emit()

    def update_memory_usage(self):
        """
        Gets memory usage information and passes it to the view
        """
        if self.update_allowed:
            mem_used_percent, mem_used, mem_avail = get_memory_info()
            return mem_used_percent

    def cancel_memory_update(self):
        """
        Ensures that the thread will not restart after it finishes next, as well as attempting to cancel it.
        """
        self.update_allowed = False
        self.thread_stopper.set()

    def workspace_interacted_with(self, ws_name):
        """
        If a workspace is used for a preview, it is considered interacted with and
        thus moved at the end of the queue for deletion
        @param ws_name: the name of the workspace
        """
        try:
            # if the workspace already exists, we remove it
            self._current_workspaces.remove(ws_name)
        except ValueError:
            pass
        self._current_workspaces.append(ws_name)

    def on_free_requested(self):
        """
        Slot called when a workspace needs to be deleted to free memory.
        It will delete the oldest workspace managed by the raw data explorer that is not currently shown in a preview.
        """
        index = 0
        while index < len(self._current_workspaces):
            ws_name = self._current_workspaces[index]
            if self.rdexp_model.can_delete_workspace(ws_name):
                self._current_workspaces.pop(index)
                if mtd.doesExist(ws_name):
                    logger.information("Deleting cached workspace " + ws_name)
                    DeleteWorkspace(Workspace=ws_name)
                    break
            index += 1
