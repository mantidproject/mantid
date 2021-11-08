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

from mantid.simpleapi import Load, config, mtd
from mantid.api import PreviewType


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

    def new_preview(self, filenames, preview_name):
        """
        Add a preview to the model.
        """

        # preview = PreviewManager.Instance().getPreview("ILL", preview_name)

        preview = preview_name

        # TODO find a way to check again
        if preview is None:
            return

        # TODO Plus/Minus if several workspaces
        if len(filenames) != 1:
            return

        ws_name = ""
        for filename in filenames:
            ws_name = path.basename(filename)[:-4]
            if not mtd.doesExist(ws_name):
                Load(Filename=filename, OutputWorkspace=ws_name)

        preview = self.choose_preview(ws_name)

        if preview is None:
            return
        preview_model = PreviewModel(preview, ws_name)
        self._previews.append(preview_model)
        self.sig_new_preview.emit(preview_model)

    def modify_preview(self, filenames, preview_name):
        """
        Modify the last preview of the same type. If none is found, a new
        preview is opened.
        """
        preview = preview_name

        if preview is None:
            return

        # TODO Plus/Minus if several workspaces
        if len(filenames) != 1:
            return

        ws_name = ""
        for filename in filenames:
            ws_name = path.basename(filename)[:-4]
            if not mtd.doesExist(ws_name):
                Load(Filename=filename, OutputWorkspace=ws_name)

        for i in range(len(self._previews) - 1, -1, -1):
            if self._previews[i].get_preview_type() == preview:
                self._previews[i].set_workspace_name(ws_name)
                return

        preview = self.choose_preview(ws_name)

        if preview is None:
            return

        preview_model = PreviewModel(preview, ws_name)
        self._previews.append(preview_model)
        self.sig_new_preview.emit(preview_model)

    def del_preview(self, previewModel):
        """
        Delete a preview model.
        @param preview_model(PreviewModel): reference to the model.
        """
        self._previews.remove(previewModel)

    def choose_preview(self, ws_name):
        """
        TODO
        """
        ws = mtd[ws_name]

        # TODO find a way to know if ws is a group workspace -- stupid idea : try to get child wspaces
        # if ws.isGroupWorkspace():
        #     # that's probably D7 or some processed data
        #     print("INVALID ENTRY : GROUP WORKSPACE")
        #     return

        if ws.getNumberHistograms() > 1:
            if ws.getNumberBins() > 1:
                # plot2D
                return PreviewType.PLOT2D
            else:
                return PreviewType.PLOT1D
        else:
            return PreviewType.PLOTBINS


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
