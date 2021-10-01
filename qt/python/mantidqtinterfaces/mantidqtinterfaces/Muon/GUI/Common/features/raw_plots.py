# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.features.add_feature import AddFeature


RAWPLOTS = "raw_plots"
ADD = 1


class AddRawPlots(AddFeature):
    """
    Add model analysis to the GUI
    """

    def __init__(self, GUI, feature_dict):
        super().__init__(GUI, feature_dict)

    def _get_features(self, feature_dict):
        features = []
        if RAWPLOTS in feature_dict.keys() and feature_dict[RAWPLOTS]==ADD:
            features.append(ADD)
        return features

    def _add_features(self, GUI):
        if ADD in self.feature_list:
            GUI.plot_widget.create_raw_pane()

    def add_observers_to_feature(self, GUI):
        if ADD in self.feature_list:
            GUI.load_widget.load_widget.loadNotifier.add_subscriber(
                GUI.plot_widget.raw_mode.new_data_observer)
