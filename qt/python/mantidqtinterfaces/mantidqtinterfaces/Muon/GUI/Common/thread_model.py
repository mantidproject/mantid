# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from qtpy.QtCore import Signal
from qtpy.QtCore import Slot
from mantidqt.utils.async_qt_adaptor import AsyncTaskQtAdaptor
from mantidqtinterfaces.Muon.GUI.Common.message_box import warning


class WorkerSignals(QtCore.QObject):
    """
    Defines the signals available from a running ThreadModelWorker.
    """

    started = Signal()
    finished = Signal()
    error = Signal(str)
    cancel = Signal()
    # Use this to start the worker
    start = Signal()


class ThreadModelWorker(QtCore.QObject):
    """
    The worker objects wraps the code that is to be run on the QThread, allowing us to "move" the instance of this class
    into the thread (this prevents us needing to inherit from QThread, with is not a QObject, which causes many issues
    for parenting widgets and managing the threading).
    """

    def __init__(self, model):
        super(ThreadModelWorker, self).__init__()
        self.signals = WorkerSignals()
        self.model = model

    # This decorator is needed for the method to be successfully run on another thread.
    # https://stackoverflow.com/questions/20752154/pyqt-connecting-a-signal-to-a-slot-to-start-a-background-operation/20818401#20818401
    @Slot()
    def run(self):
        self.signals.started.emit()
        try:
            self.model.execute()
            self.model.output()
        except KeyboardInterrupt:
            self.signals.cancel.emit()
        except Exception as error:
            try:
                self.signals.error.emit(error.args[0])
            except IndexError:
                self.signals.error.emit("")
        finally:
            self.signals.finished.emit()
            current_thread = QtCore.QThread.currentThread()
            if current_thread is not None:
                current_thread.quit()


class ThreadModel(QtWidgets.QWidget):
    """
    A wrapper to allow threading with
    the MaxEnt models.
    """

    exceptionSignal = QtCore.Signal(str)

    def __init__(self, model):
        super(ThreadModel, self).__init__()
        self.model = model
        self.worker = None
        # callbacks for the .started() and .finished() signals of the worker
        self.start_slot = lambda: 0
        self.end_slot = lambda: 0
        self._exception_callback = self._default_exception_callback

        self.check_model_has_correct_attributes()

    def check_model_has_correct_attributes(self):
        if hasattr(self.model, "execute"):
            return
        raise AttributeError("Please ensure the model passed to ThreadModel has implemented" " execute() method")

    def setup_thread_and_start(self):
        # Construct the Async thread
        self.worker = AsyncTaskQtAdaptor(target=self.model.execute, error_cb=self.warning, finished_cb=self.threadWrapperTearDown)
        self.worker.start()

        # trigger the slot for the start of the process
        self.start_slot()

    def start(self):
        # keep this method to maintain consistency with older usages of the ThreadModel
        self.setup_thread_and_start()

    def warning(self, message):
        self._exception_callback(message)

    def join(self):
        if self.exception is not None:
            raise self.exception

    def cancel(self):
        self.model.cancel()

    # if there is one set of inputs (1 alg)
    def setInputs(self, inputs, runName):
        self.model.setInputs(inputs, runName)

    # if there are multiple inputs (alg>1)
    def loadData(self, inputs):
        if not hasattr(self.model, "loadData"):
            raise AttributeError(
                "The model passed to ThreadModel has not implemented" " loadData() method, which it is attempting to call."
            )
        self.model.loadData(inputs)

    def threadWrapperSetUp(self, on_thread_start_callback=lambda: 0, on_thread_end_callback=lambda: 0, on_thread_exception_callback=None):
        assert hasattr(on_thread_start_callback, "__call__")
        assert hasattr(on_thread_end_callback, "__call__")

        self.start_slot = on_thread_start_callback
        self.end_slot = on_thread_end_callback
        if on_thread_exception_callback is not None:
            assert hasattr(on_thread_exception_callback, "__call__")
            self._exception_callback = on_thread_exception_callback
        else:
            self._exception_callback = self._default_exception_callback

    def threadWrapperTearDown(self):
        self.end_slot()
        self.start_slot = lambda: 0
        self.end_slot = lambda: 0

    def _default_exception_callback(self, message):
        warning(message)
