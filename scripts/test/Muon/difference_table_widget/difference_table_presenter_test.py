# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QWidget

from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from Muon.GUI.Common.difference_table_widget.difference_widget_presenter import DifferencePresenter
from Muon.GUI.Common.difference_table_widget.difference_widget_view import DifferenceView
from mantidqt.utils.observer_pattern import Observer
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests

@start_qapplication
class DifferenceTablePresenterTest(unittest.TestCase):

    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QWidget()

        setup_context_for_tests(self)

        self.gui_variable_observer = Observer()

        self.gui_context.gui_variables_notifier.add_subscriber(self.gui_variable_observer)
        self.model = GroupingTabModel(context=self.context)
        self.view = DifferenceView(parent=self.obj)
        self.presenter = DifferencePresenter(self.view, self.model)

    def tearDown(self):
        self.obj = None

    def assert_model_empty(self):
        self.assertEqual(0, len(self.model.diff_names))
        self.assertEqual(0, len(self.model.diffs))

    def assert_view_empty(self):
        self.assertEqual(0, self.view.num_rows())

