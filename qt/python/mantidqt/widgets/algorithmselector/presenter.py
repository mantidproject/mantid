from __future__ import absolute_import
from mantid import AlgorithmFactory


class IAlgorithmSelectorView(object):
    """
    The interface to the actual algorithm selector view.
    Presenter interacts with the view through this interface only.
    """
    def __init__(self):
        # Actual view creates its ui elements
        self.init_ui()
        # The view will have a reference to the Presenter
        self.presenter = Presenter(self)

    def init_ui(self):
        raise NotImplementedError('Method has to be implemented in a subclass')

    def populate_ui(self, data):
        raise NotImplementedError('Method has to be implemented in a subclass')


class Presenter(object):
    """
    Presents (controls) an algorithm selector view. This UI element allows the user
    to select and execute a Mantid algorithm.
    """
    def __init__(self, view):
        self.view = view
        descriptors = AlgorithmFactory.getDescriptors(False)
        self.view.populate_ui(descriptors)
