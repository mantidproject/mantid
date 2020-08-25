# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidworkbench package

import unittest
from unittest.mock import MagicMock, patch, Mock
from mantidqt.utils.qt.testing import start_qapplication
from utils.recentlyclosedscriptsmenu import RECENT_SCRIPTS_KEY, RecentlyClosedScriptsMenu

test_CONF_settings = {RECENT_SCRIPTS_KEY: ["C:/script1.py", "C:/script2.py",
                                           "C:/thisisaverylongfolder/thisisalongerfilename.py"]}


class MockCONF(object):
    def __init__(self):
        self.set = MagicMock()
        self.get = MagicMock(side_effect=lambda x: test_CONF_settings[x])
        self.has = MagicMock(side_effect=lambda x: x in test_CONF_settings)


@start_qapplication
class RecentlyClosedScriptsMenuTest(unittest.TestCase):
    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    @patch("workbench.utils.recentlyclosedscriptsmenu.RecentlyClosedScriptsMenu.repopulate_menu", new_callable=MagicMock)
    def test_repopulate_menu_is_called_on_aboutToShow_signal_being_triggered(self, _, repopulate_menu):
        mainwindow = MagicMock()
        menu = RecentlyClosedScriptsMenu(mainwindow)

        menu.aboutToShow.emit()

        repopulate_menu.assert_called_once()

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_repopulate_menu_clears_and_then_adds_to_itself(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_populate_menu_adds_a_default_if_no_scripts_found(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_populate_menu_adds_same_number_of_actions_as_script_paths(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_size_path_correctly_shortens_too_long_string(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_size_path_doesnt_shorten_short_enough_string(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_open_script_attempts_to_open_scripts_that_exists(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_open_script_doesnt_open_script_that_doesnt_exist(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_remove_script_from_settings_removes_given_path(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_load_scripts_from_settings_removes_duplicates(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_load_scripts_from_settings_returns_nothing_if_config_is_missing(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_load_scripts_loads_scripts_from_settings(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_add_script_to_settings_adds_a_script_to_settings(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_add_script_doesnt_add_duplicates(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_add_script_doesnt_add_none(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_add_script_doesnt_add_empty_string(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_add_script_doesnt_add_more_than_max_number_of_scripts(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_add_script_adds_new_scripts_to_the_first_index(self, _):
        pass

    @patch("workbench.utils.recentlyclosedscriptsmenu.CONF", new_callable=MockCONF)
    def test_add_script_adds_removes_excess_scripts_from_the_last_index(self, _):
        pass
