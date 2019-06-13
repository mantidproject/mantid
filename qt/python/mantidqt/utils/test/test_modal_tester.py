# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

import unittest
from qtpy.QtWidgets import QInputDialog

from mantid.py3compat.mock import patch
from mantidqt.utils.qt.testing import GuiTest, ModalTester


class TestModalTester(GuiTest):

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
        self.assertEqual(tester.widget, None)
        self.assertFalse(tester.passed)


if __name__ == '__main__':
    unittest.main()
