import unittest

from mantidqt.utils.qt.testing import GuiTest

from Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView


class LoadRunWidgetViewTest(GuiTest):

    def setUp(self):
        self.view = LoadRunWidgetView()
        self.view.run_edit.insert('')

    def test_run_number_line_edit_doesnt_accept_alphabetical(self):
        self.view.run_edit.insert('h')
        result = self.view.run_edit.text()

        self.assertEqual(result, '')

    def test_run_number_line_edit_accepts_simple_number(self):
        self.view.run_edit.insert('10000')
        result = self.view.run_edit.text()

        self.assertEqual(result, '10000')

    def test_run_number_line_edit_accepts_dash_list(self):
        self.view.run_edit.insert('10000-10')
        result = self.view.run_edit.text()

        self.assertEqual(result, '10000-10')

    def test_run_number_line_edit_accepts_comma_list(self):
        self.view.run_edit.insert('10000,10001')
        result = self.view.run_edit.text()

        self.assertEqual(result, '10000,10001')

    def test_run_number_line_edit_accepts_comma_list_with_spaces(self):
        self.view.run_edit.insert('10000  , 10001')
        result = self.view.run_edit.text()

        self.assertEqual(result, '10000  , 10001')

    def test_run_number_line_edit_more_than_2_comma_entries(self):
        self.view.run_edit.insert('10000,10001,10002')
        result = self.view.run_edit.text()

        self.assertEqual(result, '10000,10001,10002')

    def test_run_number_line_edit_accepts_mix_of_ranges_and_commas(self):
        self.view.run_edit.insert('10000,10001-10002')
        result = self.view.run_edit.text()

        self.assertEqual(result, '10000,10001-10002')

    def test_run_number_line_edit_does_not_accept_other_special_characters(self):
        self.view.run_edit.insert('+')
        self.view.run_edit.insert('=')
        self.view.run_edit.insert('<')
        self.view.run_edit.insert('>')
        result = self.view.run_edit.text()

        self.assertEqual(result, '')

    def test_mixture_of_numbers_and_letters_fails(self):
        self.view.run_edit.insert('22-3a')
        result = self.view.run_edit.text()

        self.assertEqual(result, '')


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
