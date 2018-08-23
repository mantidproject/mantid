from Muon.GUI.MuonAnalysis.loadfile.load_file_view import BrowseFileWidgetView

import unittest

from qtpy.QtWidgets import QApplication



class LoadFileWidgetViewTest(unittest.TestCase):

    def test_view_initialized_with_empty_line_edit(self):
        view = BrowseFileWidgetView()
        self.assertEqual(view.get_file_edit_text(), "")

    def test_reset_text_to_cache_resets_correctly(self):
        text = "C:\dir1\dir2\EMU00012345.nxs;C:\dir1\dir2\EMU00012345.nxs"
        view = BrowseFileWidgetView()
        view.set_file_edit(text)
        # User may then overwrite the text in the LineEdit, causing a signal to be sent
        # and the corresponding slot should implement reset_edit_to_cached_value()
        view.reset_edit_to_cached_value()
        self.assertEqual(view.get_file_edit_text(), text)

    def test_text_clears_correctly(self):
        text = "C:\dir1\dir2\EMU00012345.nxs;C:\dir1\dir2\EMU00012345.nxs"
        view = BrowseFileWidgetView()
        view.set_file_edit(text)
        view.clear()
        self.assertEqual(view.get_file_edit_text(), "")
        view.reset_edit_to_cached_value()
        self.assertEqual(view.get_file_edit_text(), "")

    def test_text_stored_correctly_when_not_visible(self):
        text = "C:\dir1\dir2\EMU00012345.nxs;C:\dir1\dir2\EMU00012345.nxs"
        view = BrowseFileWidgetView()
        view.set_file_edit(text, store=True)
        self.assertEqual(view.get_file_edit_text(), text)
        view.reset_edit_to_cached_value()
        self.assertEqual(view.get_file_edit_text(), text)


if __name__ == '__main__':
    QT_APP = QApplication([])
    unittest.main(buffer=False, verbosity=2)
    QT_APP.exec_()
