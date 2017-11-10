from __future__ import (absolute_import, division, print_function)

import unittest
import sys

import mantid

from sans.gui_logic.presenter.beam_centre_presenter import BeamCentrePresenter
from sans.test_helper.mock_objects import (FakeParentPresenter, FakeState, create_mock_masking_table, create_run_tab_presenter_mock)
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class BeamCentrePresenterTest(unittest.TestCase):
    def test_that_can_get_state_for_index(self):
        parent_presenter = create_run_tab_presenter_mock()
        presenter = BeamCentrePresenter(parent_presenter)
        state = presenter.get_state(3)
        self.assertTrue(isinstance(state, FakeState))

if __name__ == '__main__':
    unittest.main()
