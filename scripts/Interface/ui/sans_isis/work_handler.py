from PyQt4.QtCore import pyqtSlot, QThreadPool
from abc import ABCMeta, abstractmethod
from six import with_metaclass
from worker import Worker


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
        self._listener = None
        self._worker = None

    def _add_listener(self, listener):
        if not isinstance(listener, WorkHandler.WorkListener):
            raise ValueError("The listener ist not of type WorkListener but rather {}".format(type(listener)))
        self._listener = listener

    @pyqtSlot()
    def on_finished(self):
        result = self._worker.result
        self._worker = None
        self._listener.on_processing_finished(result)

    @pyqtSlot()
    def on_error(self, error):
        self._worker = None
        self._listener.on_processing_error(error)

    def process(self, caller, func, *args, **kwargs):
        # Add the caller
        self._add_listener(caller)

        # Generate worker
        self._worker = Worker(func, *args, **kwargs)
        self._worker.signals.finished.connect(self.on_finished)
        self._worker.signals.error.connect(self.on_error)

        QThreadPool.globalInstance().start(self._worker)
