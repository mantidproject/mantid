from __future__ import absolute_import, print_function

from mantid.api import AlgorithmObserver


class ProgressObserver(AlgorithmObserver):
    """
    Observes a single algorithm for its progress and finish notifications
    and updates presenter accordingly.
    """
    def __init__(self, model):
        super(ProgressObserver, self).__init__()
        self.model = model
        self.message = None
        self.progress = 0.0

    def finishHandle(self):
        self.model.remove_observer(self)

    def progressHandle(self, p, message):
        self.progress = p
        if len(message) > 0:
            self.message = message
        else:
            self.message = None
        self.model.update_progress(self)


class AlgorithmProgressModel(AlgorithmObserver):
    """
    Observes AlgorithmManager for new algorithms and starts
    ProgressObservers.
    """
    def __init__(self, presenter):
        super(AlgorithmProgressModel, self).__init__()
        self.presenter = presenter
        self.progress_observers = []
        self.observeStarting()

    def startingHandle(self, alg):
        progress_observer = ProgressObserver(self)
        progress_observer.observeProgress(alg)
        progress_observer.observeFinish(alg)
        self.progress_observers.append(progress_observer)

    def update_progress(self, progress_observer):
        """
        Update the progress bar in the view.
        :param progress_observer: A observer reporting a progress.
        """
        self.presenter.update_progress_bar(progress_observer.progress, progress_observer.message)

    def remove_observer(self, progress_observer):
        """
        Remove the progress observer when it's finished
        :param progress_observer: A ProgressObserver that needs removing
        """
        index = self.progress_observers.index(progress_observer)
        if index >= 0:
            del self.progress_observers[index]
