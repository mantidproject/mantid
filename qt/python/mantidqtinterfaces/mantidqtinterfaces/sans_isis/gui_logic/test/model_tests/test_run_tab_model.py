# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.enums import ReductionDimensionality
from mantidqtinterfaces.sans_isis.gui_logic.models.POD.save_options import SaveOptions
from mantidqtinterfaces.sans_isis.gui_logic.models.run_tab_model import RunTabModel


class TestRunTabModel(unittest.TestCase):
    def setUp(self) -> None:
        self.model = RunTabModel()

    def test_constructor_sets_defaults(self):
        self.assertEqual(ReductionDimensionality.ONE_DIM, self.model.get_reduction_mode())
        # Represents default save opts for 1D
        self.assertEqual(SaveOptions(can_sas_1d=True, nxs_can_sas=True), self.model.get_save_types())

    def test_reduction_mode_stored(self):
        for mode in [ReductionDimensionality.TWO_DIM, ReductionDimensionality.ONE_DIM]:
            self.model.update_reduction_mode(mode)
            self.assertEqual(mode, self.model.get_reduction_mode())

    def test_reduction_mode_updates_save_opts(self):
        for mode, expected in [
            (ReductionDimensionality.TWO_DIM, SaveOptions(nxs_can_sas=True)),
            (ReductionDimensionality.ONE_DIM, SaveOptions(can_sas_1d=True, nxs_can_sas=True)),
        ]:
            self.model.update_reduction_mode(mode)
            self.assertEqual(expected, self.model.get_save_types())


if __name__ == "__main__":
    unittest.main()
