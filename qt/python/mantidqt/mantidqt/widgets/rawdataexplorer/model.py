# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import os.path
from qtpy.QtCore import *

from mantid.simpleapi import Load, config, mtd, Plus, RenameWorkspace, DeleteWorkspace
from mantid.kernel import logger

from ...utils.asynchronous import set_interval
from ..memorywidget.memoryinfo import get_memory_info
from .PreviewFinder import PreviewFinder, AcquisitionType


class PreviewModel(QObject):

    """
    Type of the preview.
    """
    _type = None

    """
    Name of the current workspace.
    """
    _workspace_name = None

    """
    Sent when the workspace is changed.
    """
    sig_workspace_changed = Signal()

    def __init__(self, preview_type, workspace_name):
        super().__init__()
        self._type = preview_type
        self._workspace_name = workspace_name

    def get_preview_type(self):
        """
        Get the type of the preview.
        @return (PreviewType): preview type
        """
        return self._type

    def set_workspace_name(self, workspace_name):
        """
        Set the workspace. This function raises the corresponding signal.
        @param workspace_name (str): name of the workspace
        """
        self._workspace_name = workspace_name
        self.sig_workspace_changed.emit()

    def get_workspace_name(self):
        """
        Get the name of the current workspace.
        @return (str): name
        """
        return self._workspace_name


class MemoryManager(QObject):
    """
    Gets system memory usage information and manages the caching of the workspaces loaded by the raw data explorer.
    It keeps track of the loaded workspaces and delete the oldest ones if the memory goes above MEMORY_THRESHOLD percent
    Update is made every TIME_INTERVAL_MEMORY_USAGE_UPDATE (s) using threading.Timer;
    """

    TIME_INTERVAL_MEMORY_USAGE_UPDATE = 5.000  # in s
    MEMORY_THRESHOLD = 80  # in percentage of the total

    sig_free_memory = Signal()

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
                    logger.information("Deleting cached workspace ", ws_name)
                    DeleteWorkspace(Workspace=ws_name)
                    break
            index += 1


class RawDataExplorerModel(QObject):
    """
    Model for the RawDataExplorer widget
    """

    """
    Signal that a preview model has been added.
    Args:
        PreviewModel: the new preview model
    """
    sig_new_preview = Signal(PreviewModel)

    """
    List of the current previews.
    """
    _previews = None

    def __init__(self, presenter):
        """
        Initialise the model
        @param presenter: The presenter controlling this model
        """
        super().__init__()
        self.presenter = presenter

        self._previews = list()

        self.instrument = config["default.instrument"]

        self.memory_manager = MemoryManager(self)

    def on_instrument_changed(self, new_instrument):
        """
        Slot triggered by changing the instrument
        @param new_instrument: the name of the instrument, from the instrument selector
        """
        self.instrument = new_instrument
        self.presenter.populate_acquisitions()

    def modify_preview(self, filename):
        """
        Modify the last preview of the same type. If none is found, a new preview is opened.
        @param filename: the full path to the file to show
        """

        ws_name = os.path.basename(filename)[:-4]

        if not mtd.doesExist(ws_name):
            Load(Filename=filename, OutputWorkspace=ws_name)

        self.memory_manager.workspace_interacted_with(ws_name)

        # determine the preview given the prepared data
        preview = self.choose_preview(ws_name)

        if preview is None:
            return

        for current_preview in reversed(self._previews):
            if current_preview.get_preview_type() == preview:
                if self.presenter.is_accumulate_checked():
                    ws_name = self.accumulate(current_preview.get_workspace_name(), ws_name)
                    if ws_name is None:
                        return
                    self.memory_manager.workspace_interacted_with(ws_name)
                current_preview.set_workspace_name(ws_name)
                return

        preview_model = PreviewModel(preview, ws_name)
        self._previews.append(preview_model)
        self.sig_new_preview.emit(preview_model)

    def del_preview(self, previewModel):
        """
        Delete a preview model.
        @param previewModel(PreviewModel): reference to the model.
        """
        self._previews.remove(previewModel)

    def can_delete_workspace(self, ws_name):
        """
        Checks if a workspace is not shown in any preview and thus can be deleted.
        @param ws_name: the name of the workspace
        @return whether the workspace can safely be deleted
        """
        for preview in self._previews:
            if preview.get_workspace_name() == ws_name:
                return False
        return True

    def choose_preview(self, ws_name):
        """
        Reading the dimensions of the data, this determines the plot to use to show it.
        @param ws_name: the name of the workspace which data are to be shown
        @return the preview type if it was determined, else None
        """
        workspace = mtd[ws_name]

        preview_finder = PreviewFinder()

        if workspace.isGroup():
            # that's probably D7 or some processed data
            if workspace.size() == 0:
                return

            workspace = workspace.getItem(0)
        instrument_name = workspace.getInstrument().getName()
        is_acquisition_type_needed = preview_finder.need_acquisition_mode(instrument_name)
        if is_acquisition_type_needed:
            acquisition_mode = self.determine_acquisition_mode(workspace)
            return preview_finder.get_preview(instrument_name, acquisition_mode)
        else:
            return preview_finder.get_preview(instrument_name)

    @staticmethod
    def determine_acquisition_mode(workspace):
        """
        Reading the workspace run's parameters, attempts to find what kind of acquisition was used to get this data.
        @param workspace: the workspace to check
        @return (AcquisitionType) the predicted acquisition type
        """
        if workspace.blocksize() > 1:
            # TODO FIND A BETTER WAY TO DISSOCIATE THE 2
            # particularly, TOF can also be only channels -> what then ?
            if str(workspace.getAxis(0).getUnit().symbol()) == "microsecond":
                # TOF case
                return AcquisitionType.TOF
            else:
                # SCAN case
                return AcquisitionType.SCAN
        else:
            return AcquisitionType.MONO

    @staticmethod
    def accumulate(target_ws, ws_to_add):
        """
        Accumulate the new workspace on the currently relevant opened workspace
        @param target_ws: the workspace on which to sum
        @param ws_to_add: the new workspace to add
        @return the name of the workspace containing the accumulation, that is to be shown
        """

        final_ws = RawDataExplorerModel.accumulate_name(target_ws, ws_to_add)

        try:
            Plus(LHSWorkspace=target_ws, RHSWorkspace=ws_to_add, OutputWorkspace=target_ws)
        except ValueError as e:
            logger.error("Unable to accumulate: {0}".format(e))
            return None

        RenameWorkspace(InputWorkspace=target_ws, OutputWorkspace=final_ws)
        return final_ws

    @staticmethod
    def accumulate_name(last_ws, ws_to_add):
        """
        Create the name of a workspace created through accumulation
        @param last_ws: the name of the workspace on which to accumulate
        @param ws_to_add: the name of the workspace that will be added
        @return the name of the accumulated workspace
        """
        split = last_ws.split("_")
        if len(split) >= 2:
            ws_name = split[0] + "_..._" + ws_to_add
        elif len(split) == 1:
            ws_name = last_ws + "_" + ws_to_add
        else:
            # shouldn't reach this case with the naming convention, but if it happens, default to the new name
            ws_name = ws_to_add
        return ws_name
