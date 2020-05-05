# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock

from sans.state.StateRunDataBuilder import StateRunDataBuilder
from sans.state.StateObjects.StateScale import StateScale


class StateRunDataBuilderTest(unittest.TestCase):
    def setUp(self):
        self.mock_file_information = mock.Mock()
        self.instance = StateRunDataBuilder(file_information=self.mock_file_information)

    def test_pack_all_calls_all_other_methods(self):
        mocked_all_states = mock.Mock()
        mock_state = mock.Mock(spec=StateScale)
        mocked_all_states.scale = mock_state

        self.instance.pack_all_states(all_states=mocked_all_states)
        self.assertTrue(mock_state.set_geometry_from_file.called)


if __name__ == '__main__':
    unittest.main()
