# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from functools import wraps
from typing import Any, Optional, Callable

from qtpy import QtCore
from qtpy.QtCore import QObject, Signal

from mantidqt.utils.asynchronous import AsyncTask, AsyncTaskFailure, AsyncTaskSuccess
from mantid.kernel import Logger


class IQtAsync:
    """
    A base class which should be extended to write async methods. This allows the user to
    optionally overwrite the callbacks they require, and automatically wires up the Qt signal
    and slots.
    This allows us to use native Python tooling that can't work the "Qt blackbox" whilst still
    allowing us to use slots within the callbacks which cannot be done directly from a threading
    callback. For more in-depth details see AsyncTaskQtAdaptor.

    To make a Qt Async method simply:
    - Inherit from this class and add any method(s) to run async
    - Overwrite any of the below callbacks are required
    - Annotate anything to run async with the @qt_async_task decorator
    - Get yourself a nice coffee to celebrate a job well done

    For the benefits of using this over QThreads see AsyncTaskQtAdaptor.
    """
    def __init__(self):
        self._worker = None
        self._run_synchronously = False

    def set_unit_test_mode(self, value: bool):
        """
        Enables unit test mode to prevent having to deal with race-y tests due
        to threading. Instead it forces the target to run as a blocking call.

        @param value: True: Force tasks to run on current thread instead of async
        False (default): Tasks will run in async mode. This may cause random failures in testing.
        """
        self._run_synchronously = value

    def success_cb_slot(self, result: AsyncTaskSuccess) -> None:
        pass

    def error_cb_slot(self, result: AsyncTaskFailure) -> None:
        pass

    def finished_cb_slot(self) -> None:
        pass

    def _internal_finished_handler(self):
        self.finished_cb_slot()
        self._worker = None


def qt_async_task(method: Callable):
    """
    Decorator to mark a given method as async. This method must belong to a class that
    inherits from IQtAsync.

    Example:
        class Bar(IQtAsync):
            def __init__(self):
                super().__init()

            # Callback overrides omitted for brevity, see IQtAsync

            @qt_async_task
            def async_method(some_param_1, some_param_2, ...etc):
                some_expensive_call(some_param_1, some_param_2) # Will run async
    """
    @wraps(method)
    def _inner(self, *args, **kwargs):
        # So that we can store the worker so it doesn't end up garbage collected with no refs
        # also to make setting up callbacks easier too, as you can't use self.x within a decorator
        assert isinstance(self, IQtAsync), "This class must inherit from IQtAsync"

        if self._worker:
            Logger("Qt Async Task").warning(
                "Starting a new task, the task in progress has been cancelled")
            self._worker.abort()

        self._worker = AsyncTaskQtAdaptor(target=method, args=(self, *args), kwargs=kwargs,
                                          success_cb=self.success_cb_slot,
                                          error_cb=self.error_cb_slot,
                                          finished_cb=self._internal_finished_handler,
                                          run_synchronously=self._run_synchronously)
        if self._run_synchronously:
            self._worker.run()
        else:
            self._worker.start()
    return _inner


class _QtSignals(QObject):
    """
    Wrapper class for the signals used to translate from Python threading -> Qt Signals/Slots
    """
    success_signal = Signal(AsyncTaskSuccess)
    error_signal = Signal(AsyncTaskFailure)
    finished_signal = Signal()


ErrorCbType = Optional[Callable[[AsyncTaskFailure], None]]
SuccessCbType = Optional[Callable[[AsyncTaskSuccess], None]]
FinishedCbType = Optional[Callable[[None], None]]


class AsyncTaskQtAdaptor(AsyncTask):
    """
    Adaptor between Python threading and Qt signals and slots.

    Why?
    ----
    This allows us to get the best of both; clean tracebacks and native tooling (such as
    profiling) are difficult to get right with Qt Threads.
    This is further compounded by the fact someone who wants to simply run something async
    on a GUI needs to then write signals, slots, connect them up ...etc.

    However, if you naively try to get the GUI thread to do anything, such as update
    or show a dialog box from a thread without the event loop you will quickly cause
    the main thread to segmentation fault. Qt does not take kindly to widgets just randomly
    doing things without it's control.

    So from the end-user perspective they can literally write a method that sits alongside the
    GUI thread, annotate their task with the decorator and they're done.

    How
    ---
    Instead we simply use the fact that a QueuedConnection is thread safe to emit a message
    to the slot. Because of duck-typing users don't even have to annotate their slot (though
    the types need to be correct for similar reasons, hence providing an interface type).

    Strictly speaking it means that there may be a brief window of time where the background
    thread and GUI thread end up visually out of sync, especially if something is hogging the
    main event loop. However, in this case it means something is incorrectly running something slow
    on the main thread and is out of scope for this.
    """

    def __init__(self, target: Callable, args: Any = (), kwargs: Any = None,
                 success_cb: SuccessCbType = None,
                 error_cb: ErrorCbType = None,
                 finished_cb: FinishedCbType = None,
                 run_synchronously: bool = False):
        self._signals = _QtSignals()
        self._run_synchronously = run_synchronously
        success_cb = self._wrap_signal(self._signals.success_signal, success_cb)
        error_cb = self._wrap_signal(self._signals.error_signal, error_cb)
        finished_cb = self._wrap_signal(self._signals.finished_signal, finished_cb)

        super(AsyncTaskQtAdaptor, self).__init__(
            target=target, args=args, kwargs=kwargs,
            success_cb=success_cb, error_cb=error_cb, finished_cb=finished_cb
        )

    def _wrap_signal(self, signal: Signal, slot: Optional[Callable]) -> Optional[Callable]:
        """
        Wraps a given signal so that the thread calls emit rather than the target directly.
        This allows us to complete the slot on the target, rather than trying
        to invoke across threads causing Qt to assert / segfault
        """
        if not slot:
            return None

        signal.connect(slot, QtCore.Qt.QueuedConnection)

        def emit_on_completion(*args, **kwargs):
            if self._run_synchronously:
                slot(*args, **kwargs)
            else:
                signal.emit(*args, **kwargs)

        return emit_on_completion if slot else None
