# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.list_selector.list_selector_view import ListSelectorView
from Muon.GUI.Common.list_selector.list_selector_presenter import ListSelectorPresenter
import unittest
from Muon.GUI.Common.mock_widget import mockQapp
from mantid.py3compat import mock


class TestListSelectorPresenter(unittest.TestCase):
    def setUp(self):
        # self.qapp = mockQapp()
        self.view = mock.MagicMock()#ListSelectorView()
        self.model = [['property_one', True, True], ['property_two', False, False]]
        self.presenter = ListSelectorPresenter(self.view, self.model)

    def test_presenter_initialised_with_correct_variables(self):
        self.assertEqual(self.presenter.model, self.model)
        self.assertEqual(self.presenter.view, self.view)

    def test_handle_filter_changed_creates_filtered_model_and_updates_view_accordingly(self):

        self.presenter.handle_filter_changed('one')

        self.view.addItems.assert_called_once_with([['property_one', True, True]])

    def test_handle_filter_changed_creates_filtered_model_and_updates_view_accordingly_for_no_filter(self):

        self.presenter.handle_filter_changed('')

        self.view.addItems.assert_called_once_with([['property_one', True, True], ['property_two', False, False]])

    def test_handle_selction_changed_updates_model_accordingly(self):
        self.presenter.handle_selection_changed('property_two', True)

        self.assertEqual(self.model[1][1], True)

    def test_get_selected_items_returns_items_names_of_selected_items(self):
        self.assertEqual(self.presenter.get_selected_items(), ['property_one'])

    def test_set_filter_type_changes_filter_type_and_updated_view(self):
        self.presenter.set_filter_type('Exclude')
        self.presenter.handle_filter_changed('one')

        self.assertEqual(self.presenter.filter_type, 'Exclude')

        self.view.addItems.assert_called_with([['property_two', False, False]])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
