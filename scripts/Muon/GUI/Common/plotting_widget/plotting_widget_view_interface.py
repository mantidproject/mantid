# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import abstractmethod, ABCMeta
from qtpy import QtWidgets


# This metaclass is required in order to implement the (strict) interface for the QtWidget
class PlottingWidgetViewMeta(type(QtWidgets.QWidget), ABCMeta):
    pass


class PlottingWidgetViewInterface(metaclass=PlottingWidgetViewMeta):

    @abstractmethod
    def setup_plot_type_options(self, options):
        """
        Setup the options which are displayed in the plot type combo box
        """
        pass

    @abstractmethod
    def setup_tiled_by_options(self, options):
        """
        Setup to options which are shown in the tiled plots combo box
        """
        pass

    @abstractmethod
    def add_canvas_widget(self, canvas_widget):
        """
        Adds the canvas widget (where the plotting will occur) to the widget view.
        """
        pass

    @abstractmethod
    def get_plot_type(self):
        """
        Returns the current plot type
        """
        pass

    @abstractmethod
    def is_tiled_plot(self):
        """
        Checks if tiled plot is currently requested
        """
        pass

    @abstractmethod
    def is_raw_plot(self):
        """
        Checks if plotting raw data
        """
        pass

    @abstractmethod
    def tiled_by(self):
        """
        Returns the option which is currently selected in the 'tiled by' combo box
        """
        pass

    @abstractmethod
    def on_rebin_options_changed(self, slot):
        """
        Connect the plot_rebin checkbox to the input slot
        """
        pass

    @abstractmethod
    def on_plot_type_changed(self, slot):
        """
        Connect the plot_type combo box to the input slot
        """
        pass

    @abstractmethod
    def on_tiled_by_type_changed(self, slot):
        """
        Connect the tiled_by combo box to the input slot
        """
        pass

    @abstractmethod
    def on_plot_tiled_checkbox_changed(self, slot):
        """
        Connect the tiled_plot checkbox to the input slot
        """
        pass

    @abstractmethod
    def on_external_plot_pressed(self, slot):
        """
        Connect the external plot button to the input slot
        """
        pass
