# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
import unittest

from qtpy.QtWidgets import QApplication

from mantidqt.utils.asynchronous import AsyncTask, TaskExitCode
from mantidqt.utils.qt.qappthreadcall import QAppThreadCall, force_method_calls_to_qapp_thread
from mantidqt.utils.qt.testing import start_qapplication


# Helper functions for various task scenarios
class CustomException(Exception):
    pass


def raises_exception():
    raise CustomException()


@start_qapplication
class QAppThreadCallTest(unittest.TestCase):
    def test_successive_non_blocking_calls_receive_expected_arguments(self):
        self.do_successive_call_with_expected_args_test(blocking=True)

    def test_successive_blocking_calls_receive_expected_arguments(self):
        self.do_successive_call_with_expected_args_test(blocking=False)

    def do_successive_call_with_expected_args_test(self, blocking: bool):
        # scenario: Test calling a callable from a separate thread with successive arguments
        # Simple add function taking successive pairs of arguments.
        input_args = ((1, 2), (4, 3))
        called_args = []

        def add(lhs, rhs):
            called_args.append((lhs, rhs))
            return lhs + rhs

        # Wrap the callable with our wrapper
        wrapped_add = QAppThreadCall(add, blocking=blocking)

        # Define an outer function that is the entrypoint for the new thread
        def add_inputs():
            for arg in input_args:
                wrapped_add(*arg)

        # Start async task to do the addition and simulate
        thread = AsyncTask(add_inputs, error_cb=lambda x: self.fail(msg=str(x.exc_value)))
        thread.start()
        while thread.is_alive():
            QApplication.sendPostedEvents()
        thread.join()
        QApplication.sendPostedEvents()

        # Assert correct ordering
        self.assertEqual(
            len(called_args), len(input_args), msg="Number of called arguments does not match the expected number of input arguments"
        )
        for input_arg, called_arg in zip(input_args, called_args):
            self.assertEqual(input_arg, called_arg)

    def test_correct_exception_is_raised_when_called_on_main_thread(self):
        qappthread_wrap = QAppThreadCall(raises_exception)

        self.assertRaises(CustomException, qappthread_wrap)

    def test_correct_exception_is_raised_when_called_on_other_thread(self):
        self.exc = None
        self.exit_code = None

        def collect_error(e):
            self.exc = e

        def raise_an_error_on_qapp_thread():
            qappthread_wrap = QAppThreadCall(raises_exception)
            qappthread_wrap()

        thread = AsyncTask(raise_an_error_on_qapp_thread, error_cb=collect_error)
        thread.start()
        while thread.is_alive():
            QApplication.sendPostedEvents()
        thread.join(0.5)

        self.assertEqual(TaskExitCode.ERROR, thread.exit_code)
        self.assertTrue(isinstance(self.exc.exc_value, CustomException), msg=f"Expected CustomException, found {self.exc.exc_value}")

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


if __name__ == "__main__":
    unittest.main(verbosity=2)
