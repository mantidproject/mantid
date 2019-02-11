from unittest import TestCase

from mock import Mock
from qtpy.QtWidgets import QWidget

from mantidqt.widgets.settings.presenter import SettingsPresenter


class FakeMVP(object):
    def __init__(self):
        self.view = Mock()


class MockSettingsView(object):
    def __init__(self):
        self.mock_container = Mock(QWidget)
        self.container = Mock(return_value=self.mock_container)
        self.mock_current = Mock(QWidget)
        self.current = self.mock_current
        self.general_settings = FakeMVP()


class SettingsPresenterTest(TestCase):
    def test_action_current_row_changed(self):
        mock_view = MockSettingsView()
        p = SettingsPresenter(None, view=mock_view)

        p.action_current_row_changed(0)

        mock_view.container.replaceWidget.assert_called_once_with(mock_view.mock_current,
                                                                  mock_view.general_settings.view)

        # check that the current view has been correctly reassigned
        self.assertEqual(mock_view.current, mock_view.general_settings.view)
