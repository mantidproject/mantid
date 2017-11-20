from __future__ import absolute_import
from collections import namedtuple
from .model import Model


SelectedAlgorithm = namedtuple('SelectedAlgorithm', ['name', 'version'])


class IAlgorithmSelectorView(object):
    """
    The interface to the actual algorithm selector view.
    Presenter interacts with the view through this interface only.
    """
    def __init__(self):
        # Actual view creates its ui elements
        self.init_ui()
        # A dict key for retrieving algorithms that have no sub-categories
        self.algorithm_key = Model.algorithm_key
        # The view will have a reference to the Presenter
        self.presenter = Presenter(self)

    def init_ui(self):
        raise NotImplementedError('Method has to be implemented in a subclass')

    def populate_ui(self, data):
        raise NotImplementedError('Method has to be implemented in a subclass')

    def get_selected_algorithm(self):
        raise NotImplementedError('Method has to be implemented in a subclass')

    def execute_algorithm(self):
        algorithm = self.get_selected_algorithm()
        if algorithm is not None:
            print 'Execute %s v.%s' % algorithm


class Presenter(object):
    """
    Presents (controls) an algorithm selector view. This UI element allows the user
    to select and execute a Mantid algorithm.
    """
    def __init__(self, view):
        self.view = view
        self.model = Model(self)
        algorithm_data = self.model.get_algorithm_data()
        self.view.populate_ui(algorithm_data)
