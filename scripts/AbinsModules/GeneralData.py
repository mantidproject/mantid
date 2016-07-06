import numpy as np

class GeneralData(object):

    def __init__(self):
        self._data = [] # data


    def append(self, item=None):
        """
        Appends one element of the data to the collection.
        The abstract method which should be overridden by inheriting classes.
        """
        return None


    def set(self, items=None):
        """
        Sets a new value for a collection of the data.
        """

        if isinstance(items, list): # first calculation of the data
            self._data = []
            for item in items:
                self.append(item=item)
        elif isinstance(items, np.ndarray): # case of loading from hdf file
            self._data = items


    def extract(self):
        """
        Returns the data.
        @return: data
        """
        return self._data

