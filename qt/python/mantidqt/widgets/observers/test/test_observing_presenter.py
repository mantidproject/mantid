# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, print_function)

import unittest

from mantidqt.utils.testing.mocks.mock_observing import MockObservingPresenter


def with_presenter(workspaces_are_equal=True):
    def func_wrapper(func):
        def wrapper(self, *args, **kwargs):
            presenter = MockObservingPresenter(workspaces_are_equal)
            return func(self, presenter, *args, **kwargs)

        return wrapper

    return func_wrapper


class ObservingPresenterTest(unittest.TestCase):
    def assertNotCalled(self, mock):
        self.assertEqual(0, mock.call_count)

    @with_presenter()
    def test_close(self, presenter):
        mock_name = "dawn"
        presenter.close(mock_name)
        presenter.model.workspace_equals.assert_called_once_with(mock_name)
        presenter.container.close_signal.emit.assert_called_once_with()
        self.assertIsNone(presenter.ads_observer)

    @with_presenter(workspaces_are_equal=False)
    def test_not_closing_with_invalid_name(self, presenter):
        mock_name = "dawn"
        presenter.close(mock_name)
        presenter.model.workspace_equals.assert_called_once_with(mock_name)
        self.assertEqual(0, presenter.container.close_signal.emit.call_count)
        self.assertIsNotNone(presenter.ads_observer)

    @with_presenter(workspaces_are_equal=False)
    def test_force_close(self, presenter):
        presenter.force_close()
        presenter.container.close_signal.emit.assert_called_once_with()
        self.assertIsNone(presenter.ads_observer)

    @with_presenter(workspaces_are_equal=False)
    def test_replace_workspace_not_implemented(self, presenter):
        self.assertRaises(NotImplementedError, presenter.replace_workspace, "", "")

    @with_presenter()
    def test_rename_workspace(self, presenter):
        new_name = "xax"
        presenter.rename_workspace("", new_name)
        presenter.container.rename_signal.emit.assert_called_once_with(new_name)

    @with_presenter(workspaces_are_equal=False)
    def test_not_renaming_workspace_with_invalid_name(self, presenter):
        new_name = "xax"
        presenter.rename_workspace("", new_name)
        self.assertNotCalled(presenter.container.rename_signal.emit)
