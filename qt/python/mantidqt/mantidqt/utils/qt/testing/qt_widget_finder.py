# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from qtpy.QtWidgets import QApplication


class QtWidgetFinder(object):
    """
    This class provides common functions for finding widgets within all currently existing ones.
    """

    def find_qt_toplevel_widget(self, name):
        all = QApplication.topLevelWidgets()
        return [x for x in all if name.lower() in str(type(x)).lower()]

    def find_qt_widget_by_name(self, name):
        a = QApplication.allWidgets()
        return [x for x in a if name.lower() in str(type(x)).lower()]

    def assert_widget_exists(self, name, expected_count=None):
        all = self.find_qt_widget_by_name(name)
        if not expected_count:
            self.assertGreater(0, len(all))
        else:
            self.assertEqual(expected_count, len(all))

    def assert_widget_not_present(self, name):
        all = self.find_qt_widget_by_name(name)

        self.assertEqual(
            0, len(all), "Widgets with name '{}' are present in the QApplication. Something has not been deleted: {}".format(name, all)
        )

    def assert_widget_created(self):
        self.assertGreater(len(QApplication.topLevelWidgets()), 0)

    def assert_no_toplevel_widgets(self):
        a = QApplication.topLevelWidgets()
        self.assertEqual(len(a), 0, "Widgets are present in the QApplication: {}".format(a))

    def assert_number_of_widgets_matching(self, name, number):
        all = self.find_qt_widget_by_name(name)
        self.assertEqual(number, len(all), "Widgets are present in the QApplication: {}".format(all))

    def find_widgets_of_type(self, type_name):
        top_widgets = QApplication.topLevelWidgets()
        return [x for x in top_widgets if type_name.lower() in str(type(x)).lower()]

    def assert_widget_type_exists(self, type_name):
        widget_list = self.find_widgets_of_type(type_name)
        self.assertLess(0, len(widget_list), msg="No widgets of type '{}' found in QApplication.".format(type_name))

    def assert_widget_type_doesnt_exist(self, type_name):
        widget_list = self.find_widgets_of_type(type_name)
        self.assertEqual(0, len(widget_list), msg="{} widgets of type '{}' found in QApplication.".format(len(widget_list), type_name))
