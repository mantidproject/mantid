# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import time

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QApplication

from mantidqtinterfaces.Muon.GUI.Common.thread_model import ThreadModel


class testModelWithoutExecute:
    def __init__(self):
        pass


class testModelWithoutOutput:
    def __init__(self):
        pass

    def execute(self):
        pass


class testModelWithoutLoadData:
    def __init__(self):
        pass

    def execute(self):
        pass


class testModel:
    def __init__(self):
        self._data = None

    def loadData(self, data):
        self._data = data

    def execute(self):
        pass


@start_qapplication
class LoadFileWidgetViewTest(unittest.TestCase):
    class Runner:
        """This runner class creates a main event loop for threaded code to run within (otherwise the threaded
        code will not connect signals/slots properly).
        The finished signal of a QThread is connected to the finished method below"""

        def __init__(self, thread_model):
            if thread_model:
                thread_model.start()

    def setUp(self):
        self._qapp = QApplication.instance()
        patcher = mock.patch("mantidqtinterfaces.Muon.GUI.Common.thread_model.warning")
        self.addCleanup(patcher.stop)
        self.warning_box_patcher = patcher.start()
        self.model = testModel()
        self.thread = ThreadModel(self.model)

    def mock_model(self):
        model = mock.Mock()
        model.loadData = mock.Mock(side_effect=[1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
        model.execute = mock.Mock()
        model.output = mock.Mock()
        return model

    def wait_for_thread(self, thread_model):
        if thread_model and thread_model.worker:
            while thread_model.worker.is_alive():
                time.sleep(0.1)
            QApplication.sendPostedEvents()

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_loadData_called_in_model_with_correct_inputs(self):
        self.model.loadData = mock.Mock()

        self.thread.loadData([1, 2, 3, 4, 5])

        self.assertEqual(self.model.loadData.call_count, 1)
        self.assertEqual(self.model.loadData.call_args_list[0][0][0], [1, 2, 3, 4, 5])

    def test_that_execute_is_called_in_model_when_thread_is_started(self):
        self.model.execute = mock.Mock()

        self.Runner(self.thread)
        self.wait_for_thread(self.thread)
        self._qapp.processEvents()

        self.assertEqual(self.model.execute.call_count, 1)

    def test_that_starting_and_finishing_callbacks_are_called_when_thread_starts_and_finishes(self):
        start_slot = mock.Mock()
        end_slot = mock.Mock()

        self.thread.threadWrapperSetUp(start_slot, end_slot)

        self.Runner(self.thread)
        self.wait_for_thread(self.thread)
        self._qapp.processEvents()

        self.assertEqual(start_slot.call_count, 1)
        self.assertEqual(end_slot.call_count, 1)

    def test_that_AttributeError_raised_if_trying_to_load_data_into_model_without_loadData_method(self):
        model = testModelWithoutLoadData()

        thread = ThreadModel(model)
        with self.assertRaises(AttributeError):
            thread.loadData(None)

    def test_that_attribute_error_raised_if_model_does_not_contain_execute_method(self):
        model = testModelWithoutExecute()

        with self.assertRaises(AttributeError):
            ThreadModel(model)

    def test_that_tearDown_function_called_automatically(self):
        start_slot = mock.Mock()
        end_slot = mock.Mock()

        self.thread.threadWrapperSetUp(start_slot, end_slot)

        self.Runner(self.thread)
        self.wait_for_thread(self.thread)
        self._qapp.processEvents()

        self.assertEqual(start_slot.call_count, 1)
        self.assertEqual(end_slot.call_count, 1)

    def test_that_message_box_called_when_execute_throws_even_without_setup_and_teardown_methods(self):
        def raise_error():
            raise ValueError()

        self.model.execute = mock.Mock(side_effect=raise_error)

        self.Runner(self.thread)
        self.wait_for_thread(self.thread)
        self._qapp.processEvents()

        self.assertEqual(self.warning_box_patcher.call_count, 1)

    def test_that_passing_non_callables_to_setUp_throws_AssertionError(self):
        with self.assertRaises(AssertionError):
            self.thread.threadWrapperSetUp(1, 2)


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
