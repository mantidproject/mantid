# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os.path
from qtpy.QtCore import *

from mantid.simpleapi import mtd, Plus, RenameWorkspace
from mantid.api import AlgorithmManager
from mantid.kernel import logger
from _dataobjects import PeaksWorkspace, TableWorkspace

from .PreviewFinder import PreviewFinder, AcquisitionType
from .memoryManager import MemoryManager


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
    Signal sent when the workspace is changed.
    """
    sig_workspace_changed = Signal()

    """
    Signal requesting the preview to close
    """
    sig_request_close = Signal()

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

        self.memory_manager = MemoryManager(self)

    def modify_preview(self, filename):
        """
        Modify the last preview of the same type. If none is found, a new preview is opened.
        @param filename: the full path to the file to show
        """

        ws_name = os.path.basename(filename)[:-4]

        # load the file if necessary
        if not mtd.doesExist(ws_name):
            success = self.load_file(ws_name, filename)
            if not success:
                return

        # notify the memory manager about this interaction
        self.memory_manager.workspace_interacted_with(ws_name)

        # determine the preview given the prepared data
        preview = self.choose_preview(ws_name)

        if preview is None:
            return

        # check the previews currently opened in case one of the correct type already exists and can be replaced.
        # NB: currently, previews only has one element max at a given time, but that could change so the design stays.
        for current_preview in reversed(self._previews):
            if current_preview.get_preview_type() == preview:
                if self.presenter.is_accumulating():
                    ws_name = self.accumulate(current_preview.get_workspace_name(), ws_name)
                    if ws_name is None:
                        return
                    self.memory_manager.workspace_interacted_with(ws_name)
                current_preview.set_workspace_name(ws_name)
                return

        # create a new preview, since none corresponding were found
        preview_model = PreviewModel(preview, ws_name)

        # close all already existing previews
        previews = [preview for preview in self._previews]
        for preview in previews:
            preview.sig_request_close.emit()
            self.del_preview(preview)

        # add this new preview
        self._previews.append(preview_model)
        self.sig_new_preview.emit(preview_model)

    @staticmethod
    def load_file(workspace_name, filename):
        """
        Load the workspace to the workspace name.
        @param workspace_name: the name of the workspace to create.
        @param filename: the path to the file to load.
        @return whether the loading succeeded.
        """
        load_alg = AlgorithmManager.create("Load")
        load_alg.setLogging(True)
        load_alg.setProperty("Filename", filename)
        load_alg.setProperty("OutputWorkspace", workspace_name)
        load_alg.execute()
        if not load_alg.isExecuted():
            logger.error("Failed to load " + filename)
        return load_alg.isExecuted()

    def del_preview(self, previewModel):
        """
        Delete a preview model.
        @param previewModel(PreviewModel): reference to the model.
        """
        if previewModel in self._previews:
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

        if isinstance(workspace, PeaksWorkspace) or isinstance(workspace, TableWorkspace):
            return

        is_group = workspace.isGroup()
        if is_group:
            # that's probably D7 or some processed data
            if workspace.size() == 0:
                return

            workspace = workspace.getItem(0)

        instrument_name = workspace.getInstrument().getName()
        is_acquisition_type_needed = preview_finder.need_acquisition_mode(instrument_name)

        if is_acquisition_type_needed:
            acquisition_mode = self.determine_acquisition_mode(workspace)
            return preview_finder.get_preview(instrument_name, acquisition_mode, is_group=is_group)
        else:
            return preview_finder.get_preview(instrument_name, is_group=is_group)

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
