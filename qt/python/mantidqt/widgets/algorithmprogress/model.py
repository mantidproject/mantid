from __future__ import absolute_import, print_function

from mantid.api import AlgorithmObserver


class ProgressObserver(AlgorithmObserver):
    """
    Observes a single algorithm for its progress and finish notifications
    and updates presenter accordingly.
    """
    def __init__(self, model, alg):
        super(ProgressObserver, self).__init__()
        self.model = model
        self.algorithm = alg
        self.message = None
        self.progress = 0.0

    def name(self):
        return self.algorithm.name()

    def properties(self):
        """
        Get algorithm properties.
        :return: A list of objects with fields: name, value, isDefault.
        """
        return self.algorithm.getProperties()

    def finishHandle(self):
        self.model.remove_observer(self)

    def progressHandle(self, p, message):
        self.progress = p
        if len(message) > 0:
            self.message = message
        else:
            self.message = None
        self.model.update_progress(self)

    def errorHandle(self, message):
        """
        Error handling including cancelling algorithm
        :param message: Error message (unused)
        """
        self.model.remove_observer(self)


class AlgorithmProgressModel(AlgorithmObserver):
    """
    Observes AlgorithmManager for new algorithms and starts
    ProgressObservers.
    """
    def __init__(self):
        super(AlgorithmProgressModel, self).__init__()
        self.presenters = []
        self.progress_observers = []
        self.observeStarting()

    def add_presenter(self, presenter):
        self.presenters.append(presenter)

    def remove_presenter(self, presenter):
        index = self.presenters.index(presenter)
        if index >= 0:
            del self.presenters[index]

    def startingHandle(self, alg):
        """
        Called when a new algorithm starts. Registers a new ProgressObserver for
        this algorithm.
        :param alg: The new algorithm.
        """
        progress_observer = ProgressObserver(self, alg)
        progress_observer.observeProgress(alg)
        progress_observer.observeFinish(alg)
        progress_observer.observeError(alg)
        self.progress_observers.append(progress_observer)
        self.update_presenter()

    def update_presenter(self):
        for presenter in self.presenters:
            presenter.update()

    def update_progress(self, progress_observer):
        """
        Update the progress bar in the view.
        :param progress_observer: A observer reporting a progress.
        """
        for presenter in self.presenters:
            presenter.update_progress_bar(progress_observer.algorithm,
                                          progress_observer.progress,
                                          progress_observer.message)

    def remove_observer(self, progress_observer):
        """
        Remove the progress observer when it's finished
        :param progress_observer: A ProgressObserver that needs removing
        """
        index = self.progress_observers.index(progress_observer)
        if index >= 0:
            del self.progress_observers[index]
            self.update_presenter()

    def get_running_algorithms(self):
        return [obs.algorithm for obs in self.progress_observers]

    def get_running_algorithm_data(self):
        """
        Create data describing running algorithms.
        :return: A list of tuples of three elements:
            1. an id of the algorithm
            2. the algorithm's name
            3. a list of algorithm property descriptors - 3-element lists of the form:
                [prop_name, prop_value_as_str, 'Default' or '']
        """
        algorithm_data = []
        for observer in self.progress_observers:
            properties = []
            for prop in observer.properties():
                properties.append([prop.name, str(prop.value), 'Default' if prop.isDefault else ''])
            algorithm_data.append((observer.name(), observer.algorithm, properties))
        return algorithm_data
