from __future__ import absolute_import, print_function
from mantid import AlgorithmFactory


class AlgorithmSelectorModel(object):
    """
    This is a model for the algorithm selector widget.
    """
    algorithm_key = '_'

    def __init__(self, presenter):
        self.presenter = presenter

    def get_algorithm_data(self):
        """
        Prepare the algorithm description data for displaying in the view.
        :return: A tuple of two elements. The first is a list of all algorithm names.
            The second is a tree of nested dicts where keys either category names
            or self.algorithm_key. Values of the category names are sub-trees
            and those of self.algorithm_key have dicts mapping algorithm names
            to lists of their versions. For example (as yaml):

                Arithmetic:
                  Errors:
                    _:
                      PoissonErrors: [1]
                      SetUncertainties: [1]
                      SignalOverError: [1]
                  FFT:
                    _:
                      ExtractFFTSpectrum: [1]
                      FFT: [1]
                      FFTDerivative: [1]
                      FFTSmooth: [1, 2]
                  _:
                    CrossCorrelate: [1]
                    Divide: [1]
                    Exponential: [1]
                    GeneralisedSecondDifference: [1]

            Here self.algorithm_key == '_'

        """
        descriptors = AlgorithmFactory.getDescriptors(False)
        algorithm_names = []
        data = {}
        for descriptor in descriptors:
            # d is a list of [name, version, category, alias]
            name, version, categories, alias = tuple(descriptor)
            if name not in algorithm_names:
                algorithm_names.append(name)
            categories = categories.split('\\')
            # Create nested dictionaries in which the key is a category and the value
            # is a similar dictionary with sub-categories as keys
            d = data
            for cat in categories:
                if cat not in d:
                    d[cat] = {}
                d = d[cat]
            # Entry with key == '' contains a dict with algorithm names as keys
            # The values are lists of version numbers
            if self.algorithm_key not in d:
                d[self.algorithm_key] = {}
            d = d[self.algorithm_key]
            if name not in d:
                d[name] = []
            d[name].append(version)
        return algorithm_names, data
