# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from sans.gui_logic.models.POD.save_options import SaveOptions

from scripts.SANS.sans.common.enums import ReductionDimensionality


class RunTabModel:
    def __init__(self):
        self._reduction_mode: ReductionDimensionality = ReductionDimensionality.ONE_DIM
        self._save_types = SaveOptions()

    def get_save_types(self):
        return self._save_types

    def update_save_types(self, selected_types: SaveOptions):
        self._save_types = selected_types
        self._save_types.user_modified = True

    def get_reduction_mode(self):
        return self._reduction_mode

    def update_reduction_mode(self, val: ReductionDimensionality):
        self._reduction_mode = val
