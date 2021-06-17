# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
from os import path
from qtpy.QtCore import *

from mantid.simpleapi import Load, config, mtd, Plus, Minus
from mantid.api import PreviewManager


class PreviewModel(QObject):

    """
    Type of the preview.
    """
    _type  = None

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

        self.current_workspaces = []

    def on_instrument_changed(self, new_instrument):
        """
        Slot triggered by changing the instrument
        @param new_instrument: the name of the instrument, from the instrument selector
        """
        self.instrument = new_instrument
        self.presenter.populate_acquisitions()
        # TODO emit a signal to modify both preview manager and target

    def new_preview(self, filenames, instrument, acquisition_mode, preview_name):
        """
        Add a preview to the model.
        """
        technique = config.getInstrument(instrument).techniques()[0]
        preview = PreviewManager.Instance().getPreview("ILL", technique,
                                                       acquisition_mode,
                                                       preview_name)
        if not preview:
            return

        # TODO Plus/Minus if several workspaces
        if len(filenames) != 1:
            return

        for filename in filenames:
            ws_name = path.basename(filename)[:-4]
            if not mtd.doesExist(ws_name):
                Load(Filename=filename, OutputWorkspace=ws_name)
        preview_model = PreviewModel(preview.type(), ws_name)
        self._previews.append(preview_model)
        self.sig_new_preview.emit(preview_model)

    def del_preview(self, previewModel):
        """
        Delete a preview model.
        @param preview_model(PreviewModel): reference to the model.
        """
        self._previews.remove(previewModel)

    def on_file_clicked(self, file_index):
        """
        Slot triggered by clicking on a file. If it is not loaded already, tries to do so.
        @param file_index: a QModelIndex object, referencing the position of the file in the model
        """
        file_path = self.file_model.filePath(file_index)
        if not path.isfile(file_path):
            return
        ws_name = path.basename(file_path)[:-4]

        # TODO define caching policy
        # for now, we keep everything because it makes for faster testing
        if not mtd.doesExist(ws_name):
            self.presenter.view.fileTree.setCursor(Qt.BusyCursor)
            Load(Filename=file_path, OutputWorkspace=ws_name)
            self.presenter.view.fileTree.unsetCursor()

        if self.presenter.is_accumulate_checked():

            preview_type = self.presenter.get_current_preview_type()
            ws_on_last_display = self.presenter.displays.get_last_workspaces(preview_type)

            if ws_on_last_display:
                last_ws = ws_on_last_display[-1]
                # TODO manage the case where all the workspaces have been deleted
                acc_ws_name = accumulate_name(last_ws, ws_name)
                if self.presenter.view.fileTree.selectionModel().isSelected(file_index):
                    Plus(LHSWorkspace=last_ws, RHSWorkspace=ws_name, OutputWorkspace=acc_ws_name)
                else:
                    Minus(LHSWorkspace=last_ws, RHSWorkspace=ws_name, OutputWorkspace=acc_ws_name)

                ws_name = acc_ws_name
        self.presenter.show_ws(ws_name)
        self.presenter.populate_targets()


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
