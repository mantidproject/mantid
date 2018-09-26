from PyQt4.QtCore import pyqtSlot, QThreadPool
from abc import ABCMeta, abstractmethod
from six import with_metaclass
from worker import Worker
import functools

class WorkHandler(object):
    class WorkListener(with_metaclass(ABCMeta, object)):
        def __init__(self):
            pass

        @abstractmethod
        def on_processing_finished(self, result):
            pass

        @abstractmethod
        def on_processing_error(self, error):
            pass

    def __init__(self):
        self.thread_pool = None
        self._listener = {}
        self._worker = {}

    def _add_listener(self, listener, process_id):
        if not isinstance(listener, WorkHandler.WorkListener):
            raise ValueError("The listener is not of type WorkListener but rather {}".format(type(listener)))
        self._listener.update({process_id: listener})

    @pyqtSlot()
    def on_finished(self, process_id):
        result = self._worker.pop(process_id).result
        self._listener[process_id].on_processing_finished(result)
        self._listener.pop(process_id)

    @pyqtSlot()
    def on_error(self, process_id, error):
        self._listener[process_id].on_processing_error(error)

    def process(self, caller, func, process_id, *args, **kwargs):
        # Add the caller
        self._add_listener(caller, process_id)

        finished_callback = functools.partial(self.on_finished, process_id)
        error_callback = functools.partial(self.on_error, process_id)

        worker = Worker(func, *args, **kwargs)
        worker.signals.finished.connect(finished_callback)
        worker.signals.error.connect(error_callback)

        self._worker.update({process_id: worker})

        QThreadPool.globalInstance().start(self._worker[process_id])

    def wait_for_done(self):
        QThreadPool.globalInstance().waitForDone()