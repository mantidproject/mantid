# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtCore import Qt, Slot, QObject
from mantid.api import AlgorithmManager, AnalysisDataService, ITableWorkspace
from mantidqt.utils.qt import import_qt

PropertyHandler = import_qt(".._common", "mantidqt.widgets", "PropertyHandler")


class FitPropertyBrowserPlotInteraction(QObject):
    """
    Defines an interface between the FitPropertyBrowser and the Matplotlib plot
    """

    def __init__(self, fit_browser, canvas, parent=None):
        super().__init__(parent)
        self.fit_browser = fit_browser
        self.canvas = canvas

        self._connect_signals()
        # Initialise state storage
        self.guess_line = None
        self.guess_all_line = None
        self.guess_line_handlers = None
        self.guess_lines = {}

    def _connect_signals(self):
        """
        Connect this object with signals from the FitPropertyBrowser
        """
        self.fit_browser.plotGuess.connect(self.plot_guess_all_slot, Qt.QueuedConnection)
        self.fit_browser.plotCurrentGuess.connect(self.plot_current_guess_slot, Qt.QueuedConnection)
        self.fit_browser.removeCurrentGuess.connect(self.remove_current_guess_slot, Qt.QueuedConnection)
        self.fit_browser.changedParameterOf.connect(self.parameters_changed_slot, Qt.QueuedConnection)
        self.fit_browser.functionRemoved.connect(self.slot_for_function_removed)

    @Slot()
    def plot_guess_all_slot(self):
        """
        Toggle the guess all line
        """
        if self.guess_all_line:
            self.remove_guess_all()
        else:
            self.plot_guess_all()

    @Slot()
    def plot_current_guess_slot(self):
        """
        Plots the guess workspace for the current function,
        which is currently selected Handler
        """
        self.fit_browser.currentHandler().setHasPlot(True)
        self.plot_current_guess()

    @Slot()
    def remove_current_guess_slot(self):
        """
        Removes the guess workspace for the current function,
        which is currently selected Handler
        """
        self.fit_browser.currentHandler().setHasPlot(False)
        self.remove_current_guess()

    @Slot(str)
    def parameters_changed_slot(self, prefix):
        """
        Responds to parameters being changed in the FitPropertyBrowser
        :param prefix of the function which the parameters have been changed (unused)
        """
        if not self.fit_browser.currentHandler():
            return
        name = self._get_current_prefixed_function_name()
        if name in self.guess_lines:
            self.update_current_handler_guess()
        if self.guess_all_line:
            self.update_guess_all()

    @Slot()
    def slot_for_function_removed(self):
        """
        Responds to a function being removed from the FitPropertyBrowser,
        the function is found from the currently selected handler
        If the function, or guess all is plotted the plot is updated accordingly
        """
        if not self.fit_browser.currentHandler():
            return
        prefix = self.fit_browser.currentHandler().functionPrefix()
        self.remove_current_guess()
        self._shift_stored_prefixes(prefix)
        if self.guess_all_line:
            self.update_guess_all()

    def plot_current_guess(self):
        """
        Plot the guess workspace of the currently selected function
        """
        fun = self.fit_browser.currentHandler().ifun()
        ws_name = self.fit_browser.workspaceName()
        if fun == "" or ws_name == "":
            return
        out_ws_name = self._get_current_prefixed_function_name()

        line = self._plot_guess_workspace(ws_name, fun, out_ws_name)

        if line:
            self.guess_lines[self._get_current_prefixed_function_name()] = line

    def evaluate_function(self, ws_name, fun, out_ws_name):
        """
        Evaluates the guess workspace for the input workspace and function
        :param ws_name: Name of the workspace in the ADS
        :param fun: Function to be evaluated
        :param out_ws_name: Output workspace name
        :return: Output guess workspace
        """
        workspace = AnalysisDataService.retrieve(ws_name)
        alg = AlgorithmManager.createUnmanaged("EvaluateFunction")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Function", fun)
        alg.setProperty("InputWorkspace", ws_name)
        if isinstance(workspace, ITableWorkspace):
            alg.setProperty("XColumn", self.fit_browser.getXColumnName())
            alg.setProperty("YColumn", self.fit_browser.getYColumnName())
            if self.fit_browser.getErrColumnName():
                alg.setProperty("ErrColumn", self.fit_browser.getErrColumnName())
        else:
            alg.setProperty("WorkspaceIndex", self.fit_browser.workspaceIndex())
        alg.setProperty("StartX", self.fit_browser.startX())
        alg.setProperty("EndX", self.fit_browser.endX())
        alg.setProperty("IgnoreInvalidData", self.fit_browser.ignoreInvalidData())
        alg.setProperty("OutputWorkspace", out_ws_name)
        alg.execute()

        return alg.getProperty("OutputWorkspace").value

    def plot_guess_all(self):
        """
        Plot the guess workspace for the full function
        """
        fun = self.fit_browser.getFittingFunction()
        ws_name = self.fit_browser.workspaceName()
        if fun == "" or ws_name == "":
            return
        out_ws_name = f"{ws_name}_guess"

        line = self._plot_guess_workspace(ws_name, fun, out_ws_name)

        if line:
            self.guess_all_line = line
            self.fit_browser.setTextPlotGuess("Remove Guess")

    def remove_guess_all(self):
        """
        Removes the guess all workspace from the plot
        """
        if self.guess_all_line is None:
            return
        try:
            self.guess_all_line.remove()
        except ValueError:
            # line already removed
            pass
        self.guess_all_line = None
        self.update_legend()
        self.fit_browser.setTextPlotGuess("Plot Guess")
        self.canvas.draw()

    def remove_current_guess(self):
        """
        Removes the currently selected function from the plot
        """
        if not self.guess_lines:
            return
        try:
            line = self.guess_lines.pop(self._get_current_prefixed_function_name())
            line.remove()
            self.update_legend()
            self.canvas.draw()
        except (KeyError, ValueError):
            pass

    def update_guess(self):
        """
        Update the guess all curve. This function is called by the FitPropertyBrowser
        """
        if self.guess_all_line is None:
            return
        self.update_guess_all()

    def update_current_handler_guess(self):
        """
        Updates the guess workspace for the currently selected function
        """
        fun = self.fit_browser.currentHandler().ifun()
        ws_name = self.fit_browser.workspaceName()
        if fun == "" or ws_name == "":
            return
        out_ws_name = self._get_current_prefixed_function_name()
        line = self.guess_lines[self._get_current_prefixed_function_name()]
        color = line.get_color()
        try:
            line.remove()
        except ValueError:
            # line already removed
            pass
        line = self._plot_guess_workspace(ws_name, fun, out_ws_name, color=color)
        if line:
            self.guess_lines[self._get_current_prefixed_function_name()] = line

    def update_guess_all(self):
        """
        Updates the guess all workspace
        """
        fun = self.fit_browser.getFittingFunction()
        ws_name = self.fit_browser.workspaceName()
        if fun == "" or ws_name == "":
            return
        out_ws_name = f"{ws_name}_guess"
        old_line = self.guess_all_line
        color = old_line.get_color()
        try:
            old_line.remove()
        except ValueError:
            # line must have already been removed
            pass

        line = self._plot_guess_workspace(ws_name, fun, out_ws_name, color=color)
        if line is not None:
            self.guess_all_line = line

    def update_legend(self):
        """
        Updates the legend, which is necessary after removing/adding lines
        """
        ax = self.canvas.figure.get_axes()[0]
        # only create a legend if the plot already had one
        if ax.get_legend():
            ax.make_legend()

    def get_axes(self):
        """
        Get the pyplot's Axes object.
        :rtype: matplotlib.axes.Axes
        """
        return self.canvas.figure.get_axes()[0]

    def _get_current_prefixed_function_name(self):
        """
        Returns the prefixed function name, e.g f0.LinearBackground
        :return: Prefixed function name
        """
        return self.fit_browser.currentHandler().functionPrefix() + "." + self.fit_browser.currentHandler().ifun().name()

    def _plot_guess_workspace(self, workspace_name, function, output_workspace_name, **plotkwargs):
        """
        Private method which plot the guess workspace provided in the inputs
        :param workspace_name: name of the workspace stored in the fit browser
        :param function: The function to be evaluated
        :param output_workspace_name: Name of the output guess workspace
        :param plotkwargs: Optional arguments to the plot, e.g color='C1'
        :return: The matplotlib Line2D object
        """
        try:
            out_ws = self.evaluate_function(workspace_name, function, output_workspace_name)
        except (ValueError, RuntimeError):
            return

        ax = self.get_axes()
        legend = ax.get_legend()

        # Setting distribution=True prevents the guess being normalised
        line = ax.plot(
            out_ws,
            wkspIndex=1,
            label=output_workspace_name,
            distribution=True,
            update_axes_labels=False,
            autoscale_on_update=False,
            **plotkwargs,
        )[0]
        if legend:
            ax.make_legend()

        self.canvas.draw()

        return line

    def _shift_stored_prefixes(self, removed_prefix):
        """
        Shifts the stored prefixes, e.g if f0.A, f1.B, f2.C plotted and f1.B removed
        stored prefixes become f0.A, f1.C. Skips if no guess lines plotted,
        or removing last function in browser
        :param removed_prefix: Prefix of the function removed e.g f0
        :return:
        """
        if not self.guess_lines:
            return
        if len(removed_prefix) < 2:
            return
        if removed_prefix[1] == len(self.guess_lines):
            return
        for prefixed_function in reversed(list(self.guess_lines)):
            prefix_and_function = prefixed_function.split(".")
            if int(prefix_and_function[0][1]) > int(removed_prefix[1]):
                new_function_label = "f" + str(int(prefix_and_function[0][1]) - 1) + "." + prefix_and_function[1]
                line = self.guess_lines.pop(prefixed_function)
                self.guess_lines[new_function_label] = line
                line.set_label(new_function_label)

        self._refresh_legend()

    def _refresh_legend(self):
        """Refreshes the legend of the plot.
        Necessary if we change the line label for example (using set_label)
        """
        ax = self.get_axes()
        legend = ax.get_legend()
        if legend:
            ax.make_legend()
        self.canvas.draw()
