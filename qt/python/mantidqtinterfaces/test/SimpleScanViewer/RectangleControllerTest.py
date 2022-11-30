# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
import sys

from qtpy.QtWidgets import QApplication
from matplotlib.widgets import Rectangle

from mantid.simpleapi import config
from mantid.api import mtd

from mantidqtinterfaces.simplescanviewer.rectangle_controller import RectangleController, RectanglesManager

app = QApplication(sys.argv)


class RectangleManagerTest(unittest.TestCase):

    def setUp(self) -> None:
        self.facility = config['default.facility']
        self.instrument = config['default.instrument']
        config['default.facility'] = "ILL"
        config['default.instrument'] = "D16"

        self.manager = RectanglesManager()

    def tearDown(self) -> None:
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument
        mtd.clear()

    def test_add_rectangle(self):
        rect1 = Rectangle((0, 0), 1, 1)
        rect2 = Rectangle((1, 1), 2, 2)

        self.manager.add_rectangle(rect1)
        self.assertEqual(len(self.manager.rectangles), 1)
        self.assertEqual(self.manager.current_rectangle_index, 0)
        self.assertEqual(self.manager.table.rowCount(), 5)

        self.manager.add_rectangle(rect2)
        self.assertEqual(len(self.manager.rectangles), 2)
        self.assertEqual(self.manager.current_rectangle_index, 1)
        self.assertEqual(self.manager.table.rowCount(), 10)

    def test_remove_rectangle(self):
        rect = mock.Mock()
        rect.get_xy.return_value = (0, 0)
        rect.get_width.return_value = 1
        rect.get_height.return_value = 1
        self.manager.add_rectangle(rect)

        self.manager.delete_current()
        self.assertEqual(len(self.manager.get_rectangles()), 0)
        self.assertEqual(self.manager.current_rectangle_index, -1)
        self.assertEqual(self.manager.table.rowCount(), 0)
        rect.remove.assert_called_once()

    def test_set_as_current(self):
        rect1 = Rectangle((0, 0), 1, 1)
        rect2 = Rectangle((1, 1), 2, 2)

        self.manager.add_rectangle(rect1)
        self.manager.add_rectangle(rect2)

        self.manager.set_as_current_rectangle(rect1)
        self.assertEqual(self.manager.current_rectangle_index, 0)
        self.assertEqual(self.manager.get_current_rectangle(), rect1)

    def test_clear(self):
        rect1 = mock.Mock()
        rect1.get_xy.return_value = (0, 0)
        rect1.get_width.return_value = 1
        rect1.get_height.return_value = 1

        rect2 = mock.Mock()
        rect2.get_xy.return_value = (0, 0)
        rect2.get_width.return_value = 1
        rect2.get_height.return_value = 1

        self.manager.add_rectangle(rect1)
        self.manager.add_rectangle(rect2)

        self.manager.clear()
        self.assertEqual(self.manager.current_rectangle_index, -1)
        self.assertEqual(len(self.manager.get_rectangles()), 0)
        self.assertEqual(self.manager.table.rowCount(), 0)
        rect1.remove.assert_called_once()
        rect2.remove.assert_called_once()

    def test_find(self):
        rect1 = Rectangle((0, 0), 1, 1)
        rect2 = Rectangle((1, 1), 2, 2)

        self.manager.add_rectangle(rect1)
        self.manager.add_rectangle(rect2)

        self.assertEqual(self.manager.find_controller(0, 0, 1, 1), 0)
        self.assertEqual(self.manager.find_controller(1, 1, 3, 3), 1)
        self.assertEqual(self.manager.find_controller(1, 2, 3, 4), -1)

    def test_field_update(self):
        rect1 = Rectangle((0, 0), 1, 1)
        rect2 = Rectangle((1, 1), 2, 2)

        self.manager.add_rectangle(rect1)
        self.manager.add_rectangle(rect2)

        trigger_check = mock.Mock()
        self.manager.sig_controller_updated.connect(trigger_check)

        self.manager.table.item(1, 1).setText("0.123")  # set x0 of the first rectangle

        self.assertEqual(self.manager.get_rectangles()[0].get_xy()[0], .123)
        self.assertEqual(self.manager.get_rectangles()[0].get_width(), 0.877)
        self.assertEqual(self.manager.current_rectangle_index, 1)  # the current rectangle is still the second one
        trigger_check.assert_called_once()

        trigger_check.reset_mock()

        self.manager.table.item(1, 1).setText("azerty")

        # check the value revert to the previous one when the input is ill-formed
        self.assertEqual(self.manager.get_rectangles()[0].get_xy()[0], 0.123)
        self.assertEqual(trigger_check.call_count, 2)

    def test_insert_in(self):
        controller = RectangleController(1, 2.1, 3.21, 4.321)
        controller.insert_in(self.manager.table)

        self.assertEqual(self.manager.table.rowCount(), 5)
        self.assertEqual(self.manager.table.columnCount(), 2)
        self.assertEqual(self.manager.table.item(0, 0).text(), "Parameter")
        self.assertEqual(self.manager.table.item(0, 1).text(), "Value")
        self.assertEqual(self.manager.table.item(1, 0).text(), "x0")
        self.assertEqual(self.manager.table.item(2, 0).text(), "y0")
        self.assertEqual(self.manager.table.item(3, 0).text(), "x1")
        self.assertEqual(self.manager.table.item(4, 0).text(), "y1")

        self.assertEqual(self.manager.table.item(1, 1).text(), " 1.00000")
        self.assertEqual(self.manager.table.item(2, 1).text(), " 2.10000")
        self.assertEqual(self.manager.table.item(3, 1).text(), " 3.21000")
        self.assertEqual(self.manager.table.item(4, 1).text(), " 4.32100")

    def test_remove_from(self):
        controller1 = RectangleController(1, 2, 3, 4)
        controller2 = RectangleController(6, 7, 8, 9)

        controller1.insert_in(self.manager.table)
        controller2.insert_in(self.manager.table)

        self.assertEqual(self.manager.table.rowCount(), 10)

        controller1.remove_from(self.manager.table)

        self.assertEqual(self.manager.table.rowCount(), 5)
        self.assertEqual(self.manager.table.item(0, 0).text(), "Parameter")

        self.assertEqual(self.manager.table.item(1, 1).text(), " 6.00000")
        self.assertEqual(self.manager.table.item(2, 1).text(), " 7.00000")
        self.assertEqual(self.manager.table.item(3, 1).text(), " 8.00000")
        self.assertEqual(self.manager.table.item(4, 1).text(), " 9.00000")

    def test_update_values(self):
        controller = RectangleController(1, 2, 3, 4)

        controller.insert_in(self.manager.table)

        controller.update_values(6, 7, 8, 9)

        self.assertEqual(self.manager.table.item(1, 1).text(), " 6.00000")
        self.assertEqual(self.manager.table.item(2, 1).text(), " 7.00000")
        self.assertEqual(self.manager.table.item(3, 1).text(), " 8.00000")
        self.assertEqual(self.manager.table.item(4, 1).text(), " 9.00000")

        self.assertEqual(tuple(controller.get_values()), (6, 7, 8, 9))


if __name__ == "__main__":
    unittest.main()
