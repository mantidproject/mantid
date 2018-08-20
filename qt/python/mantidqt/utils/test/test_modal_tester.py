#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

from mock import patch
import unittest

from qtpy.QtWidgets import QInputDialog

from mantidqt.utils.qt.test import requires_qapp, ModalTester


@requires_qapp
class TestModalTester(unittest.TestCase):

    def test_pass_widget_closed(self):
        def testing_function(widget):
            widget.setTextValue('Hello World!')
            text = widget.textValue()
            self.assertEqual(text, 'Hello World!')
            widget.accept()
        tester = ModalTester(QInputDialog, testing_function)
        tester.start()
        self.assertFalse(tester.widget.isVisible())
        self.assertTrue(tester.passed)

    def test_fail_widget_closed(self):
        def testing_function(widget):
            widget.setTextValue('Hello World!')
            text = widget.textValue()
            self.assertEqual(text, 'STUFF')
            widget.accept()
        with patch('traceback.print_exc') as print_exc:
            tester = ModalTester(QInputDialog, testing_function)
            tester.start()
            print_exc.assert_called_once_with()
        self.assertFalse(tester.widget.isVisible())
        self.assertFalse(tester.passed)

    def test_pass_widget_open(self):
        def testing_function(widget):
            widget.setTextValue('Hello World!')
            text = widget.textValue()
            self.assertEqual(text, 'Hello World!')
        tester = ModalTester(QInputDialog, testing_function)
        tester.start()
        self.assertFalse(tester.widget.isVisible())
        self.assertTrue(tester.passed)

    def test_fail_widget_open(self):
        def testing_function(widget):
            widget.setTextValue('Hello World!')
            text = widget.textValue()
            self.assertEqual(text, '')
        with patch('traceback.print_exc') as print_exc:
            tester = ModalTester(QInputDialog, testing_function)
            tester.start()
            print_exc.assert_called_once_with()
        self.assertFalse(tester.widget.isVisible())
        self.assertFalse(tester.passed)

    def test_exception(self):
        def testing_function(widget):
            raise RuntimeError('')
        with patch('traceback.print_exc') as print_exc:
            tester = ModalTester(QInputDialog, testing_function)
            tester.start()
            print_exc.assert_called_once_with()
        self.assertFalse(tester.widget.isVisible())
        self.assertFalse(tester.passed)

    def test_exception_in_creator(self):
        def create():
            raise RuntimeError('')

        def testing_function(widget):
            pass
        with patch('traceback.print_exc') as print_exc:
            tester = ModalTester(create, testing_function)
            tester.start()
            print_exc.assert_called_once_with()
        self.assertTrue(tester.widget is None)
        self.assertFalse(tester.passed)


if __name__ == '__main__':
    unittest.main()
