# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidworkbench package
import os
import shutil
import tempfile
from unittest import TestCase
from unittest.mock import MagicMock, patch
from qtpy.QtWidgets import QAction
from qtpy.QtCore import QObject
from mantidqt.utils.qt.testing import start_qapplication
from workbench.utils.recentlyclosedscriptsmenu import RECENT_SCRIPTS_KEY, RecentlyClosedScriptsMenu, RECENT_SCRIPT_MAX_NUMBER

test_CONF_settings = {RECENT_SCRIPTS_KEY: ["C:/script1.py", "C:/script2.py", "C:/thisisaverylongfolder/thisisalongerfilename.py"]}
working_directory = tempfile.mkdtemp()
fake_script = os.path.join(working_directory, "fake_script.py")


class MockCONF(object):
    def __init__(self):
        self.set = MagicMock()
        self.get = MagicMock(side_effect=lambda x: test_CONF_settings[x])
        self.has = MagicMock(side_effect=lambda x: x in test_CONF_settings)


class FakeQObject(QObject):
    def __init__(self):
        super().__init__()


class MockQObject(QObject):
    def __init__(self):
        super().__init__()
        self.editor = MagicMock()


@start_qapplication
class RecentlyClosedScriptsMenuTest(TestCase):
    def tearDown(self):
        if os.path.isdir(working_directory):
            shutil.rmtree(working_directory)

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    @patch.object(RecentlyClosedScriptsMenu, "repopulate_menu", return_value=None)
    def test_repopulate_menu_is_called_on_aboutToShow_signal_being_triggered(self, repopulate_menu, _):
        menu = RecentlyClosedScriptsMenu(None)

        menu.aboutToShow.emit()

        repopulate_menu.assert_called_once()

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    @patch.object(RecentlyClosedScriptsMenu, "populate_menu", return_value=None)
    def test_repopulate_menu_clears_and_then_populates(self, populate_menu, _):
        fake_object = FakeQObject()
        menu = RecentlyClosedScriptsMenu(None)
        menu.addAction(QAction(fake_object))

        self.assertEqual(len(menu.actions()), 1)

        menu.repopulate_menu()

        self.assertEqual(len(menu.actions()), 0)
        populate_menu.assert_called_once()

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    @patch.object(RecentlyClosedScriptsMenu, "_get_scripts_from_settings", return_value=[])
    def test_populate_menu_adds_a_default_if_no_scripts_found(self, _, __):
        fake_object = FakeQObject()
        menu = RecentlyClosedScriptsMenu(fake_object)

        self.assertEqual(len(menu.actions()), 0)

        menu.populate_menu()

        self.assertEqual(len(menu.actions()), 1)
        self.assertEqual(menu.actions()[0].text(), "No recently closed scripts found")

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    @patch.object(RecentlyClosedScriptsMenu, "_get_scripts_from_settings", return_value=["1", "2", "3"])
    def test_populate_menu_adds_same_number_of_actions_as_script_paths(self, _, __):
        fake_object = FakeQObject()
        menu = RecentlyClosedScriptsMenu(fake_object)

        self.assertEqual(len(menu.actions()), 0)
        menu.populate_menu()

        self.assertEqual(len(menu.actions()), 3)
        self.assertEqual(menu.actions()[0].text(), "1")
        self.assertEqual(menu.actions()[1].text(), "2")
        self.assertEqual(menu.actions()[2].text(), "3")

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_size_path_correctly_shortens_too_long_string(self, _):
        menu = RecentlyClosedScriptsMenu(None)
        path = "12345678912345678912345678912345678912845176451762534716412786459176549671549673"

        self.assertEqual("...534716412786459176549671549673", menu.size_path_correctly(path))

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_size_path_doesnt_shorten_short_enough_string(self, _):
        menu = RecentlyClosedScriptsMenu(None)
        path = "12345678910"

        self.assertEqual(path, menu.size_path_correctly(path))

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_open_script_attempts_to_open_scripts_that_exists(self, _):
        mock_mainwindow = MockQObject()
        menu = RecentlyClosedScriptsMenu(mock_mainwindow)
        os.mkdir(working_directory)
        open(fake_script, "w+").close()

        menu.open_script(fake_script)

        mock_mainwindow.editor.open_file_in_new_tab.assert_called_once_with(fake_script)

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    @patch.object(RecentlyClosedScriptsMenu, "remove_script_from_settings", return_value=None)
    @patch("workbench.utils.recentlyclosedscriptsmenu.QMessageBox.warning")
    def test_open_script_doesnt_open_script_that_doesnt_exist(self, warning, remove_script_from_settings, __):
        mock_mainwindow = MockQObject()
        menu = RecentlyClosedScriptsMenu(mock_mainwindow)

        menu.open_script(fake_script)

        mock_mainwindow.editor.open_file_in_new_tab.assert_not_called()
        remove_script_from_settings.assert_called_once_with(fake_script)
        warning.assert_called_once()

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF.set")
    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF.get", return_value=[(1, "C:/script1.py"), (2, "C:/script2.py")])
    def test_remove_script_from_settings_removes_given_path(self, get, set):
        menu = RecentlyClosedScriptsMenu(None)

        menu.remove_script_from_settings("C:/script1.py")

        get.assert_called_once_with(RECENT_SCRIPTS_KEY)
        set.assert_called_once_with(RECENT_SCRIPTS_KEY, [(0, "C:/script2.py")])

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF.get", side_effect=KeyError)
    def test_load_scripts_from_settings_returns_nothing_if_config_is_missing(self, get):
        menu = RecentlyClosedScriptsMenu(None)

        self.assertEqual([], menu._get_scripts_from_settings())
        get.assert_called_once_with(RECENT_SCRIPTS_KEY)

    @patch(
        "workbench.utils.recentlyclosedscriptsmenu.CONF.get",
        return_value=[(1, "C:/script1.py"), (2, "C:/script2.py"), (3, "C:/script3.py")],
    )
    def test_load_scripts_loads_scripts_from_settings(self, get):
        menu = RecentlyClosedScriptsMenu(None)

        load = menu._get_scripts_from_settings()

        get.assert_called_once_with(RECENT_SCRIPTS_KEY)
        self.assertEqual(3, len(load))
        self.assertTrue("C:/script1.py" in load)
        self.assertTrue("C:/script2.py" in load)
        self.assertTrue("C:/script3.py" in load)

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF.set")
    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF.get", return_value=[(0, "C:/script1.py"), (1, "C:/script2.py")])
    def test_add_script_to_settings_adds_a_script_to_settings(self, get, set):
        menu = RecentlyClosedScriptsMenu(None)

        menu.add_script_to_settings("C:/script3.py")

        get.assert_called_once_with(RECENT_SCRIPTS_KEY)
        set.assert_called_once_with(RECENT_SCRIPTS_KEY, [(0, "C:/script3.py"), (1, "C:/script1.py"), (2, "C:/script2.py")])

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF.set")
    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF.get", return_value=[(1, "C:/script1.py"), (2, "C:/script2.py")])
    def test_add_script_doesnt_add_duplicates(self, get, set):
        menu = RecentlyClosedScriptsMenu(None)

        menu.add_script_to_settings("C:/script1.py")

        get.assert_called_once_with(RECENT_SCRIPTS_KEY)
        set.assert_not_called()

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF.set")
    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF.get", return_value=None)
    def test_add_script_doesnt_add_none(self, get, set):
        menu = RecentlyClosedScriptsMenu(None)

        menu.add_script_to_settings(None)

        get.assert_not_called()
        set.assert_not_called()

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF.set")
    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF.get", return_value=None)
    def test_add_script_doesnt_add_empty_string(self, get, set):
        menu = RecentlyClosedScriptsMenu(None)

        menu.add_script_to_settings("")

        get.assert_not_called()
        set.assert_not_called()

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF.set")
    @patch(
        "workbench.utils.recentlyclosedscriptsmenu.CONF.get",
        return_value=[(1, "1"), (2, "2"), (3, "3"), (4, "4"), (5, "5"), (6, "6"), (7, "7"), (8, "8"), (9, "9"), (10, "10")],
    )
    def test_add_script_doesnt_add_more_than_max_number_of_scripts(self, get, set):
        menu = RecentlyClosedScriptsMenu(None)

        menu.add_script_to_settings("11")

        get.assert_called_once_with(RECENT_SCRIPTS_KEY)
        self.assertEqual(RECENT_SCRIPT_MAX_NUMBER, len(set.call_args[0][1]))
        set.assert_called_once_with(
            RECENT_SCRIPTS_KEY, [(0, "11"), (1, "1"), (2, "2"), (3, "3"), (4, "4"), (5, "5"), (6, "6"), (7, "7"), (8, "8"), (9, "9")]
        )

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF.set")
    @patch(
        "workbench.utils.recentlyclosedscriptsmenu.CONF.get",
        return_value=[(1, "1"), (2, "2"), (3, "3"), (4, "4"), (5, "5"), (6, "6"), (7, "7"), (8, "8"), (9, "9"), (10, "10")],
    )
    def test_add_script_adds_new_scripts_to_the_first_index_and_removes_excess_scripts_from_the_last_index(self, get, set):
        menu = RecentlyClosedScriptsMenu(None)

        menu.add_script_to_settings("11")

        get.assert_called_once_with(RECENT_SCRIPTS_KEY)
        set.assert_called_once_with(
            RECENT_SCRIPTS_KEY, [(0, "11"), (1, "1"), (2, "2"), (3, "3"), (4, "4"), (5, "5"), (6, "6"), (7, "7"), (8, "8"), (9, "9")]
        )
