# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import (print_function, absolute_import, unicode_literals)

from qtpy.QtCore import Qt, Signal, Slot

from mantid import logger
from mantid.api import AlgorithmManager, AnalysisDataService, ITableWorkspace, MatrixWorkspace
from mantidqt.utils.qt import import_qt
from mantidqt.widgets.plotconfigdialog.legendtabwidget import LegendProperties

from .interactive_tool import FitInteractiveTool

BaseBrowser = import_qt('.._common', 'mantidqt.widgets', 'FitPropertyBrowser')


class FitPropertyBrowserBase(BaseBrowser):
    def __init__(self, parent=None):
        super(FitPropertyBrowserBase, self).__init__(parent)
        self.init()


class FitPropertyBrowser(FitPropertyBrowserBase):
    """
    A wrapper around C++ FitPropertyBrowser with added graphical peak editing tool.
    """

    closing = Signal()

    def __init__(self, canvas, toolbar_manager, parent=None):
        super(FitPropertyBrowser, self).__init__(parent)
        self.init()
        self.setFeatures(self.DockWidgetMovable)
        self.canvas = canvas
        # The toolbar state manager to be passed to the peak editing tool
        self.toolbar_manager = toolbar_manager
        # The peak editing tool
        self.tool = None
        # The name of the fit result workspace
        self.fit_result_ws_name = ""
        # Pyplot line for the guess curve
        self.guess_line = None
        # Map the indices of the markers in the peak editing tool to the peak function prefixes
        # (in the form f0.f1...)
        self.peak_ids = {}
        self._connect_signals()

    def _connect_signals(self):
        self.xRangeChanged.connect(self.move_x_range)
        self.algorithmFinished.connect(self.fitting_done_slot)
        self.changedParameterOf.connect(self.peak_changed_slot)
        self.removeFitCurves.connect(self.clear_fit_result_lines_slot, Qt.QueuedConnection)
        self.plotGuess.connect(self.plot_guess_slot, Qt.QueuedConnection)
        self.functionChanged.connect(self.function_changed_slot, Qt.QueuedConnection)
        # Update whether data needs to be normalised when a button on the Fit menu is clicked
        self.getFitMenu().aboutToShow.connect(self._set_normalise_data_from_workspace_artist)
        self.workspaceClicked.connect(self.display_workspace)

    def _add_spectra(self, spectra):
        """
        Add spectra to the fit browser
        :param spectra: Dictionary with workspace names as keys and
                        lists of spectrum numbers as values.
        """
        for name, spec_list in spectra.items():
            self.addAllowedSpectra(name, spec_list)

    def _get_allowed_spectra(self):
        """
        Get the workspaces and spectra that can be fitted from the
        tracked workspaces.
        """
        allowed_spectra = {}
        for ax in self.canvas.figure.get_axes():
            try:
                for ws_name, artists in ax.tracked_workspaces.items():
                    spectrum_list = [artist.spec_num for artist in artists]
                    allowed_spectra[ws_name] = spectrum_list
            except AttributeError:  # scripted plots have no tracked_workspaces
                pass
        return allowed_spectra

    def _get_table_workspace(self):
        """
        Get the name of the table workspace if it exists
        """
        for ax in self.canvas.figure.get_axes():
            try:
                return ax.wsName
            except AttributeError:  # only table workspaces have wsName
                pass
        return None

    def _get_selected_workspace_artist(self):
        ws_artists_list = self.get_axes().tracked_workspaces[self.workspaceName()]
        for ws_artists in ws_artists_list:
            if ws_artists.workspace_index == self.workspaceIndex():
                return ws_artists

    def _set_normalise_data_from_workspace_artist(self):
        """
        Set if the data should be normalised before fitting using the
        normalisation state of the active workspace artist.
        """
        if AnalysisDataService.doesExist(self.workspaceName()) \
                and isinstance(AnalysisDataService.retrieve(self.workspaceName()), ITableWorkspace):
            return
        ws_artist = self._get_selected_workspace_artist()
        self.normaliseData(ws_artist.is_normalized)

    def closeEvent(self, event):
        """
        Emit self.closing signal used by figure manager to put the menu buttons
        in correct states
        """
        self.closing.emit()
        BaseBrowser.closeEvent(self, event)

    def show(self):
        """
        Override the base class method. Initialise the peak editing tool.
        """
        allowed_spectra = self._get_allowed_spectra()
        table = self._get_table_workspace()

        if allowed_spectra:
            self._add_spectra(allowed_spectra)
        elif table:
            self.addAllowedTableWorkspace(table)
        else:
            self.toolbar_manager.toggle_fit_button_checked()
            logger.warning("Cannot open fitting tool: No valid workspaces to " "fit to.")
            return

        super(FitPropertyBrowser, self).show()
        self.tool = FitInteractiveTool(self.canvas,
                                       self.toolbar_manager,
                                       current_peak_type=self.defaultPeakType())
        self.tool.fit_range_changed.connect(self.set_fit_range)
        self.tool.peak_added.connect(self.peak_added_slot)
        self.tool.peak_moved.connect(self.peak_moved_slot)
        self.tool.peak_fwhm_changed.connect(self.peak_fwhm_changed_slot)
        self.tool.peak_type_changed.connect(self.setDefaultPeakType)
        self.tool.add_background_requested.connect(self.add_function_slot)
        self.tool.add_other_requested.connect(self.add_function_slot)

        self.set_fit_bounds(self.get_fit_bounds())
        self.set_fit_range(self.tool.fit_range.get_range())

        self.setPeakToolOn(True)
        self.canvas.draw()

    def get_fit_bounds(self):
        """
        Gets the lower and upper bounds to use for the range marker tool
        """
        workspace_name = self.workspaceName()
        if AnalysisDataService.doesExist(workspace_name):
            return self.get_workspace_x_range(AnalysisDataService.retrieve(workspace_name))
        return None

    def get_workspace_x_range(self, workspace):
        """
        Gets the x limits of a workspace
        :param workspace: The workspace to get the limits from
        """
        try:
            x_data = self.getXRange()
            return x_data
        except (RuntimeError, IndexError, ValueError):
            return None

    def set_fit_bounds(self, fit_bounds):
        """
        Sets the bounds within which the range marker can be moved
        :param fit_bounds: The bounds of the fit range
        """
        if fit_bounds is not None:
            self.tool.fit_range.set_bounds(fit_bounds[0], fit_bounds[1])

    def set_fit_range(self, fit_range):
        """
        Sets the range to fit in the FitPropertyBrowser
        :param fit_range: The new fit range
        """
        if fit_range is not None:
            self.setXRange(fit_range[0], fit_range[1])

    def hide(self):
        """
        Override the base class method. Hide the peak editing tool.
        """
        if self.tool is not None:
            self.tool.fit_range_changed.disconnect()
            self.tool.disconnect()
            self.tool = None
            self.canvas.draw()
        super(FitPropertyBrowser, self).hide()
        self.setPeakToolOn(False)

    def move_x_range(self, start_x, end_x):
        """
        Let the tool know that the Fit range has been changed in the FitPropertyBrowser.
        :param start_x: New value of StartX
        :param end_x: New value of EndX
        """
        if self.tool is not None:
            new_range = sorted([start_x, end_x])
            bounds = self.tool.fit_range.get_bounds()
            if bounds[0] <= new_range[0] and bounds[1] >= new_range[1]:
                self.tool.set_fit_range(new_range[0], new_range[1])
            else:
                self.set_fit_range(self.tool.fit_range.get_range())

    def clear_fit_result_lines(self):
        """
        Delete the fit curves.
        """

        if self.fit_result_ws_name:
            ws = AnalysisDataService.retrieve(self.fit_result_ws_name)
            self.get_axes().remove_workspace_artists(ws)
        self.update_legend()

    def get_lines(self):
        """
        Get all lines in the connected plot.
        """
        return self.get_axes().get_lines()

    def get_axes(self):
        """
        Get the pyplot's Axes object.

        :rtype: matplotlib.axes.Axes
        """
        return self.canvas.figure.get_axes()[0]

    def update_legend(self):
        """
        This needs to be called to update plot's legend after removing lines.
        """
        axes = self.get_axes()
        if axes.legend_ is not None:
            props = LegendProperties.from_legend(axes.legend_)
            LegendProperties.create_legend(props, axes)

    def plot_guess(self):
        """
        Plot the guess curve.
        """
        fun = self.getFittingFunction()
        ws_name = self.workspaceName()
        if fun == '' or ws_name == '':
            return
        ws_index = self.workspaceIndex()
        startX = self.startX()
        endX = self.endX()
        out_ws_name = '{}_guess'.format(ws_name)
        workspace = AnalysisDataService.retrieve(ws_name)
        legend = self.get_axes().get_legend()

        if legend:
            legend_props = LegendProperties.from_legend(legend)

        try:
            alg = AlgorithmManager.createUnmanaged('EvaluateFunction')
            alg.setChild(True)
            alg.initialize()
            alg.setProperty('Function', fun)
            alg.setProperty('InputWorkspace', ws_name)
            if isinstance(workspace, ITableWorkspace):
                alg.setProperty('XColumn', self.getXColumnName())
                alg.setProperty('YColumn', self.getYColumnName())
                if self.getErrColumnName():
                    alg.setProperty('ErrColumn', self.getErrColumnName())
            else:
                alg.setProperty('WorkspaceIndex', ws_index)
            alg.setProperty('StartX', startX)
            alg.setProperty('EndX', endX)
            alg.setProperty('OutputWorkspace', out_ws_name)
            alg.execute()
        except RuntimeError:
            return

        out_ws = alg.getProperty('OutputWorkspace').value

        ax = self.get_axes()

        # Setting distribution=True prevents the guess being normalised
        self.guess_line = ax.plot(out_ws,
                                  wkspIndex=1,
                                  label=out_ws_name,
                                  distribution=True,
                                  update_axes_labels=False,
                                  autoscale_on_update=False)[0]

        if legend:
            LegendProperties.create_legend(legend_props, ax)

        if self.guess_line:
            self.setTextPlotGuess('Remove Guess')

        self.canvas.draw()

    def remove_guess(self):
        """
        Remove the guess curve from the plot.
        """
        if self.guess_line is None:
            return
        self.guess_line.remove()
        self.guess_line = None
        self.update_legend()
        self.setTextPlotGuess('Plot Guess')
        self.canvas.draw()

    def update_guess(self):
        """
        Update the guess curve.
        """
        if self.guess_line is None:
            return
        self.remove_guess()
        self.plot_guess()

    def add_to_menu(self, menu):
        """
        Add the relevant actions to a menu
        :param menu: A menu to hold the actions
        :return: The menu passed to us
        """
        if self.tool is not None:
            self.tool.add_to_menu(menu,
                                  peak_names=self.registeredPeaks(),
                                  current_peak_type=self.defaultPeakType(),
                                  background_names=self.registeredBackgrounds(),
                                  other_names=self.registeredOthers())
        return menu

    @Slot()
    def clear_fit_result_lines_slot(self):
        """
        Clear the fit lines.
        """
        self.clear_fit_result_lines()
        if self.tool is not None:
            self.canvas.draw()

    @Slot(str)
    def fitting_done_slot(self, name):
        """
        This is called after Fit finishes to update the fit curves.
        :param name: The name of Fit's output workspace.
        """
        self.fit_result_ws_name = name

        ax = self.get_axes()

        plot_has_legend = bool(ax.legend_)
        if plot_has_legend:
            props = LegendProperties.from_legend(ax.legend_)

        ws = AnalysisDataService.retrieve(name)

        # Keep local copy of the original lines
        original_lines = self.get_lines()

        self.clear_fit_result_lines()

        ax.plot(ws, wkspIndex=1, autoscale_on_update=False)
        if self.plotDiff():
            ax.plot(ws, wkspIndex=2, autoscale_on_update=False)

        self.addFitResultWorkspacesToTableWidget()
        # Add properties back to the lines
        new_lines = self.get_lines()
        for new_line, old_line in zip(new_lines, original_lines):
            new_line.update_from(old_line)

        if plot_has_legend:
            # Update the legend to make sure it changes to the old properties
            LegendProperties.create_legend(props, ax)

        ax.figure.canvas.draw()

    @Slot(int, float, float, float)
    def peak_added_slot(self, peak_id, centre, height, fwhm):
        """
        Respond to a signal from the peak editing tool that a peak is added.
        Add a peak function to the browser.
        :param peak_id: An index of a peak marker in the peak editing tool.
        :param centre: Peak centre
        :param height: Peak height (peak maximum)
        :param fwhm: Peak's full width at half maximum
        """
        fun = self.addFunction(self.defaultPeakType())
        self.setPeakCentreOf(fun, centre)
        self.setPeakHeightOf(fun, height)
        self.setPeakFwhmOf(fun, fwhm)
        self.peak_ids[peak_id] = fun

    @Slot(int, float, float)
    def peak_moved_slot(self, peak_id, centre, height):
        """
        Respond to the peak editing tool moving peak's top to a new position.
        :param peak_id: Peak's index/id
        :param centre: New peak centre
        :param height: New peak height
        """
        fun = self.peak_ids[peak_id]
        self.setPeakCentreOf(fun, centre)
        self.setPeakHeightOf(fun, height)
        self.update_guess()

    @Slot(int, float)
    def peak_fwhm_changed_slot(self, peak_id, fwhm):
        """
        Respond to the peak editing tool changing peak's width.
        :param peak_id: Peak's index/id
        :param fwhm: New peak full width at half maximum.
        """
        fun = self.peak_ids[peak_id]
        self.setPeakFwhmOf(fun, fwhm)
        self.update_guess()

    @Slot(str)
    def peak_changed_slot(self, fun):
        """
        Update the peak marker in the peak editing tool after peak's parameters
        change in the browser.
        :param fun: A prefix of the function that changed.
        """
        for peak_id, prefix in self.peak_ids.items():
            if prefix == fun:
                self.tool.update_peak(peak_id, self.getPeakCentreOf(prefix),
                                      self.getPeakHeightOf(prefix), self.getPeakFwhmOf(prefix))
        self.update_guess()

    @Slot(str)
    def add_function_slot(self, fun_name):
        """
        Respond to a signal from the peak editing tool to add a new function.
        :param fun_name: A registered name of a fit function
        """
        self.addFunction(fun_name)

    @Slot()
    def plot_guess_slot(self):
        """
        Toggle the guess plot.
        """
        if self.guess_line is None:
            self.plot_guess()
        else:
            self.remove_guess()

    @Slot()
    def function_changed_slot(self):
        """
        Update the peak editing tool after function structure has changed in
        the browser: functions added and/or removed.
        """
        peaks_to_add = []
        peaks = {v: k for k, v in self.peak_ids.items()}
        for prefix in self.getPeakPrefixes():
            c, h, w = self.getPeakCentreOf(prefix), self.getPeakHeightOf(
                prefix), self.getPeakFwhmOf(prefix)
            if prefix in peaks:
                self.tool.update_peak(peaks[prefix], c, h, w)
                del peaks[prefix]
            else:
                peaks_to_add.append((prefix, c, h, w))
        for i in peaks.values():
            del self.peak_ids[i]
        need_update_markers = len(peaks_to_add) > 0
        if not need_update_markers:
            plist = self.tool.get_peak_list()
            for i, c, h, w in plist:
                prefix = self.peak_ids.get(i)
                if prefix is None:
                    need_update_markers = True
                    break
                if self.getPeakCentreOf(prefix) != c or self.getPeakHeightOf(prefix) != h or\
                        self.getPeakFwhmOf(prefix) != w:
                    need_update_markers = True
                    break
        if need_update_markers:
            peak_ids, peak_updates = self.tool.update_peak_markers(self.peak_ids.keys(),
                                                                   peaks_to_add)
            self.peak_ids.update(peak_ids)
            for prefix, c, h, w in peak_updates:
                self.setPeakCentreOf(prefix, c)
                self.setPeakHeightOf(prefix, h)
                self.setPeakFwhmOf(prefix, w)
        self.update_guess()

    @Slot(str)
    def display_workspace(self, name):
        from mantidqt.widgets.workspacedisplay.matrix.presenter import MatrixWorkspaceDisplay
        from mantidqt.widgets.workspacedisplay.table.presenter import TableWorkspaceDisplay
        if AnalysisDataService.doesExist(name):
            ws = AnalysisDataService.retrieve(name)
            if isinstance(ws, MatrixWorkspace):
                presenter = MatrixWorkspaceDisplay(ws)
                presenter.show_view()
            elif isinstance(ws, ITableWorkspace):
                presenter = TableWorkspaceDisplay(ws)
                presenter.show_view()
