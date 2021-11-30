# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABC


class AddFeature(ABC):
    """
    Base class for adding a feature into one of the muon GUI's
    This allows a simple check to be added for if to add the feature
    """

    def __init__(self, GUI, feature_dict):
        self.feature_list = self._get_features(feature_dict)
        self._add_features(GUI)

    def _get_features(self, feature_dict):
        # Determines which  fetures need to be added
        raise NotImplementedError("get features not implemented")

    def _add_features(self, GUI):
        # This adds the appropriate features
        raise NotImplementedError("add features not implemented")

    def add_to_tab(self, GUI):
        # This is not always needed
        # It will add the feature to a tab in the GUI
        raise NotImplementedError("add to tab not implemented")

    def add_observers_to_feature(self, GUI):
        # Adds the observers from the rest of the GUI for a change in this feature

        raise NotImplementedError("add observers to feature not implemented")

    def set_feature_observables(self, GUI):
        # Sets the observables of the feature from the rest of the GUI
        raise NotImplementedError("set features to observer not implemented")
