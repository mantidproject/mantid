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


class SaveProjectDialogTest(unittest.TestCase):

    def setUp(self):
        self.project = Project(Mock(), Mock())
        self.project.save_as = Mock()

        self.mock_conf = Mock()
        self.mock_conf.set = lambda x, y: setattr(self.mock_conf, x, y)
        self.mock_conf.get = lambda x: getattr(self.mock_conf, x)

        self.presenter = ProjectSaveDialogPresenter(project=self.project, conf=self.mock_conf, view=Mock())

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

        self.assertEqual(self.mock_conf.get('project/save_altered_workspaces_only'), True)
