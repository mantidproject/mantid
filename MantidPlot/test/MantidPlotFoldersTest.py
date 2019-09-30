# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Test creating, switching and deleting
folders.
"""
import mantidplottests
from mantidplottests import *
from PyQt4 import QtCore

class MantidPlotFoldersTest(unittest.TestCase):

    def setUp(self):
        # Make sure NOT to ask to close a folder. This hangs up tests
        import _qti
        _qti.app.setConfirmFolderClose(False)

    def tearDown(self):
        pass

    # FIXME: Avoid folder deletion confirmation dialog
    def setup_folder(self, name):
        """ Create a folder with some windows in it """
        # Go to root
        changeFolder(rootFolder())
        # Add at root level
        f = addFolder(name)
        changeFolder(f)
        windows = []
        windows.append(newTable("table-"+name))
        windows.append(newMatrix("matrix-"+name))
        windows.append(newGraph("graph-"+name))
        windows.append(newNote("note-"+name))
        for w in windows:
            w.confirmClose(False)
        return (f, windows)

    def test_Folder_deletion(self):
        """ Create a folder then delete it """
        f = addFolder("test_folder")
        deleteFolder(f)
        self.assertEqual(f._getHeldObject(), None, "Folder was deleted")

    def test_Folder_accessing_windows(self):
        """ Access windows through a folder """
        f, old_windows = self.setup_folder("fld")
        windows = f.windows()
        self.assertEqual(len(windows), 4, "4 windows in folder")
        self.assertNotEqual( f.window("table-fld"), None)
        self.assertNotEqual( f.table("table-fld"), None)
        self.assertNotEqual( f.matrix("matrix-fld"), None)
        self.assertNotEqual( f.graph("graph-fld"), None)
        for w in windows: w.close()
        deleteFolder(f)

    def test_Folder_deletinging_windows(self):
        """ Access windows through a folder """
        f, windows = self.setup_folder("fld")
        n = 4
        for w in windows:
            w.close()
            n -= 1
            self.assertEqual(len(f.windows()), n)
        deleteFolder(f)

    def test_switchFolder_windows(self):
        """ Access windows through a folder """
        f1, windows1 = self.setup_folder("folder1")
        f2, windows2 = self.setup_folder("folder2")
        for w in windows1:
            self.assertFalse( w.isVisible(), "Window in closed folder is hidden." )
        for w in windows2:
            self.assertTrue( w.isVisible(), "Window in active folder is visible." )
        # Go back to folder 1
        changeFolder(f1)
        for w in windows2:
            self.assertFalse( w.isVisible(), "Window in closed folder is hidden." )
        for w in windows1:
            self.assertTrue( w.isVisible(), "Window in active folder is visible." )


# Run the unit tests
mantidplottests.runTests(MantidPlotFoldersTest)
