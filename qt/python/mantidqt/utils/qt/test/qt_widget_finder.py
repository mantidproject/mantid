# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from qtpy.QtWidgets import QApplication


class QtWidgetFinder(object):
    """
    This class provides common functions for finding widgets within all currently existing ones.
    """

    def find_qt_widget(self, name):
        a = QApplication.topLevelWidgets()
        all = [x for x in a if name.lower() in str(type(x)).lower()]

        self.assertEqual(0, len(all),
                         "Widgets with name '{}' are present in the QApplication. Something has not been deleted: {}".format(
                             name, all))

    def assert_window_created(self):
        self.assertGreater(QApplication.topLevelWidgets(), 0)

    def assert_no_widgets(self):
        a = QApplication.topLevelWidgets()
        self.assertEqual(len(a), 0, "Widgets are present in the QApplication: {}".format(a))
