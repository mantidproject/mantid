# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
"""
Controls the dynamic displaying of errors for line on the plot
"""
from functools import partial
from qtpy.QtWidgets import QMenu

from mantid.plots import MantidAxes
from mantid.plots.utility import MantidAxKwargs, find_errorbar_container
from mantid.simpleapi import mtd


class FigureErrorsManager(object):
    ERROR_BARS_MENU_TEXT = "Y Error Bars"
    SHOW_ERROR_BARS_BUTTON_TEXT = "Show all errors"
    HIDE_ERROR_BARS_BUTTON_TEXT = "Hide all errors"

    def __init__(self, canvas):
        self.canvas = canvas

    def add_error_bars_menu(self, menu):
        """
        Add menu actions to toggle the errors for all lines in the plot.

        :param menu: The menu to which the actions will be added
        :type menu: QMenu
        """
        ax = self.canvas.figure.axes[0]  # type: matplotlib.axes.Axes
        # if the ax is not a MantidAxes, and there is no errors plotted,
        # then do not add any options for the menu
        # because the errors cannot be toggled for plots that aren't of workspaces
        containers = ax.containers
        if not isinstance(ax, MantidAxes) and len(containers) == 0:
            return

        # if the ax is not supported we cannot add errors to the plot,
        # but we can toggle existing ones
        ax_is_supported = self._supported_ax(ax)

        lines = ax.lines

        error_bars_menu = QMenu(self.ERROR_BARS_MENU_TEXT, menu)
        # if there's more than one line plotted, then
        # add a sub menu, containing an action to hide the
        # error bar for each line
        error_bars_menu.addAction(self.SHOW_ERROR_BARS_BUTTON_TEXT,
                                  partial(self._update_plot_after,
                                          self.toggle_all_error_bars, make_visible=True))
        error_bars_menu.addAction(self.HIDE_ERROR_BARS_BUTTON_TEXT,
                                  partial(self._update_plot_after,
                                          self.toggle_all_error_bars, make_visible=False))
        menu.addMenu(error_bars_menu)
        if len(lines) > 1:
            # add into the correct menu,
            # if there is a single line, then
            # add the action to the main menu

            for index, line in enumerate(lines):
                if line.get_label() == MantidAxes.MPL_NOLEGEND:
                    self._add_line_with_existing_errors(containers, error_bars_menu, index, line)
                else:
                    self._add_line_without_errors(ax_is_supported, error_bars_menu, index, line)

    def _add_line_with_existing_errors(self, containers, error_bars_menu, index, line):
        errorbar_container = find_errorbar_container(line, containers)

        # the line had no label, but it does not have errors either
        # if errorbar_container is None:
        #     return

        y_err_index = self._find_yerr_index(errorbar_container)
        if y_err_index is not None:
            label = errorbar_container.get_label()
            action = error_bars_menu.addAction(label, partial(self._update_plot_after,
                                                              self._toggle_error_bar_for, index))
            action.setCheckable(True)
            action.setChecked(errorbar_container[2][y_err_index].get_visible())

    def _add_line_without_errors(self, ax_is_supported, error_bars_menu, index, line):
        # this line has no errorbar, so the label is contained here
        label = line.get_label()
        # only add the toggle for a line that does NOT contain errors
        # if we are operating on a MantidAxes
        if ax_is_supported:
            action = error_bars_menu.addAction(label, partial(self._update_plot_after,
                                                              self._toggle_error_bar_for, index))  # type:QAction
            # lines being added here are guaranteed to not have errors at all
            action.setCheckable(True)
            action.setChecked(False)

    def add_errorbar_for(self, index, make_visible=None):
        """
        Adds an errorbar for the line at the provided `index`.
        This will plot a new line by calling the `axes.errorbar` function,
        then the old line will be replaced in the plot, and deleted.

        :param index: Current index of the line in `lines` and `creation_args`.
                      Used to swap the newly added errorbar line with the old line
        :type index: int
        :param make_visible:
        :type make_visible: bool
        """
        ax = self.canvas.figure.axes[0]  # type: MantidAxes

        # To do the errorbar plot we need to get the
        # original workspace for the axis so that the data can be read from it
        ws = mtd[ax.creation_args[index]["workspaces"]]
        specNum = ax.creation_args[index]["specNum"]

        # if a forced state is passed, use that for the error plotting
        state_kwarg = {} if make_visible is None else {MantidAxKwargs.ERRORS_VISIBLE: make_visible}
        # Plots the spectrum with errors on the same plot.
        # This will append it to the bottom of the `lines` and `containers` lists.
        errorbar_container = ax.errorbar(ws, specNum=specNum, **state_kwarg)

        lines = ax.lines
        # change the color of the new data & error lines to match the original
        errorbar_container[0].set_color(lines[index].get_color())
        errorbar_container[2][0].set_color(lines[index].get_color())

        # The swaps are done do pretend the line was replaced 'inplace'
        # this keeps the legend order the same.

        # swap in .lines, remove the old line reference
        lines[index], lines[-1] = lines[-1], lines[index]
        # delete the reference to the old line
        del lines[-1]

        # swap in .creation_args, remove the old args reference
        cargs = self.canvas.figure.axes[0].creation_args
        cargs[index], cargs[-1] = cargs[-1], cargs[index]

        # add the creation args default argument,
        # use the make_visible, or set to True by default
        cargs[index][MantidAxKwargs.ERRORS_VISIBLE] = True if make_visible is None else make_visible
        # delete the reference to the old creation args
        del cargs[-1]

    def hide_error_bar_for(self, index):
        """
        Hides the errors for the line at `index`.
        This index is the position in the `axes.lines` list.

        This function does not update the plot after it's done any changes.

        :param index: Current index of the line in `lines` and `creation_args`.
                      Used to swap the newly added errorbar line with the old line
        :type index: int
        """
        self._toggle_error_bar_for(index, make_visible=False)

    def show_error_bar_for(self, index):
        """
        Shows the errors for the line at `index`.
        This index is the position in the `axes.lines` list.

        This function does not update the plot after it's done any changes.

        :param index: Current index of the line in `lines` and `creation_args`.
                      Used to swap the newly added errorbar line with the old line
        :type index: int
        """
        self._toggle_error_bar_for(index, make_visible=True)

    def toggle_all_error_bars(self, make_visible=None):
        """
        Iterates through all lines in the plot and toggles the error visibility.

        This function does not update the plot after it's done any changes.

        :param make_visible: Force visibility state on all lines.
        :type make_visible: bool
        """
        ax = self.canvas.figure.axes[0]  # type: MantidAxes
        lines = ax.lines

        # iterates over all lines, in order to
        # toggle error bar on lines that have errors already
        for index, line in enumerate(lines):
            # extract the line reference from the container
            if line.get_label() == MantidAxes.MPL_NOLEGEND:
                self._toggle_error_bar_for(index, make_visible)

        # errors can't be added dynamically while working with axes that aren't supported
        if not self._supported_ax(ax):
            return

        # Iterate over all lines to add new error bars.
        # Each added errorbar changes the lines list, making the iteration not just 1..n.
        # The line is always removed from pos `index`,
        # and errors are added at the end of the list, the next element is moved 1 position up.
        # Therefore the index should only be increased
        # if the line it is looking for already has errors plotted
        index = 0
        while index < len(lines):
            line = lines[index]
            # line doesn't have errors, add them
            # this will remove the line from the current index,
            # and move the next one to the current index
            if line.get_label() != MantidAxes.MPL_NOLEGEND:
                self.add_errorbar_for(index, make_visible)
            else:
                # the line has errors, move forwards in the iteration
                index += 1

    def _toggle_error_bar_for(self, index, make_visible=None):
        """
        :param make_visible: Forces this visibility state on all lines.
        :type make_visible: bool
        :param index: The index in the creation_args
                                    that contains the information about recreating the errorbar
        :type index: int
        :return:
        """
        ax = self.canvas.figure.axes[0]  # type: MantidAxes

        errorbar_container = find_errorbar_container(ax.lines[index], ax.containers)
        if errorbar_container is None:
            if not isinstance(ax, MantidAxes):
                # The public interface should NOT allow adding errors for lines that
                # are not plotted on MantidAxes, as there is no way of obtaining that information.
                raise NotImplementedError(
                    "This function does not know how to add errors for a line plotted on axes that are not MantidAxes, "
                    "and should not be called from the public interface with axes that are not MantidAxes.")
            # make the error line invisible, because the `not` below will flip it and make it visible
            # and if a forced state is provided that will overwrite it anyway
            self.add_errorbar_for(index, make_visible=False)
            errorbar_container = find_errorbar_container(ax.lines[index], ax.containers)

        # Currently the X errors are not touched at all.
        # Only operations on the Y errors are done.
        y_err_index = self._find_yerr_index(errorbar_container)

        if y_err_index is not None:
            error_line = errorbar_container.lines[2][y_err_index]
            # the container has the errors there
            # if a force_state is passed, use that, otherwise invert the current state
            new_state = not error_line.get_visible() if make_visible is None else make_visible

            # updates the creation args state if the plot would be saved out
            if self._supported_ax(ax):
                creation_args = ax.creation_args
                print("These are the current creation_args:", creation_args)
                creation_args[index][MantidAxKwargs.ERRORS_VISIBLE] = new_state

            error_line.set_visible(new_state)

    def _find_yerr_index(self, errorbar_container):
        """
        Finds the correct index for Y errors,
        depending on whether X errors are present.

        :param errorbar_container: The container which would contain the error lines.
        :return: The index of the Y errors, or None if no Y errors are present
        """

        if errorbar_container.has_yerr and errorbar_container.has_xerr:
            y_err_index = 1
        elif errorbar_container.has_yerr and not errorbar_container.has_xerr:
            y_err_index = 0
        else:
            y_err_index = None
        return y_err_index

    def _update_plot_after(self, func, *args, **kwargs):
        """
        Updates the legend and the plot after the function has been executed.
        Used to funnel through the updates through a common place

        :param func: Function to be executed, before updating the plot
        :param args: Arguments forwarded to the function
        :param kwargs: Keyword arguments forwarded to the function
        """
        func(*args, **kwargs)
        self.canvas.figure.axes[0].legend().draggable()
        self.canvas.draw()

    @staticmethod
    def _supported_ax(ax):
        return hasattr(ax, 'creation_args')
