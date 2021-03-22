# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#

from qtpy.QtWidgets import QFileSystemModel
from mantid.simpleapi import Load
from qtpy.QtCore import Signal, QObject


class RawDataExplorerModel(QObject):
    """
    TODO
    """
    show_ws = Signal(str)

    def __init__(self, presenter):
        """
        Initialise the model
        :param presenter: The presenter controlling this model
        """
        super().__init__()
        self.presenter = presenter
        self.file_model = QFileSystemModel()
        self.instrument = "D16"  # TODO set current facility instrument

        self.current_workspaces = []

    def on_instrument_changed(self, new_instrument):
        """
        Slot triggered by changing the instrument
        @param new_instrument: the name of the instrument, from the instrument selector
        """
        self.instrument = new_instrument
        # TODO emit a signal to modify both preview manager and target

    def on_file_clicked(self, file_index):
        """
        Slot triggered by clicking on a file. If it is not loaded already, tries to do so.
        @param file_index: a QModelIndex object, referencing the position of the file in the model
        """
        file_path = self.file_model.filePath(file_index)
        ws_name = "__" + file_path
        if ws_name not in self.current_workspaces:
            Load(Filename=file_path, OutputWorkspace=ws_name)
            self.current_workspaces.append(ws_name)
        self.show_ws.emit(ws_name)

