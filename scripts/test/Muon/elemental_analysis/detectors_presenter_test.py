from __future__ import (absolute_import, division, unicode_literals)

import unittest

from mantid.py3compat import mock

from Muon.GUI.ElementalAnalysis.Detectors.detectors_presenter import DetectorsPresenter

class DetectorsPresenterTest(unittest.TestCase):
    def setUp(self):
        self.mock_view = mock.Mock()
        self.mock_view.widgets = {'name1': 'GE1', 'name2': 'GE2', 'name3': 'GE3'}
        self.detector_presenter = DetectorsPresenter(self.mock_view)

    def test_that_all_view_widgets_are_appended_to_detector_list(self):
        self.assertEqual(self.detector_presenter.detectors, list(self.mock_view.widgets.values()))

    def test_that_setStateQuietly_calls_view_setStateQuietly(self):
        name = 'name'
        state = 3
        self.detector_presenter.setStateQuietly(name, state)

        self.mock_view.setStateQuietly.assert_called_with(name, state)

    def test_that_get_names_gives_correct_list_of_names(self):
        self.assertEqual(self.detector_presenter.getNames(), self.mock_view.widgets.keys())


if __name__ == '__main':
    unittest.main()