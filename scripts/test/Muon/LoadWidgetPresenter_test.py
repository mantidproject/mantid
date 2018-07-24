import unittest

from Muon.GUI.ElementalAnalysis.LoadWidget.load_presenter import LoadPresenter
from Muon.GUI.ElementalAnalysis.LoadWidget.load_view import LoadView
from Muon.GUI.ElementalAnalysis.LoadWidget.load_model import LoadModel

from Muon.GUI.Common import mock_widget

try:
    from unittest import mock
except ImportError:
    import mock


class LoadPresenterTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()

        self._view = mock.create_autospec(LoadView)
        self._model = mock.create_autospec(LoadModel)
        self.presenter = LoadPresenter(self._view, self._model)

        self.presenter.view.on_button_clicked = mock.Mock()
        self.presenter.view.unreg_on_button_clicked = mock.Mock()
        self.presenter.view.on_spinbox_val_changed = mock.Mock()
        self.presenter.view.unreg_on_spinbox_val_changed = mock.Mock()
        self.presenter.view.on_spinbox_submit = mock.Mock()
        self.presenter.view.unreg_on_spinbox_submit = mock.Mock()

        self.view = self.presenter.view

    # checks if subsequent function is called on func()
    def check_second_func_called(self, func, sub_func):
        func(mock.Mock())
        assert sub_func.call_count == 1

    def test_register_button_clicked(self):
        self.check_second_func_called(
            self.presenter.register_button_clicked,
            self.view.on_button_clicked)

    def test_unregister_button_clicked(self):
        self.check_second_func_called(
            self.presenter.unregister_button_clicked,
            self.view.unreg_on_button_clicked)

    def test_register_spinbox_val_changed(self):
        self.check_second_func_called(
            self.presenter.register_spinbox_val_changed,
            self.view.on_spinbox_val_changed)

    def test_unregister_spinbox_val_changed(self):
        self.check_second_func_called(
            self.presenter.unregister_spinbox_val_changed,
            self.view.unreg_on_spinbox_val_changed)

    def test_register_spinbox_submit(self):
        self.check_second_func_called(
            self.presenter.register_spinbox_submit,
            self.view.on_spinbox_submit)

    def test_unregister_spinbox_submit(self):
        self.check_second_func_called(
            self.presenter.unregister_spinbox_submit,
            self.view.unreg_on_spinbox_submit)


if __name__ == "__main__":
    unittest.main()
