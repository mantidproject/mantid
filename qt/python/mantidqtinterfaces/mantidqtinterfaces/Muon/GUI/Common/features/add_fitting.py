# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.features.add_feature import AddFeature


FITWIZARD = "fit_wizard"
ADD = 1


class AddFitting(AddFeature):
    """
    Add model analysis to the GUI
    """

    def __init__(self, GUI, feature_dict):
        super().__init__(GUI, feature_dict)

    def _get_features(self, feature_dict):
        features = []
        if FITWIZARD in feature_dict.keys() and feature_dict[FITWIZARD]==ADD:
            features.append(ADD)
        return features

    def _add_features(self, GUI):
        if ADD in self.feature_list:
            GUI.fitting_tab.show_fit_script_generator()
