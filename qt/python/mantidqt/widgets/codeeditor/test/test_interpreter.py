# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.codeeditor.interpreter import PythonFileInterpreter


@start_qapplication
class PythonFileInterpreterTest(unittest.TestCase):

    def setUp(self):
        w = PythonFileInterpreter(content='x=\'this is a string\'\r\nprint(x)')
        w.execute_async()
        self.assertTrue('x' in w._presenter.model._globals_ns.keys())

        w._presenter.is_executing = False
        w._presenter.view.editor.hasSelectedText = mock.MagicMock()
        w._presenter.view.editor.hasSelectedText.return_value = True
        w._presenter.view.editor.selectedText = mock.MagicMock()
        w._presenter.view.editor.selectedText.return_value = 'print(x)'
        w._presenter.view.editor.getSelection = mock.MagicMock()
        w._presenter.view.editor.getSelection.return_value = [0, 0, 0, 0]
        w.execute_async()
        self.assertTrue('x' in w._presenter.model._globals_ns.keys())

        w._presenter.view.editor.text = mock.MagicMock()
        w._presenter.view.editor.text.return_value = 'print(x)'
        w._presenter.is_executing = False
        w._presenter.view.editor.hasSelectedText.return_value = False
        w.execute_async()
        self.assertFalse('x' in w._presenter.model._globals_ns.keys())

    def test1(self):
        pass

    def test2(self):
        pass

    def test3(self):
        pass

    def test4(self):
        pass

    def test5(self):
        pass

    def test6(self):
        pass

    def test7(self):
        pass

    def test8(self):
        pass

    def test9(self):
        pass

    def test10(self):
        pass

    def test11(self):
        pass

    def test12(self):
        pass

    def test13(self):
        pass

    def test14(self):
        pass

    def test15(self):
        pass

    def test16(self):
        pass

    def test17(self):
        pass

    def test18(self):
        pass

    def test19(self):
        pass

    def test20(self):
        pass

    def test21(self):
        pass

    def test22(self):
        pass

    def test23(self):
        pass

    def test24(self):
        pass

    def test25(self):
        pass

    def test26(self):
        pass

    def test27(self):
        pass

    def test28(self):
        pass

    def test29(self):
        pass

    def test30(self):
        pass

    def test31(self):
        pass

    def test32(self):
        pass

    def test33(self):
        pass

    def test34(self):
        pass

    def test35(self):
        pass

    def test36(self):
        pass

    def test37(self):
        pass

    def test38(self):
        pass

    def test39(self):
        pass

    def test40(self):
        pass
    # def test_construction(self):
    #     w = PythonFileInterpreter()
    #     self.assertTrue("Status: Idle", w.status.currentMessage())
    #
    # def test_empty_code_does_nothing_on_exec(self):
    #     w = PythonFileInterpreter()
    #     w._presenter.model.execute_async = mock.MagicMock()
    #     w.execute_async()
    #     w._presenter.model.execute_async.assert_not_called()
    #     self.assertTrue("Status: Idle", w.status.currentMessage())
    #
    # def test_constructor_populates_editor_with_content(self):
    #     w = PythonFileInterpreter(content='# my funky code')
    #     self.assertEqual('# my funky code', w.editor.text())
    #
    # def test_constructor_respects_filename(self):
    #     w = PythonFileInterpreter(filename='test.py')
    #     self.assertEqual('test.py', w.filename)
    #
    # def test_successful_execution(self):
    #     w = PythonFileInterpreter()
    #     w.editor.setText("x = 1 + 2")
    #     w.execute_async()
    #     self.assertTrue("Status: Idle", w.status.currentMessage())
    #
    # def test_clear_key_binding(self):
    #     test_cases = {'Ctrl+A': None, 'Shift+A': ValueError,
    #                   'Ctrl+AAA': ValueError, 'Ctrl+Shift+A': ValueError}
    #     w = PythonFileInterpreter()
    #     for key_combo, expected_result in test_cases.items():
    #         fail_msg = ("Failed on case '{}' with expected result '{}'"
    #                     "".format(key_combo, expected_result))
    #         if expected_result is ValueError:
    #             with self.assertRaises(expected_result, msg=fail_msg):
    #                 w.clear_key_binding(key_combo)
    #         else:
    #             self.assertEqual(w.clear_key_binding(key_combo), None,
    #                              msg=fail_msg)

    # def test_variables_reset(self):
    #     w = PythonFileInterpreter(content='x=\'this is a string\'\r\nprint(x)')
    #     w.execute_async()
    #     self.assertTrue('x' in w._presenter.model._globals_ns.keys())
    #
    #     w._presenter.is_executing = False
    #     w._presenter.view.editor.hasSelectedText = mock.MagicMock()
    #     w._presenter.view.editor.hasSelectedText.return_value = True
    #     w._presenter.view.editor.selectedText = mock.MagicMock()
    #     w._presenter.view.editor.selectedText.return_value = 'print(x)'
    #     w._presenter.view.editor.getSelection = mock.MagicMock()
    #     w._presenter.view.editor.getSelection.return_value = [0, 0, 0, 0]
    #     w.execute_async()
    #     self.assertTrue('x' in w._presenter.model._globals_ns.keys())
    #
    #     w._presenter.view.editor.text = mock.MagicMock()
    #     w._presenter.view.editor.text.return_value = 'print(x)'
    #     w._presenter.is_executing = False
    #     w._presenter.view.editor.hasSelectedText.return_value = False
    #     w.execute_async()
    #     self.assertFalse('x' in w._presenter.model._globals_ns.keys())

    # def test_connect_to_progress_reports_connects_sig_process_to_the_code_editor_progress(self):
    #     w = PythonFileInterpreter()
    #     w.editor = mock.MagicMock()
    #     this = mock.MagicMock()
    #
    #     w.connect_to_progress_reports(this)
    #     w.editor.progressMade.connect.assert_called_once()
    #
    # def test_disconnect_from_progress_reports_attempts_to_disconnect_sig_process_from_the_code_editor(self):
    #     w = PythonFileInterpreter()
    #     w.editor = mock.MagicMock()
    #
    #     w.disconnect_from_progress_reports()
    #     w.editor.progressMade.disconnect.assert_called_once()


if __name__ == '__main__':
    unittest.main()
