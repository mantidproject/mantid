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

    AXES_NOT_MANTIDAXES_ERR_MESSAGE = "Plot axes are not MantidAxes. There is no way to automatically load error data."

    def __init__(self, canvas):
        self.canvas = canvas

    @staticmethod
    def get_data_lines(ax):
        """
        Gets all the lines that are not used to draw caps on error lines.

        :param ax: The Ax from where the lines will be inspected
        :return: A new list containing the references to the lines that are used to plot data.
                 This list has the same order as creation_args.
        """
        all_cap_lines = []

        for container in ax.containers:
            # subtract lines that are used to draw line caps
            all_cap_lines.extend(container[1])

        # converting to set makes the `if item not in` check constant time
        all_cap_lines_set = set(all_cap_lines)
        lines = [item for item in ax.lines if item not in all_cap_lines_set]

        return lines

    def iterate_data_lines(self, ax):
        lines = self.get_data_lines(ax)

        for index, line in enumerate(lines):
            yield index, line

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

        error_bars_menu = QMenu(self.ERROR_BARS_MENU_TEXT, menu)
        error_bars_menu.addAction(self.SHOW_ERROR_BARS_BUTTON_TEXT,
                                  partial(self._update_plot_after,
                                          self.toggle_all_error_bars, make_visible=True))
        error_bars_menu.addAction(self.HIDE_ERROR_BARS_BUTTON_TEXT,
                                  partial(self._update_plot_after,
                                          self.toggle_all_error_bars, make_visible=False))
        menu.addMenu(error_bars_menu)

        data_lines = self.get_data_lines(ax)

        # if there's more than one line plotted, then
        # add a sub menu, containing an action to hide the
        # error bar for each line
        if len(data_lines) > 1:
            for index, line in enumerate(data_lines):
                if line.get_label() == MantidAxes.MPL_NOLEGEND:
                    self._add_line_with_existing_errors(containers, error_bars_menu, index, line)
                else:
                    self._add_line_without_errors(ax_is_supported, error_bars_menu, index, line)

    def _add_line_with_existing_errors(self, containers, error_bars_menu, index, line):
        errorbar_container = find_errorbar_container(line, containers)

        # the line had no label, but it does not have errors either
        if errorbar_container is None:
            return

        y_err_index = self._find_yerr_index(errorbar_container)
        if y_err_index is not None:
            label = errorbar_container.get_label()
            action = error_bars_menu.addAction(label, partial(self._update_plot_after,
                                                              self._toggle_error_bar_for, index, errorbar_container))
            action.setCheckable(True)
            action.setChecked(errorbar_container[2][y_err_index].get_visible())

    def _add_line_without_errors(self, ax_is_supported, error_bars_menu, index, line):
        # this line has no errorbar, so the label is contained here
        label = line.get_label()
        # only add the toggle for a line that does NOT contain errors
        # if we are operating on a MantidAxes
        if ax_is_supported:
            action = error_bars_menu.addAction(label, partial(self._update_plot_after,
                                                              self._add_errorbar_for, index, line))  # type: QAction
            # lines being added here are guaranteed to not have errors at all
            action.setCheckable(True)
            action.setChecked(False)

    def _add_errorbar_for(self, creation_args_index, line, make_visible=None):
        """
        Adds an errorbar for the line at the provided `index`.
        This will plot a new line by calling the `axes.errorbar` function,
        then the old line will be replaced in the plot, and deleted.

        :param creation_args_index: Current index of the line in `lines` and `creation_args`.
                      Used to swap the newly added errorbar line with the old line
        :type creation_args_index: int
        :param make_visible:
        :type make_visible: bool

        :rtype ErrorbarContainer
        """
        ax = self.canvas.figure.axes[0]  # type: matplotlib.axes.Axes

        # To do the errorbar plot we need to get the
        # original workspace for the axis so that the data can be read from it
        ws = mtd[ax.creation_args[creation_args_index]["workspaces"]]
        specNum = ax.creation_args[creation_args_index]["specNum"]

        # if a forced state is passed, use that for the error plotting
        state_kwarg = {} if make_visible is None else {MantidAxKwargs.ERRORS_VISIBLE: make_visible}
        # Plots the spectrum with errors on the same plot.
        # This will append it to the bottom of the `lines` and `containers` lists.
        errorbar_container = ax.errorbar(ws, specNum=specNum, **state_kwarg)

        # change the color of the new data & error lines to match the original
        errorbar_container[0].set_color(line.get_color())
        errorbar_container[2][0].set_color(line.get_color())

        # swap in .creation_args, remove the old args reference
        cargs = self.canvas.figure.axes[0].creation_args
        cargs[creation_args_index], cargs[-1] = cargs[-1], cargs[creation_args_index]

        # add the creation args default argument,
        # use the make_visible, or set to True by default
        cargs[creation_args_index][MantidAxKwargs.ERRORS_VISIBLE] = True if make_visible is None else make_visible
        # delete the reference to the old creation args
        del cargs[-1]

        # The swaps are done to pretend the line was replaced 'inplace'
        # this keeps the legend order the same.
        lines_index = ax.lines.index(line)
        errorline_index = ax.lines.index(errorbar_container[0])
        lines = ax.lines

        # swap in .lines, remove the old line reference
        lines[lines_index], lines[errorline_index] = lines[errorline_index], lines[lines_index]
        # delete the reference to the old line
        del lines[errorline_index]

        return errorbar_container

    def hide_error_bar_for(self, data_lines_index):
        """
        Hides the errors for the line at `index`.

        This function does not redraw the plot after it's done any changes.

        :param data_lines_index: Current index of the line in `creation_args`.
                                 Also the position of the line in the list created by FigureErrorsManager.get_data_lines
                                 Used to swap the newly added errorbar line with the old line
        :type data_lines_index: int
        """
        errorbar_container = self._ensure_errorbar_present(data_lines_index)

        self._toggle_error_bar_for(data_lines_index, errorbar_container, make_visible=False)

    def _ensure_errorbar_present(self, data_lines_index):
        """
        Ensures an errorbar is present for the line at the provided index.
        :param data_lines_index:
        :return:
        """
        ax = self.canvas.figure.axes[0]  # type: matplotlib.axes.Axes
        data_lines = self.get_data_lines(ax)
        line = data_lines[data_lines_index]
        errorbar_container = find_errorbar_container(line, ax.containers)
        if errorbar_container is None:
            if not self._supported_ax(ax):
                raise ValueError(self.AXES_NOT_MANTIDAXES_ERR_MESSAGE)
            errorbar_container = self._add_errorbar_for(data_lines_index, line)
        return errorbar_container

    def show_error_bar_for(self, data_lines_index):
        """
        Shows the errors for the line at `data_line_index`.

        This function does not redraw the plot after it's done any changes.

        :param data_lines_index: Current index of the line in `creation_args`.
                                 Also the position of the line in the list created by FigureErrorsManager.get_data_lines
                                 Used to swap the newly added errorbar line with the old line
        :type data_lines_index: int
        """
        errorbar_container = self._ensure_errorbar_present(data_lines_index)
        self._toggle_error_bar_for(data_lines_index, errorbar_container, make_visible=True)

    def toggle_all_error_bars(self, make_visible=None):
        """
        Iterates through all lines in the plot and toggles the error visibility.

        This function does not update the plot after it's done any changes.

        :param make_visible: Force visibility state on all lines.
        :type make_visible: bool
        """
        ax = self.canvas.figure.axes[0]  # type: matplotlib.axes.Axes

        # iterates over all lines, in order to
        # toggle error bar on lines that have errors already
        data_lines = self.get_data_lines(ax)
        for index, container in enumerate(ax.containers):
            data_line = container[0]

            # extract the line reference from the container
            if data_line.get_label() == MantidAxes.MPL_NOLEGEND:
                self._toggle_error_bar_for(data_lines.index(data_line),
                                           errorbar_container=container,
                                           make_visible=make_visible)

        # errors can't be added dynamically while working with axes that aren't supported
        if not self._supported_ax(ax):
            return

        # Iterate over all lines to add new error bars.
        # Each added errorbar changes the lines list, making the iteration not just 1..n.
        # The line is always removed from pos `index`,
        # and errors are added at the end of the list, the next element is moved 1 position up.
        # Therefore the index should only be increased
        # if the line it is looking for already has errors plotted
        for index, data_line in enumerate(data_lines):
            if data_line.get_label() != MantidAxes.MPL_NOLEGEND:
                self._add_errorbar_for(index, data_line, make_visible)

    def _toggle_error_bar_for(self, index, errorbar_container, make_visible=None):
        """
        :param make_visible: Forces this visibility state on all lines.
        :type make_visible: bool
        :param index: The index in the creation_args
        :type index: int
        :return:
        """

        ax = self.canvas.figure.axes[0]  # type: matplotlib.axes.Axes

        # Currently the X errors are not touched at all.
        # Only operations on the Y errors are done.
        y_err_index = self._find_yerr_index(errorbar_container)

        # if the yerr is plotted on the plot
        if y_err_index is not None:
            error_line = errorbar_container.lines[2][y_err_index]
            # the container has the errors there
            # if a force_state is passed, use that, otherwise invert the current state
            new_state = not error_line.get_visible() if make_visible is None else make_visible

            # updates the creation args state if the plot would be saved out
            if self._supported_ax(ax):
                creation_args = ax.creation_args
                creation_args[index][MantidAxKwargs.ERRORS_VISIBLE] = new_state

            error_line.set_visible(new_state)

            # set the proper visibility for caps on the error lines
            error_caps_lines = errorbar_container[1]
            if len(error_caps_lines) > 0:
                for cap in error_caps_lines:
                    cap.set_visible(new_state)

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
