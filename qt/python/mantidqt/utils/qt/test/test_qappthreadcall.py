# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import unittest

from qtpy.QtWidgets import QApplication

from mantidqt.utils.asynchronous import AsyncTask, TaskExitCode
from mantidqt.utils.qt.qappthreadcall import QAppThreadCall, force_method_calls_to_qapp_thread
from mantidqt.utils.qt.testing import start_qapplication


class CustomException(Exception):
    pass


@start_qapplication
class QAppThreadCallTest(unittest.TestCase):
    def test_correct_exception_is_raised_when_called_on_main_thread(self):
        qappthread_wrap = QAppThreadCall(self.raises_exception)

        self.assertRaises(CustomException, qappthread_wrap)

    def test_correct_exception_is_raised_when_called_on_other_thread(self):
        self.exc = None
        self.exit_code = None

        def collect_error(e):
            self.exc = e

        thread = AsyncTask(self.task, error_cb=collect_error)
        thread.start()
        while thread.is_alive():
            QApplication.sendPostedEvents()
        thread.join(0.5)
        self.assertTrue(isinstance(self.exc.exc_value, CustomException))
        self.assertEqual(TaskExitCode.ERROR, thread.exit_code)

    def raises_exception(self):
        raise CustomException()

    def task(self):
        qappthread_wrap = QAppThreadCall(self.raises_exception)
        qappthread_wrap()

    def test_force_method_calls_to_qapp_thread(self):
        class Impl:
            def public(self):
                pass

            def _private(self):
                pass

        public_only = force_method_calls_to_qapp_thread(Impl())
        self.assertTrue(isinstance(public_only.public, QAppThreadCall))
        self.assertFalse(isinstance(public_only._private, QAppThreadCall))

        all_methods = force_method_calls_to_qapp_thread(Impl(), all_methods=True)
        self.assertTrue(isinstance(all_methods.public, QAppThreadCall))
        self.assertTrue(isinstance(all_methods._private, QAppThreadCall))
