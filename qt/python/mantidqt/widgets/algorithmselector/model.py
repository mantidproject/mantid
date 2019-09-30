# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

from mantid.api import AlgorithmFactory

CATEGORY_SEP = '\\'


class AlgorithmSelectorModel(object):
    """
    This is a model for the algorithm selector widget.
    """
    algorithm_key = '_'

    def __init__(self, presenter, include_hidden=False):
        """
        Initialise a new instance of AlgorithmSelectorModel
        :param presenter: A presenter controlling this model.
        :param include_hidden: If True the widget must include all hidden algorithms
        """
        self.presenter = presenter
        self.include_hidden = include_hidden

    def get_algorithm_data(self):
        """
        Prepare the algorithm description data for displaying in the view.
        :return: A tuple of two elements. The first is a list of all algorithm names.
            This always contains all known algorithms regardless of their hidden state
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
        algm_factory = AlgorithmFactory.Instance()
        descriptors = algm_factory.getDescriptors(self.include_hidden)
        data = {}
        for descriptor in descriptors:
            categories = descriptor.category.split(CATEGORY_SEP)
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
            if descriptor.name not in d:
                d[descriptor.name] = []
            d[descriptor.name].append(descriptor.version)

        unique_alg_names = set(descr.name
                               for descr in algm_factory.getDescriptors(True))
        return sorted(unique_alg_names), data
