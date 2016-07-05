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
        The abstract method which should be overridden by inheriting classes.
        """
        return None


    def extract(self):
        """
        Returns the data.
        @return: data
        """
        return self._data

