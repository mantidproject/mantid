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

HELP_URL = "qthelp://org.mantidproject/doc/tutorials/mantid_basic_course/loading_and_displaying_data/06_formatting_plots.html"


class PlotConfigDialogPresenter:
    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        if view:
            self.view = view
        else:
            self.view = PlotConfigDialogView(parent)

        self.tab_widget_presenters = [None, None, None, None]
        self.tab_widget_views = [None, None, None, None]
        legend_tab = None
        # Axes tab
        if len(self.fig.get_axes()) > 0:
            axes_tab = AxesTabWidgetPresenter(
                self.fig, parent=self.view, success_callback=self.success_callback, error_callback=self.error_callback
            )
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
        self.view.show()

        # Signals
        self.view.ok_button.clicked.connect(self.apply_properties_and_exit)
        self.view.apply_button.clicked.connect(self.apply_properties)
        self.view.cancel_button.clicked.connect(self.exit)
        self.view.help_button.clicked.connect(self.open_help_window)

        self.error_state = False

    def _add_tab_widget_views(self):
        for tab_view in self.tab_widget_views:
            if tab_view:
                self.view.add_tab_widget(tab_view)

    def apply_properties(self):
        """Attempts to apply properties. Returns a bool denoting whether
        there was an error drawing the canvas drawn"""
        for tab in reversed(self.tab_widget_presenters):
            if tab:
                tab.apply_properties()
        try:
            self.fig.canvas.draw()
        except Exception as exception:
            self.error_callback(str(exception))
            return

        for tab in reversed(self.tab_widget_presenters):
            if tab == self.tab_widget_presenters[0]:
                # Do not call update view on the legend tab because the legend config
                # does not depend on the curves or axes - it only changes how the legend
                # is displayed (e.g. legend fonts, border color, etc.)
                continue
            if tab:
                tab.update_view()

        self.success_callback()

    def apply_properties_and_exit(self):
        self.apply_properties()
        if self.error_state:
            # If we did exit with an error, the plot would be 'damaged', as canvas.draw()
            # did not get to finish drawing, because it raised an exception part way through.
            return
        self.exit()

    def exit(self):
        self.view.close()

    def open_help_window(self):
        # Show the help documentation relevant to the plot type.
        if self.tab_widget_presenters[3] is not None:
            # If the dialog has the images tab then go to the section on image plots.
            InterfaceManager().showHelpPage(HELP_URL + "#image-plots")
        else:
            InterfaceManager().showHelpPage(HELP_URL + "#figureoptionsgear-png-ptions-menu")

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

    def configure_curves_tab(self, axes, curve):
        curves_tab_presenter = self.tab_widget_presenters[2]
        if not curves_tab_presenter:
            return
        curves_tab_widget, _ = self.tab_widget_views[1]

        try:
            curves_tab_presenter.set_axes_from_object(axes)
        except ValueError as err:
            if str(err) == "Axes object does not exist in curves tab":
                # This can happen when there are no curves on the given axes.
                return
            raise err

        self.view.set_current_tab_widget(curves_tab_widget)

        if curve:
            try:
                curves_tab_presenter.set_curve_from_object(curve)
            except ValueError as err:
                if str(err) == "Curve object does not exist in curves tab":
                    return
                raise err

    def error_callback(self, error_message):
        """To be called by self or a child presenter when there is an error"""
        self.view.set_error_text(error_message)
        self.error_state = True

    def success_callback(self):
        """To be called by self or a child presenter to report a success, clearing the error."""
        self.view.set_error_text(None)
        self.error_state = False
