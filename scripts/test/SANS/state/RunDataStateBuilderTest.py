# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import mock

from sans.state.RunDataStateBuilder import RunDataStateBuilder
from sans.state.StateObjects.StateScale import StateScale


class RunDataStateBuilderTest(unittest.TestCase):
    def setUp(self):
        self.mock_file_information = mock.Mock()
        self.instance = RunDataStateBuilder(file_information=self.mock_file_information)

    def test_packing_state_scale(self):
        mock_state = mock.Mock(spec=StateScale)
        self.instance.pack_state_scale(state_scale=mock_state)

        mock_state.set_geometry_from_file.assert_called_with(self.mock_file_information)

    def test_pack_all_calls_all_other_methods(self):
        with mock.patch.multiple(self.instance,
                                 pack_state_scale=mock.DEFAULT) as patched_methods:

            self.instance.pack_all_states(mock.Mock())
            patched_methods['pack_state_scale'].assert_called_once()


if __name__ == '__main__':
    unittest.main()
