# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from qtpy.QtCore import Qt, Signal, Slot

import matplotlib.pyplot
from mantid import logger
from mantid.api import AlgorithmManager, AnalysisDataService, ITableWorkspace, MatrixWorkspace
from mantidqt.plotting.functions import plot
from mantidqt.utils.qt import import_qt
from .fitpropertybrowserplotinteraction import FitPropertyBrowserPlotInteraction
from .interactive_tool import FitInteractiveTool
import numpy as np

BaseBrowser = import_qt(".._common", "mantidqt.widgets", "FitPropertyBrowser")


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
        self.setFeatures(self.DockWidgetMovable)
        self.canvas = canvas
        self.plot_interaction_manager = FitPropertyBrowserPlotInteraction(self, canvas)
        # The toolbar state manager to be passed to the peak editing tool
        self.toolbar_manager = toolbar_manager
        # The peak editing tool
        self.tool = None
        # The name of the fit result workspace
        self.fit_result_ws_name = ""
        # Pyplot line for the sequential fit curve
        self.sequential_fit_line = None
        # Map the indices of the markers in the peak editing tool to the peak function prefixes
        # (in the form f0.f1...)
        self.peak_ids = {}
        self._connect_signals()
        self.canvas_draw_signal = None
        self.allowed_spectra = {}

    def _connect_signals(self):
        self.xRangeChanged.connect(self.move_x_range)
        self.algorithmFinished.connect(self.fitting_done_slot)
        self.changedParameterOf.connect(self.peak_changed_slot)
        self.removeFitCurves.connect(self.clear_fit_result_lines_slot, Qt.QueuedConnection)
        self.functionChanged.connect(self.function_changed_slot, Qt.QueuedConnection)
        # Update whether data needs to be normalised when a button on the Fit menu is clicked
        self.getFitMenu().aboutToShow.connect(self._set_normalise_data)
        self.sequentialFitDone.connect(self._sequential_fit_done_slot)
        self.workspaceClicked.connect(self.display_workspace)

    def _update_workspace_info_slot(self, event):
        "Callback when the matplotlib canvas updates"
        if not self._update_workspace_info():
            self.hide()

    def _update_workspace_info(self):
        "Update the allowed spectra/tableworkspace in the fit browser"
        allowed_spectra_old = self.allowed_spectra
        allowed_spectra = self._get_allowed_spectra()
        table = self._get_table_workspace()
        if allowed_spectra:
            self._update_spectra(allowed_spectra, allowed_spectra_old)
        elif table:
            self.addAllowedTableWorkspace(table)
        else:
            self.toolbar_manager.toggle_fit_button_checked()
            logger.warning("Cannot use fitting tool: No valid workspaces to fit to.")
            return False
        return True

    def _update_spectra(self, allowed_spectra, allowed_spectra_old):
        previous_workspace_index = self.workspaceIndex()
        previous_name = self.workspaceName()
        # remove workspaces that are no longer plotted
        for workspace in np.setdiff1d(list(allowed_spectra_old.keys()), list(allowed_spectra.keys())):
            self.removeWorkspaceAndSpectra(workspace)
        self._add_spectra(allowed_spectra)
        if (self.workspaceName(), self.workspaceIndex()) != (previous_name, previous_workspace_index):
            self.clear_fit_result_lines()

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
        output_wsnames = [self.getWorkspaceList().item(ii).text() for ii in range(self.getWorkspaceList().count())]
        for ax in self.canvas.figure.get_axes():
            try:
                for ws_name, artists in ax.tracked_workspaces.items():
                    # we don't want to include the fit workspace in our selection
                    if ws_name in output_wsnames:
                        continue
                    # loop through arists and get relevant spec numbers if spec_num represents a spectrum as opposed to a bin axes.
                    spectrum_list = [artist.spec_num for artist in artists if artist.is_spec]

                    if spectrum_list:
                        spectrum_list = sorted(list(set(spectrum_list)))
                        allowed_spectra[ws_name] = spectrum_list
            except AttributeError:  # scripted plots have no tracked_workspaces
                pass
        self.allowed_spectra = allowed_spectra

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

    def _set_normalise_data(self):
        """
        Set if the data should be normalised before fitting using the
        normalisation state of the active workspace artist.
        If the workspace is a distribution normalisation is set to False so it is not normalised twice.
        """
        ws_is_distribution = False
        if AnalysisDataService.doesExist(self.workspaceName()):
            workspace = AnalysisDataService.retrieve(self.workspaceName())
            if isinstance(workspace, ITableWorkspace):
                return
            if hasattr(workspace, "isDistribution") and workspace.isDistribution():
                ws_is_distribution = True
        ws_artist = self._get_selected_workspace_artist()
        should_normalise_before_fit = ws_artist.is_normalized and not ws_is_distribution
        self.normaliseData(should_normalise_before_fit)

    def _set_peak_initial_fwhm(self, fun, fwhm):
        """
        Overwrite fwhm if has not been set already - this is for back to back exponential type funcs
        which have had the width parameter (S) as func d-spacing refined for a standard sample (coefs stored in
        instrument Paramters.xml) and has already been set.
        :param fun: peak function prefix
        :param fwhm: estimated fwhm of peak added
        """
        if not self.getWidthParameterNameOf(fun) or not self.isParameterExplicitlySetOf(fun, self.getWidthParameterNameOf(fun)):
            self.setPeakFwhmOf(fun, fwhm)

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
            logger.warning("Cannot open fitting tool: No valid workspaces to fit to.")
            return

        self.canvas_draw_signal = self.canvas.mpl_connect("draw_event", self._update_workspace_info_slot)
        super(FitPropertyBrowser, self).show()
        self.tool = FitInteractiveTool(
            self.canvas, self.toolbar_manager, current_peak_type=self.defaultPeakType(), default_background=self.defaultBackgroundType()
        )
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
        self.set_output_window_names()

    def set_output_window_names(self):
        import matplotlib.pyplot as plt  # unfortunately need to import again

        """
        Change the output name if more than one plot of the same workspace
        """
        window_title = self.canvas.manager.get_window_title()
        workspace_name = window_title.rsplit("-", 1)[0]
        for open_figures in plt.get_figlabels():
            if open_figures != window_title and open_figures.rsplit("-", 1)[0] == workspace_name:
                self.setOutputName(window_title)
        return None

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

        if self.canvas_draw_signal is not None:
            self.canvas.mpl_disconnect(self.canvas_draw_signal)
            self.canvas_draw_signal = None

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
        for line in self.get_lines():
            if line.get_label() == self.sequential_fit_line:
                line.remove()

        if self.fit_result_ws_name:
            if AnalysisDataService.doesExist(self.fit_result_ws_name):
                ws = AnalysisDataService.retrieve(self.fit_result_ws_name)
                self.get_axes().remove_workspace_artists(ws)
            self.fit_result_ws_name = ""
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
        ax = self.get_axes()
        # only create a legend if the plot already had one
        if ax.get_legend():
            ax.make_legend()

    def evaluate_function(self, ws_name, fun, out_ws_name):
        workspace = AnalysisDataService.retrieve(ws_name)
        alg = AlgorithmManager.createUnmanaged("EvaluateFunction")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Function", fun)
        alg.setProperty("InputWorkspace", ws_name)
        if isinstance(workspace, ITableWorkspace):
            alg.setProperty("XColumn", self.getXColumnName())
            alg.setProperty("YColumn", self.getYColumnName())
            if self.getErrColumnName():
                alg.setProperty("ErrColumn", self.getErrColumnName())
        else:
            alg.setProperty("WorkspaceIndex", self.workspaceIndex())
        alg.setProperty("StartX", self.startX())
        alg.setProperty("EndX", self.endX())
        alg.setProperty("IgnoreInvalidData", self.ignoreInvalidData())
        alg.setProperty("OutputWorkspace", out_ws_name)
        alg.execute()

        return alg.getProperty("OutputWorkspace").value

    def add_to_menu(self, menu):
        """
        Add the relevant actions to a menu
        :param menu: A menu to hold the actions
        :return: The menu passed to us
        """
        if self.tool is not None:
            self.tool.add_to_menu(
                menu,
                peak_names=self.registeredPeaks(),
                current_peak_type=self.defaultPeakType(),
                background_names=self.registeredBackgrounds(),
                other_names=self.registeredOthers(),
            )
        return menu

    def do_plot(self, ws, plot_diff=False, **plot_kwargs):
        ax = self.get_axes()

        self.clear_fit_result_lines()

        # Keep local copy of the original lines
        original_lines = self.get_lines()

        ax.plot(ws, wkspIndex=1, autoscale_on_update=False, **plot_kwargs)
        if plot_diff:
            ax.plot(ws, wkspIndex=2, autoscale_on_update=False, **plot_kwargs)

        self.addFitResultWorkspacesToTableWidget()
        # Add properties back to the lines
        new_lines = self.get_lines()
        for new_line, old_line in zip(new_lines, original_lines):
            new_line.update_from(old_line)

        if ax.get_legend():
            # Update the legend to make sure it changes to the old properties
            ax.make_legend()

        ax.figure.canvas.draw()

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

        ws = AnalysisDataService.retrieve(name)
        self.do_plot(ws, plot_diff=self.plotDiff())
        self.fit_result_ws_name = name

    @Slot()
    def _sequential_fit_done_slot(self):
        fun = self.getFittingFunction()
        ws_name = self.workspaceName()
        out_ws_name = self.outputName() + "_res"

        try:
            out_ws = self.evaluate_function(ws_name, fun, out_ws_name)
        except RuntimeError:
            return

        plot_kwargs = {"label": out_ws_name}
        self.do_plot(out_ws, plot_diff=False, **plot_kwargs)
        self.sequential_fit_line = out_ws_name

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
        self._set_peak_initial_fwhm(fun, fwhm)
        if height != 0:
            self.setPeakHeightOf(fun, height)
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
        self.plot_interaction_manager.update_guess()

    @Slot(int, float)
    def peak_fwhm_changed_slot(self, peak_id, fwhm):
        """
        Respond to the peak editing tool changing peak's width.
        :param peak_id: Peak's index/id
        :param fwhm: New peak full width at half maximum.
        """
        fun = self.peak_ids[peak_id]
        self.setPeakFwhmOf(fun, fwhm)
        self.plot_interaction_manager.update_guess()

    @Slot(str)
    def peak_changed_slot(self, fun):
        """
        Update the peak marker in the peak editing tool after peak's parameters
        change in the browser.
        :param fun: A prefix of the function that changed.
        """
        for peak_id, prefix in self.peak_ids.items():
            if prefix == fun:
                self.tool.update_peak(peak_id, self.getPeakCentreOf(prefix), self.getPeakHeightOf(prefix), self.getPeakFwhmOf(prefix))
        self.plot_interaction_manager.update_guess()

    @Slot(str)
    def add_function_slot(self, fun_name):
        """
        Respond to a signal from the peak editing tool to add a new function.
        :param fun_name: A registered name of a fit function
        """
        self.addFunction(fun_name)

    @Slot()
    def function_changed_slot(self):
        """
        Update the peak editing tool after function structure has changed in
        the browser: functions added and/or removed.
        """
        peaks_to_add = []
        peaks = {v: k for k, v in self.peak_ids.items()}
        for prefix in self.getPeakPrefixes():
            c, h, w = self.getPeakCentreOf(prefix), self.getPeakHeightOf(prefix), self.getPeakFwhmOf(prefix)
            if w > (self.endX() - self.startX()):
                logger.warning("Peak FWHM > X range. Width markers will not be shown")
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
                if self.getPeakCentreOf(prefix) != c or self.getPeakHeightOf(prefix) != h or self.getPeakFwhmOf(prefix) != w:
                    need_update_markers = True
                    break
        if need_update_markers:
            peak_ids, peak_updates = self.tool.update_peak_markers(self.peak_ids.keys(), peaks_to_add)
            self.peak_ids.update(peak_ids)
            for prefix, c, h, w in peak_updates:
                self.setPeakCentreOf(prefix, c)
                self.setPeakFwhmOf(prefix, w)
                self.setPeakHeightOf(prefix, h)
        self.plot_interaction_manager.update_guess()

    @Slot(str)
    def display_workspace(self, name):
        from mantidqt.widgets.workspacedisplay.matrix.presenter import MatrixWorkspaceDisplay
        from mantidqt.widgets.workspacedisplay.table.presenter import TableWorkspaceDisplay

        if AnalysisDataService.doesExist(name):
            ws = AnalysisDataService.retrieve(name)
            if isinstance(ws, MatrixWorkspace):
                presenter = MatrixWorkspaceDisplay(ws, plot=plot)
                presenter.show_view()
            elif isinstance(ws, ITableWorkspace):
                presenter = TableWorkspaceDisplay(ws, plot=matplotlib.pyplot, batch=True)
                presenter.show_view()
