# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from sans.gui_logic.models.POD.save_options import SaveOptions

from sans.common.enums import ReductionDimensionality


class RunTabModel:
    def __init__(self):
        self._reduction_mode: ReductionDimensionality = ReductionDimensionality.ONE_DIM
        self._save_types = SaveOptions()

        self._set_save_opts_for_reduction_mode(self._reduction_mode)

    def get_save_types(self):
        return self._save_types

    def update_save_types(self, selected_types: SaveOptions):
        self._save_types = selected_types

    def get_reduction_mode(self):
        return self._reduction_mode

    def update_reduction_mode(self, val: ReductionDimensionality):
        self._reduction_mode = val
        self._set_save_opts_for_reduction_mode(val)

    def _set_save_opts_for_reduction_mode(self, val: ReductionDimensionality):
        """
        Sets the save options for the given reduction mode depending on reduction mode
        :param val: New reduction dimensionality
        """
        if val is ReductionDimensionality.ONE_DIM:
            self._save_types = SaveOptions(can_sas_1d=True, nxs_can_sas=True)
        elif val is ReductionDimensionality.TWO_DIM:
            self._save_types = SaveOptions(nxs_can_sas=True)
        else:
            raise RuntimeError(f"Reduction Dimensionality: {val} is unknown")
