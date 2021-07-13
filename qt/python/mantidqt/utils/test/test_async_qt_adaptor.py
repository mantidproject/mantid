# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqt.utils.async_qt_adaptor import IQtAsync, qt_async_task, AsyncTaskQtAdaptor


class _StubbedCbHandler(IQtAsync):
    def __init__(self):
        super().__init__()
        self.finished_called = False

    def finished_cb_slot(self) -> None:
        self.finished_called = True


class IQtAsyncTest(unittest.TestCase):
    def test_internal_finish_handler_cleans_up(self):
        fixture = IQtAsync()
        self.assertIsNone(fixture._worker)

        fixture._worker = mock.Mock()
        fixture._internal_finished_handler()
        self.assertIsNone(fixture._worker)

    def test_internal_finished_handler_calls_user_one(self):
        fixture = _StubbedCbHandler()
        fixture._internal_finished_handler()
        self.assertTrue(fixture.finished_called)

    def test_all_standard_cb_are_callable(self):
        fixture = IQtAsync()
        for f in [fixture.success_cb_slot, fixture.error_cb_slot]:
            self.assertIsNone(f(mock.NonCallableMock()))

        self.assertIsNone(fixture.finished_cb_slot())


class StubTask(_StubbedCbHandler):
    @qt_async_task
    def some_task(self):
        pass

    @qt_async_task
    def some_exception(self):
        raise Exception("Some exception")


@mock.patch("mantidqt.utils.async_qt_adaptor.AsyncTaskQtAdaptor", autospec=True)
class QtAsyncTaskDecoratorTest(unittest.TestCase):
    def test_start_called(self, mocked_adaptor):
        tasks = StubTask()
        tasks.some_task()

        mocked_adaptor.return_value.start.assert_called_once()
        mocked_adaptor.return_value.run.assert_not_called()
        # This should not have run as we have mocked the async backend
        # if it has run it's likely were calling 'method()' directly
        # instead of using .start()
        self.assertFalse(tasks.finished_called)

    def test_unit_test_mode_uses_run(self, mocked_adaptor):
        class StubTestFixture(IQtAsync):
            def __init__(self):
                super().__init__()
                self.set_unit_test_mode(True)

            @qt_async_task
            def run_synchronous(self):
                pass

        test_fixture = StubTestFixture()
        test_fixture.run_synchronous()
        mocked_adaptor.return_value.start.assert_not_called()
        mocked_adaptor.return_value.run.assert_called_once()

    @mock.patch("mantidqt.utils.async_qt_adaptor.Logger", autospec=True)
    def test_abort_called_in_progress(self, mocked_logger, mocked_adaptor):
        task = StubTask()
        original_worker = mock.Mock()
        task._worker = original_worker

        task.some_task()
        mocked_logger.return_value.warning.assert_called_once()
        original_worker.abort.assert_called_once()

        self.assertNotEqual(original_worker, task._worker)
        self.assertEqual(mocked_adaptor.return_value, task._worker)

    def test_callbacks_passed_through_decorator(self, mocked_adaptor):
        task = StubTask()
        task.some_task()

        mocked_adaptor.assert_called_with(
            target=mock.ANY, args=mock.ANY, kwargs=mock.ANY,
            error_cb=task.error_cb_slot,
            success_cb=task.success_cb_slot,
            # This is special as we have cleanup code afterwards
            finished_cb=task._internal_finished_handler)


@mock.patch("mantidqt.utils.async_qt_adaptor._QtSignals")
class AsyncTaskQtAdaptorTest(unittest.TestCase):
    def test_signal_connected_to_user_cb(self, mocked_signals):
        signals_instance = mocked_signals.return_value
        cb_class = IQtAsync()
        AsyncTaskQtAdaptor(target=lambda: None, error_cb=cb_class.error_cb_slot,
                           finished_cb=cb_class.finished_cb_slot, success_cb=cb_class.success_cb_slot)

        for signal, slot in [(signals_instance.error_signal, cb_class.error_cb_slot),
                             (signals_instance.success_signal, cb_class.success_cb_slot),
                             (signals_instance.finished_signal, cb_class.finished_cb_slot)]:
            signal.connect.assert_called_once_with(slot, mock.ANY)

    def test_signal_can_handle_none_types(self, mocked_signals):
        signals_instance = mocked_signals.return_value
        cb_class = IQtAsync()
        AsyncTaskQtAdaptor(target=lambda: None, error_cb=cb_class.error_cb_slot)

        signals_instance.error_signal.connect.assert_called_once_with(cb_class.error_cb_slot, mock.ANY)

        for signal in [signals_instance.success_signal, signals_instance.finished_signal]:
            signal.connect.assert_not_called()

    def test_async_cb_wrapped_into_emit(self, mocked_signals):
        def null_task():
            pass

        fixture = AsyncTaskQtAdaptor(target=null_task, success_cb=null_task,
                                          error_cb=null_task, finished_cb=null_task)

        signals_instance = mocked_signals.return_value

        for callback, signal in [(fixture.error_cb, signals_instance.error_signal),
                                 (fixture.success_cb, signals_instance.success_signal),
                                 (fixture.finished_cb, signals_instance.finished_signal)]:
            signal.emit.assert_not_called()
            callback()
            signal.emit.assert_called_once()


if __name__ == '__main__':
    unittest.main()
