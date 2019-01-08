from mock import Mock

from mantidqt.widgets.matrixworkspacedisplay.test_helpers.mock_matrixworkspacedisplay import MockQTableView


class MockTableWorkspaceDisplayView:
    def __init__(self):
        self.set_context_menu_actions = Mock()
        self.table_x = MockQTableView()
        self.table_y = MockQTableView()
        self.table_e = MockQTableView()
        self.set_model = Mock()
        self.copy_to_clipboard = Mock()
        self.show_mouse_toast = Mock()
        self.ask_confirmation = None


class MockTableWorkspaceDisplayModel:
    def __init__(self):
        self.get_spectrum_plot_label = Mock()
        self.get_bin_plot_label = Mock()
