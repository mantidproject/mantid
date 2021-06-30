# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from sans.gui_logic.models.POD.save_options import SaveOptions


class RunTabModel:
    def __init__(self):
        self._save_types = SaveOptions()

    def update_save_types(self):
        print("Update called")

    def get_save_types(self):
        pass
