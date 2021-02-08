# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from mantidqt.interfacemanager import InterfaceManager
from mantidqt.widgets.plotconfigdialog import curve_in_figure, image_in_figure, legend_in_figure
from mantidqt.widgets.plotconfigdialog.view import PlotConfigDialogView
from mantidqt.widgets.plotconfigdialog.axestabwidget.presenter import AxesTabWidgetPresenter
from mantidqt.widgets.plotconfigdialog.curvestabwidget.presenter import CurvesTabWidgetPresenter
from mantidqt.widgets.plotconfigdialog.imagestabwidget.presenter import ImagesTabWidgetPresenter
from mantidqt.widgets.plotconfigdialog.legendtabwidget.presenter import LegendTabWidgetPresenter

HELP_URL = 'qthelp://org.mantidproject/doc/tutorials/mantid_basic_course/loading_and_displaying_data/' \
           '06_formatting_plots.html'


class PlotConfigDialogPresenter:

    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        if view:
            self.view = view
        else:
            self.view = PlotConfigDialogView(parent)
        self.view.show()

        self.tab_widget_presenters = [None, None, None, None]
        self.tab_widget_views = [None, None, None, None]
        legend_tab = None
        # Axes tab
        if len(self.fig.get_axes()) > 0:
            axes_tab = AxesTabWidgetPresenter(self.fig, parent=self.view)
            self.tab_widget_presenters[1] = axes_tab
            self.tab_widget_views[0] = (axes_tab.view, "Axes")
        # Legend tab
        if legend_in_figure(self.fig):
            legend_tab = LegendTabWidgetPresenter(self.fig, parent_view=self.view, parent_presenter=self)
            self.tab_widget_presenters[0] = legend_tab
            self.tab_widget_views[3] = (legend_tab.view, "Legend")
        # Curves tab
        if curve_in_figure(self.fig):
            curves_tab = CurvesTabWidgetPresenter(self.fig, parent_view=self.view, parent_presenter=self, legend_tab=legend_tab)
            self.tab_widget_presenters[2] = curves_tab
            self.tab_widget_views[1] = (curves_tab.view, "Curves")
        # Images tab
        if image_in_figure(self.fig):
            images_tab = ImagesTabWidgetPresenter(self.fig, parent=self.view)
            self.tab_widget_presenters[3] = images_tab
            self.tab_widget_views[2] = (images_tab.view, "Images")

        self._add_tab_widget_views()

        # Signals
        self.view.ok_button.clicked.connect(self.apply_properties_and_exit)
        self.view.apply_button.clicked.connect(self.apply_properties)
        self.view.cancel_button.clicked.connect(self.exit)
        self.view.help_button.clicked.connect(self.open_help_window)

    def _add_tab_widget_views(self):
        for tab_view in self.tab_widget_views:
            if tab_view:
                self.view.add_tab_widget(tab_view)

    def apply_properties(self):
        for tab in reversed(self.tab_widget_presenters):
            if tab:
                tab.apply_properties()
        self.fig.canvas.draw()
        for tab in reversed(self.tab_widget_presenters):
            if tab == self.tab_widget_presenters[0]:
                # Do not call update view on the legend tab because the legend config
                # does not depend on the curves or axes - it only changes how the legend
                # is displayed (e.g. legend fonts, border color, etc.)
                continue
            if tab:
                tab.update_view()

    def apply_properties_and_exit(self):
        self.apply_properties()
        self.exit()

    def exit(self):
        self.view.close()

    def open_help_window(self):
        # Show the help documentation relevant to the plot type.
        if self.tab_widget_presenters[3] is not None:
            # If the dialog has the images tab then go to the section on image plots.
            InterfaceManager().showHelpPage(HELP_URL + '#image-plots')
        else:
            InterfaceManager().showHelpPage(HELP_URL + '#figureoptionsgear-png-ptions-menu')

    def forget_tab_from_presenter(self, tab_presenter):
        """Given the presenter of a tab, forgets the tab's presenter and view
        by setting them to none, for example when the tab view is closed."""
        for index, view_name_pair in enumerate(self.tab_widget_views):
            if view_name_pair:
                view, name = view_name_pair
                if view == tab_presenter.view:
                    self.tab_widget_views[index] = None
                    break

        for index, presenter in enumerate(self.tab_widget_presenters):
            if presenter == tab_presenter:
                self.tab_widget_presenters[index] = None
                return
