# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import unittest
from unittest.mock import Mock

from mantidqt.widgets.saveprojectdialog.presenter import ProjectSaveDialogPresenter
from mantidqt.project.project import Project
from workbench.config import CONF


class SaveProjectDialogTest(unittest.TestCase):

    def setUp(self):
        self.project = Project(Mock(), Mock())
        self.project.save_as = Mock()
        self.presenter = ProjectSaveDialogPresenter(self.project, Mock())

    def test_selecting_save_altered_workspaces_only_sets_project_attribute_to_true(self):
        self.presenter.view.get_save_altered_workspaces_only.return_value = True

        self.presenter.save_as()

        self.assertTrue(self.project.save_altered_workspaces_only)

    def test_selecting_save_all_workspaces_sets_project_attribute_to_false(self):
        self.presenter.view.get_save_altered_workspaces_only.return_value = False

        self.presenter.save_as()

        self.assertFalse(self.project.save_altered_workspaces_only)

    def test_correct_setting_is_set(self):
        self.presenter.view.get_save_altered_workspaces_only.return_value = True

        self.presenter.save_as()

        self.assertEqual(CONF.get('project', 'save_altered_workspaces_only'), True)
