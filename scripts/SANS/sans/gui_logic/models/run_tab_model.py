# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List

from sans.common.enums import SaveType
from sans.gui_logic.models.POD.save_options import SaveOptions


class RunTabModel:
    def __init__(self):
        self._save_types = SaveOptions()

    def update_save_types(self, selected_types: List[SaveType]):
        print("Update called")
        self._save_types = SaveOptions()  # Reset all to false
        if SaveType.CAN_SAS in selected_types:
            self._save_types.can_sas_1d = True
        if SaveType.NX_CAN_SAS in selected_types:
            self._save_types.nxs_can_sas = True
        if SaveType.RKH in selected_types:
            self._save_types.rkh = True

        self._save_types.user_modified = True

    def get_save_types(self):
        return self._save_types
