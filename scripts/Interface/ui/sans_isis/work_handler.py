# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from PyQt4.QtCore import pyqtSlot, QThreadPool
from abc import ABCMeta, abstractmethod
from six import with_metaclass
from worker import Worker
import functools
import uuid


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
        self.thread_pool = QThreadPool()

    def _add_listener(self, listener, process_id, id):
        if not isinstance(listener, WorkHandler.WorkListener):
            raise ValueError("The listener is not of type WorkListener but rather {}".format(type(listener)))
        self._listener.update({process_id: {'id':id,'listener': listener}})

    @pyqtSlot()
    def on_finished(self, process_id):
        if process_id not in self._worker:
            return
        result = self._worker.pop(process_id)['worker'].result
        self._listener.pop(process_id)['listener'].on_processing_finished(result)

    @pyqtSlot()
    def on_error(self, process_id, error):
        if process_id not in self._worker:
            return
        self._listener[process_id]['listener'].on_processing_error(error)

    def process(self, caller, func, id, *args, **kwargs):
        self.remove_already_processing(id)
        process_id = uuid.uuid4()
        # Add the caller
        self._add_listener(caller, process_id, id)

        finished_callback = functools.partial(self.on_finished, process_id)
        error_callback = functools.partial(self.on_error, process_id)

        worker = Worker(func, *args, **kwargs)
        worker.signals.finished.connect(finished_callback)
        worker.signals.error.connect(error_callback)

        self._worker.update({process_id: {'id': id, 'worker': worker}})

        self.thread_pool.start(self._worker[process_id]['worker'])

    def remove_already_processing(self, id):
        for key, process in self._listener.items():
            if process['id'] == id:
                self._listener.pop(key)
                self._worker.pop(key)

    def wait_for_done(self):
        self.thread_pool.waitForDone()

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    def __ne__(self, other):
        return self.__dict__ != other.__dict__
