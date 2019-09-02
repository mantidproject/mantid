# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication

from Muon.GUI.Common.load_file_widget.view import BrowseFileWidgetView


@start_qapplication
class LoadFileWidgetViewTest(unittest.TestCase):

    def setUp(self):
        self.view = BrowseFileWidgetView()

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_view_initialized_with_empty_line_edit(self):
        self.assertEqual(self.view.get_file_edit_text(), "No data loaded")

    def test_reset_text_to_cache_resets_correctly(self):
        text = "C:\dir1\dir2\EMU00012345.nxs;C:\dir1\dir2\EMU00012345.nxs"
        self.view.set_file_edit(text)
        # User may then overwrite the text in the LineEdit, causing a signal to be sent
        # and the corresponding slot should implement reset_edit_to_cached_value()
        self.view.reset_edit_to_cached_value()
        self.assertEqual(self.view.get_file_edit_text(), text)

    def test_text_clears_from_line_edit_correctly(self):
        text = "C:\dir1\dir2\EMU00012345.nxs;C:\dir1\dir2\EMU00012345.nxs"
        self.view.set_file_edit(text)
        self.view.clear()
        self.assertEqual(self.view.get_file_edit_text(), "No data loaded")
        self.view.reset_edit_to_cached_value()
        self.assertEqual(self.view.get_file_edit_text(), "No data loaded")

    def test_text_stored_correctly_when_not_visible_in_line_edit(self):
        # This feature is currently unused
        text = "C:\dir1\dir2\EMU00012345.nxs;C:\dir1\dir2\EMU00012345.nxs"
        self.view.set_file_edit(text, store=True)
        self.assertEqual(self.view.get_file_edit_text(), text)
        self.view.reset_edit_to_cached_value()
        self.assertEqual(self.view.get_file_edit_text(), text)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
