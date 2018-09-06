import unittest
import sys

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

from Muon.GUI.Common.thread_model import ThreadModel
from Muon.GUI.Common import mock_widget
from PyQt4.QtGui import QApplication
from PyQt4 import QtGui
from Muon.GUI.Common.message_box import warning


class testModelWithoutExecute:

    def __init__(self):
        pass

    def output(self):
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

    def output(self):
        pass


class testModel:

    def __init__(self):
        self._data = None

    def loadData(self, data):
        self._data = data

    def output(self):
        pass

    def execute(self):
        pass


class LoadFileWidgetViewTest(unittest.TestCase):
    class Runner:
        """This runner class creates a main event loop for threaded code to run within (otherwise the threaded
        code will not connect signals/slots properly).
        The finished signal of a QThread is connected to the finished method below"""
        QT_APP = mock_widget.mockQapp()

        def __init__(self, thread):
            if thread:
                self._thread = thread
                self._thread.finished.connect(self.finished)
                self._thread.start()
                if self._thread.isRunning():
                    self.QT_APP.exec_()

        def finished(self):
            self.QT_APP.processEvents()
            self.QT_APP.exit(0)

    def setUp(self):
        self.obj = QtGui.QWidget()
        self.model = testModel()
        self.exception_callback = mock.Mock()
        self.thread = ThreadModel(self.model, self.exception_callback)

    def tearDown(self):
        self.obj = None

    def mock_model(self):
        model = mock.Mock()
        model.loadData = mock.Mock(side_effect=[1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
        model.execute = mock.Mock()
        model.output = mock.Mock()
        return model

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

        self.assertEqual(self.model.execute.call_count, 1)

    def test_that_output_is_called_if_thread_executes_successfully(self):
        self.model.execute = mock.Mock()
        self.model.output = mock.Mock()

        self.Runner(self.thread)

        self.assertEqual(self.model.output.call_count, 1)

    def test_that_starting_and_finishing_callbacks_are_called_when_thread_starts_and_finishes(self):
        start_slot = mock.Mock()
        end_slot = mock.Mock()

        self.thread.threadWrapperSetUp(start_slot, end_slot)

        self.Runner(self.thread)

        self.assertEqual(start_slot.call_count, 1)
        self.assertEqual(end_slot.call_count, 1)

    def test_that_AttributeError_raised_if_trying_to_load_data_into_model_without_loadData_method(self):
        model = testModelWithoutLoadData()

        thread = ThreadModel(model, self.exception_callback)
        with self.assertRaises(AttributeError):
            thread.loadData(None)

    def test_that_attribute_error_raised_if_model_does_not_contain_execute_method(self):
        model = testModelWithoutExecute()

        with self.assertRaises(AttributeError):
            ThreadModel(model, self.exception_callback)

    def test_that_attribute_error_raised_if_model_does_not_contain_output_method(self):
        model = testModelWithoutOutput

        with self.assertRaises(AttributeError):
            ThreadModel(model, self.exception_callback)

    def test_that_tearDown_function_called_automatically(self):
        start_slot = mock.Mock()
        end_slot = mock.Mock()

        self.thread.threadWrapperSetUp(start_slot, end_slot)

        self.Runner(self.thread)
        self.Runner(self.thread)

        self.assertEqual(start_slot.call_count, 1)
        self.assertEqual(end_slot.call_count, 1)

    def test_that_exception_callback_called_when_execute_throws_even_without_setup_and_teardown_methods(self):
        # Need to instantiate a new thread for patch to work
        self.thread = ThreadModel(self.model, self.exception_callback)

        def raise_error():
            raise ValueError()

        self.model.execute = mock.Mock(side_effect=raise_error)

        self.Runner(self.thread)

        self.assertEqual(self.exception_callback.call_count, 1)

    def test_that_passing_non_callables_to_setUp_throws_AssertionError(self):

        with self.assertRaises(AssertionError):
            self.thread.threadWrapperSetUp(1, 2)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
