# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from .view import SuperplotView
from .model import SuperplotModel

from mantid.kernel import ConfigService
from mantid.api import mtd, MatrixWorkspace
from mantid.plots.utility import MantidAxType, legend_set_draggable
from mantid.plots.plotfunctions import MARKER_MAP
from mantid.plots import MantidAxes


class SuperplotPresenter:
    BIN_MODE_TEXT = "Column (bin)"
    SPECTRUM_MODE_TEXT = "Row (spectrum)"
    HOLD_BUTTON_TEXT_CHECKED = "Remove"
    HOLD_BUTTON_TEXT_UNCHECKED = "Add"
    _view = None
    _model = None
    _canvas = None
    _error_bars = False
    _synchronized = False

    def __init__(self, canvas, parent=None):
        self._view = SuperplotView(self, parent)
        self._model = SuperplotModel()
        self._canvas = canvas
        self.parent = parent
        if not isinstance(self._canvas.figure.gca(), MantidAxes):
            return

        # fix size of hold button with the longest text
        self._view.set_hold_button_text(self.HOLD_BUTTON_TEXT_CHECKED)
        width, height = self._view.get_hold_button_size()
        self._view.set_hold_button_size(width, height)

        if self.parent:
            self.parent.plot_updated.connect(self.on_plot_updated)
            self.parent.resized.connect(self.on_resize)

        self._model.sig_workspace_deleted.connect(self.on_workspace_deleted)
        self._model.sig_workspace_renamed.connect(self.on_workspace_renamed)
        self._model.sig_workspace_replaced.connect(self.on_workspace_replaced)

        # initial state
        self._sync_with_current_plot()

        self._update_list()
        self._update_plot()
        plotted_data = self._model.get_plotted_data()
        selection = dict()
        for ws, sp in plotted_data:
            if ws not in selection:
                selection[ws] = [sp]
            else:
                if selection[ws][0] < sp:
                    selection[ws] = [sp]
        self._view.set_selection(selection)
        self._update_spectrum_slider()
        self._update_hold_button()
        self._synchronized = True

    def is_valid(self):
        """
        Check that the superplot started correctly.

        Returns:
            (bool): true if the superplot is in a valid state
        """
        return self._synchronized

    def set_bin_mode(self, state):
        """
        Set the plot mode.

        Args:
            state (bool): if true, bin mode is set, if false, spectrum mode
        """
        if state:
            self._view.set_mode(self.BIN_MODE_TEXT)
        else:
            self._view.set_mode(self.SPECTRUM_MODE_TEXT)
        self._update_spectrum_slider()
        self._update_plot()

    def enable_error_bars(self, state):
        """
        Enable/disable error bars in plot.

        Args:
            state (bool): if True, the error bars will be on
        """
        self._error_bars = state
        self._update_plot()

    def set_workspaces(self, workspaces):
        """
        Instead of synchronization with the current plot, this function can be
        used to set the list of workspaces to be handled by the superplot.

        Args:
            workspaces (list(str)): workspace names
        """
        plotted_workspaces = self._model.get_workspaces()
        for workspace in plotted_workspaces:
            self._model.del_workspace(workspace)
        for workspace in workspaces:
            self._model.add_workspace(workspace)
        self._update_list()
        selection = {name: [-1] for name in workspaces}
        self._view.set_selection(selection)
        self._update_spectrum_slider()
        self._update_hold_button()
        self._update_plot()

    def show(self):
        """
        Show the superplot.
        """
        self._view.show()

    def close(self):
        if self.parent:
            try:
                self.parent.plot_updated.disconnect()
            except:
                pass
        self._view.close()
        del self._model

    def on_resize(self):
        """
        Triggered when the window/dockwidgets is(are) resized.
        """
        self._redraw()

    def _sync_with_current_plot(self):
        """
        This methods synchronize the model with the current plotted data. It
        first checks that the plotted data are consistent (i.e. only bins, only
        spectra), if not, it returns without updating the model.
        """

        figure = self._canvas.figure
        axes = figure.gca()
        artists = axes.get_tracked_artists()
        if not artists:
            self._view.set_available_modes([self.SPECTRUM_MODE_TEXT, self.BIN_MODE_TEXT])
        else:
            try:
                args = axes.creation_args
            except:
                args = [{}]
            if "axis" in args[0]:
                if args[0]["axis"] == MantidAxType.BIN or args[0]["axis"] == MantidAxType.BIN.value:
                    for arg in args:
                        if "axis" not in arg or (arg["axis"] != MantidAxType.BIN and arg["axis"] != MantidAxType.BIN.value):
                            return
                    self._model.set_bin_mode()
                    self._view.set_available_modes([self.BIN_MODE_TEXT])
                else:
                    for arg in args:
                        if "axis" in arg and (arg["axis"] == MantidAxType.BIN or arg["axis"] == MantidAxType.BIN.value):
                            return
                    self._model.set_spectrum_mode()
                    self._view.set_available_modes([self.SPECTRUM_MODE_TEXT])
            else:
                for arg in args:
                    if "axis" in arg and (arg["axis"] != MantidAxType.SPECTRUM and arg["axis"] != MantidAxType.SPECTRUM.value):
                        return
                self._model.set_spectrum_mode()
                self._view.set_available_modes([self.SPECTRUM_MODE_TEXT])
            if "function" in args[0]:
                self._error_bars = args[0]["function"] == "errorbar"

        for artist in artists:
            ws, spec_index = axes.get_artists_workspace_and_workspace_index(artist)
            if spec_index is None:
                i = artists.index(artist)
                if i >= len(args):
                    i = 0
                spec_index = args[i]["wkspIndex"]
            ws_name = ws.name()
            self._model.add_workspace(ws_name)
            self._model.add_data(ws_name, spec_index)

    def get_kwargs_from_settings(self):
        """
        Get the useful plot keyword arguments from the global settings.

        Returns:
            dict(str: str): plot keyword arguments
        """
        kwargs = dict()
        kwargs["linestyle"] = ConfigService.getString("plots.line.Style")
        kwargs["drawstyle"] = ConfigService.getString("plots.line.DrawStyle")
        kwargs["linewidth"] = float(ConfigService.getString("plots.line.Width"))
        kwargs["marker"] = MARKER_MAP[ConfigService.getString("plots.marker.Style")]
        if self._error_bars:
            kwargs["capsize"] = float(ConfigService.getString("plots.errorbar.Capsize"))
            kwargs["capthick"] = float(ConfigService.getString("plots.errorbar.CapThickness"))
            kwargs["errorevery"] = int(ConfigService.getString("plots.errorbar.errorEvery"))
            kwargs["elinewidth"] = float(ConfigService.getString("plots.errorbar.Width"))
        return kwargs

    def on_visibility_changed(self, visible):
        """
        Triggered when the visibility of the superplot widget changed. This
        funcion rescale the figure to be sure that the axis and labels are not
        hidden behind the dockwidgets.

        Args:
            visible (bool): True if the widget is now visible
        """
        if visible:
            self._redraw()

    def on_normalise_checked(self, checked: bool):
        """
        Transmit the normalisation checkbox state to the model.

        Args:
            checked (bool): True when the checkbox is checked
        """
        self._model.normalise(checked)
        self._update_plot(True)
        figure = self._canvas.figure
        axes = figure.gca()
        axes.relim()
        axes.autoscale()
        self._update_plot()

    def on_drop(self, name):
        """
        Triggered when a drop event is received in the list widget. Here, name
        is assumed to be a workspace name.

        Args:
            name (str): workspace name
        """
        selection = self._view.get_selection()
        self._model.add_workspace(name)
        self._update_list()
        self._view.set_selection(selection)
        self._update_plot()

    def on_add_button_clicked(self):
        """
        Triggered when the add button is pressed. This function adds the
        workspace to the selection list.
        """
        selection = self._view.get_selection()
        added_workspace = self._view.get_selected_workspace()
        self._model.add_workspace(added_workspace)
        self._update_list()
        self._view.set_selection(selection)
        self._update_plot()

    def on_del_button_clicked(self, ws_name=None):
        """
        Triggered when the del button is pressed. This function removes the
        selected workspace from the selection list.
        """
        selection = self._view.get_selection()
        if ws_name is None:
            selected_workspaces = selection.copy()
        else:
            selected_workspaces = [ws_name]
        for selected_workspace in selected_workspaces:
            self._model.del_workspace(selected_workspace)
        self._update_list()
        if not self._model.is_bin_mode() and not self._model.is_spectrum_mode():
            mode = self._view.get_mode()
            self._view.set_available_modes([self.SPECTRUM_MODE_TEXT, self.BIN_MODE_TEXT])
            self._view.set_mode(mode)
        self._view.set_selection(selection)
        if all(name in selected_workspaces for name in selection.keys()):
            self._update_spectrum_slider()
        self._update_plot()

    def _update_spectrum_slider(self):
        """
        Update the spectrum slider and spinbox to match the selected workspaces.
        """
        selection = self._view.get_selection()
        if not selection:
            self._view.set_spectrum_slider_position(0)
            self._view.set_spectrum_slider_max(0)
            self._view.set_spectrum_spin_box_value(0)
            self._view.set_spectrum_spin_box_max(0)
            self._view.set_spectrum_selection_disabled(True)
            self._view.set_hold_button_text("")
            return
        maximum_dimension_size = None
        position = None
        mode = self._view.get_mode()
        for ws_name in selection:
            ws = mtd[ws_name]
            for sp in selection[ws_name]:
                if position is None:
                    position = sp
                elif position != sp:
                    self._view.set_spectrum_slider_position(0)
                    self._view.set_spectrum_slider_max(0)
                    self._view.set_spectrum_spin_box_value(0)
                    self._view.set_spectrum_spin_box_max(0)
                    self._view.set_spectrum_selection_disabled(True)
                    return
            if mode == self.SPECTRUM_MODE_TEXT:
                dimension_size = ws.getNumberHistograms()
            else:
                dimension_size = ws.blocksize()
            if maximum_dimension_size is None:
                maximum_dimension_size = dimension_size
            elif dimension_size < maximum_dimension_size:
                maximum_dimension_size = dimension_size
        if position is None or position >= maximum_dimension_size:
            position = 0
        self._view.set_spectrum_selection_disabled(False)
        self._view.set_spectrum_slider_max(maximum_dimension_size - 1)
        self._view.set_spectrum_slider_position(position)
        self._view.set_spectrum_spin_box_max(maximum_dimension_size - 1)
        self._view.set_spectrum_spin_box_value(position)

    def _update_hold_button(self):
        """
        Update the hold button state based on the selection.
        """
        selection = self._view.get_selection()
        if not selection:
            self._view.set_hold_button_text(self.HOLD_BUTTON_TEXT_UNCHECKED)
            return
        index = self._view.get_spectrum_slider_position()
        plotted_data = self._model.get_plotted_data()
        for ws in selection:
            if (ws, index) not in plotted_data:
                self._view.set_hold_button_text(self.HOLD_BUTTON_TEXT_UNCHECKED)
                return
        self._view.set_hold_button_text(self.HOLD_BUTTON_TEXT_CHECKED)

    def _update_list(self):
        """
        Update the workspaces/spectra list.
        """
        names = self._model.get_workspaces()
        plotted_data = self._model.get_plotted_data()
        self._view.set_workspaces_list(names)
        for name in names:
            spectra = list()
            for data in plotted_data:
                if data[0] == name:
                    spectra.append(data[1])
            self._view.set_spectra_list(name, spectra)

    def _update_plot(self, replot: bool = False):
        """
        Update the plot. This function overplots the memorized data with the
        currently selected workspace and spectrum index. It keeps a memory of
        the last plot and removes it if is not part of the memorised data. It
        can also replot all curves if needed.

        Args:
            replot (bool): if True, all curves are removed and replotted
        """
        selection = self._view.get_selection()
        plotted_data = self._model.get_plotted_data()

        figure = self._canvas.figure
        figure.set_visible(True)

        self._remove_unneeded_curves(replot)
        self._plot_selection()

        self._redraw(not selection and not plotted_data)

    def _redraw(self, empty=False):
        """
        Redraw the matplotlib canvas using a tight layout. The function takes
        care of the legend and avoid that it interfere with the layout.

        Args:
            empty (bool): True if the plot is empty. In this case, the figure is
                          not drawn
        """
        figure = self._canvas.figure
        if empty:
            figure.set_visible(False)
        else:
            axes = figure.gca()
            hasLegend = False
            legend = axes.get_legend()
            if legend:
                hasLegend = legend.get_visible()
                legend.remove()
            try:
                figure.tight_layout()
            except ValueError:
                pass
            legend = axes.legend()
            legend.set_visible(hasLegend)
            legend_set_draggable(legend, True)
        try:
            self._canvas.draw()
        except:
            # exception are thown by the mantid plot layer
            pass

    def _remove_unneeded_curves(self, replot: bool):
        """
        Removes all unneeded curves from the actual plot.

        Args:
            replot (bool): if True, all remaining curves are reploted
        """
        plotted_data = self._model.get_plotted_data()
        mode = self._view.get_mode()
        normalised = self._model.is_normalised()

        figure = self._canvas.figure
        axes = figure.gca()
        artists = axes.get_tracked_artists()

        for artist in artists:
            try:
                ws, sp = axes.get_artists_workspace_and_workspace_index(artist)
            except KeyError:
                # avoid race condition when many workspaces are remove from the
                # ADS at the same type
                continue
            ws_name = ws.name()
            if (ws_name, sp) not in plotted_data:
                axes.remove_artists_if(lambda a: a == artist)
            else:
                label = artist.get_label()
                try:
                    color = artist.get_color()
                except:
                    color = artist.lines[0].get_color()
                if color == self._model.get_workspace_color(ws_name):
                    self._model.set_workspace_color(ws_name, None)
                self._view.modify_spectrum_label(ws_name, sp, label, color)
                if replot:
                    axes.remove_artists_if(lambda a: a == artist)
                    kwargs = self._fill_plot_kwargs(ws_name, sp, normalised, mode, color)
                    ws = mtd[ws_name]
                    if self._error_bars:
                        axes.errorbar(ws, **kwargs)
                    else:
                        axes.plot(ws, **kwargs)

    def _plot_selection(self):
        """
        Adds selected workspaces/spectra to the plot.
        """
        selection = self._view.get_selection()
        current_spectrum_index = self._view.get_spectrum_slider_position()
        plotted_data = self._model.get_plotted_data()
        mode = self._view.get_mode()
        normalised = self._model.is_normalised()

        figure = self._canvas.figure
        axes = figure.gca()

        # add selection to plot
        for ws_name, spectra in selection.items():
            if current_spectrum_index not in spectra and not self._view.is_spectrum_selection_disabled():
                spectra.append(current_spectrum_index)
            for sp in spectra:
                if sp == -1:
                    continue
                if (ws_name, sp) not in plotted_data:
                    color = self._model.get_workspace_color(ws_name)
                    kwargs = self._fill_plot_kwargs(ws_name, sp, normalised, mode, color)
                    if ws_name not in mtd:
                        continue
                    ws = mtd[ws_name]
                    error_ws = False
                    if isinstance(ws, MatrixWorkspace):
                        plot_type = ws.getPlotType()
                        if "errorbar" in plot_type:
                            error_ws = True
                    if self._error_bars or error_ws:
                        lines = axes.errorbar(ws, **kwargs)
                        color = lines.lines[0].get_color()
                    else:
                        lines = axes.plot(ws, **kwargs)
                        color = lines[0].get_color()
                    self._model.set_workspace_color(ws_name, color)

    def _fill_plot_kwargs(self, ws_name: str, spectrum: int, normalise: bool, mode: str, color: str) -> dict:
        """
        Fill the keywork arguments dictionnary needed by the mantid plot
        function.

        Args:
            ws_name (str): name of the workspace
            spectrum (int): index of the spectrum
            normalise (bool): True if the plot has to be normalised
            mode (self.SPECTRUM_MODE_TEXT or self.BIN_MODE_TEXT): spectrum or
                bin mode plot
            color (str): color of the curve

        Returns:
            dict(str: str): plot keywork arguments
        """
        kwargs = dict()
        kwargs["wkspIndex"] = spectrum
        if normalise is True:
            kwargs["normalise_spectrum"] = True
        if mode == self.SPECTRUM_MODE_TEXT:
            kwargs["axis"] = MantidAxType.SPECTRUM
        elif mode == self.BIN_MODE_TEXT:
            kwargs["axis"] = MantidAxType.BIN
        kwargs["color"] = color
        return kwargs

    def on_workspace_selection_changed(self):
        """
        Triggered when the selected workspace (in the workspace tree) changed.
        """
        self._update_spectrum_slider()
        self._update_hold_button()
        self._update_plot()

    def on_spectrum_slider_moved(self, position):
        """
        Triggered when the spectrum slider moved.

        Args:
            position (int): slider position
        """
        self._view.set_spectrum_spin_box_value(position)
        self._update_hold_button()
        self._update_plot()

    def on_spectrum_spin_box_changed(self, value):
        """
        Triggered when the spectrum spinbox changed.

        Args:
            value (int): spinbox value
        """
        self._view.set_spectrum_slider_position(value)
        self._update_hold_button()
        self._update_plot()

    def on_del_spectrum_button_clicked(self, ws_name, index):
        """
        Triggered when the delete button of a selected spectrum has been
        pressed.

        Args:
            ws_name (str): name of the corresponding workspace
            index (int): index of the corresponding spectrum
        """
        selection = self._view.get_selection()
        mode = self._view.get_mode()
        self._model.remove_data(ws_name, index)
        if not self._model.is_bin_mode() and not self._model.is_spectrum_mode():
            self._view.set_available_modes([self.SPECTRUM_MODE_TEXT, self.BIN_MODE_TEXT])
            self._view.set_mode(mode)
        if ws_name in selection:
            if index in selection[ws_name]:
                selection[ws_name].remove(index)
                if not selection[ws_name]:
                    del selection[ws_name]
        self._update_list()
        self._view.set_selection(selection)
        self._update_spectrum_slider()
        self._update_plot()

    def _on_hold(self):
        """
        Add the selected ws, sp pair to the plot.
        """
        if self._view.is_spectrum_selection_disabled():
            return
        selection = self._view.get_selection()
        spectrum_index = self._view.get_spectrum_slider_position()
        mode = self._view.get_mode()
        for ws_name in selection:
            self._model.add_data(ws_name, spectrum_index)
            selection[ws_name] = [spectrum_index]
        if mode == self.SPECTRUM_MODE_TEXT:
            self._model.set_spectrum_mode()
            self._view.set_available_modes([self.SPECTRUM_MODE_TEXT])
        else:
            self._model.set_bin_mode()
            self._view.set_available_modes([self.BIN_MODE_TEXT])
        self._view.set_hold_button_text(self.HOLD_BUTTON_TEXT_CHECKED)
        self._update_list()
        self._view.set_selection(selection)
        self._update_plot()

    def _on_un_hold(self):
        """
        Remove the selected ws, sp pair from the plot.
        """
        selection = self._view.get_selection()
        spectrum_index = self._view.get_spectrum_slider_position()
        mode = self._view.get_mode()
        for ws_name in selection:
            if not self._view.is_spectrum_selection_disabled():
                self._model.remove_data(ws_name, spectrum_index)
            else:
                for spectrum in selection[ws_name]:
                    self._model.remove_data(ws_name, spectrum)
            selection[ws_name] = [-1]
        if not self._model.is_bin_mode() and not self._model.is_spectrum_mode():
            self._view.set_available_modes([self.SPECTRUM_MODE_TEXT, self.BIN_MODE_TEXT])
            self._view.set_mode(mode)
        self._update_list()
        self._update_hold_button()
        self._view.set_selection(selection)
        self._update_plot()

    def on_hold_button_clicked(self):
        """
        Triggered when the hold button state changed.

        Args:
            state (bool): status of the two state button
        """
        if self._view.get_hold_button_text() == self.HOLD_BUTTON_TEXT_CHECKED:
            self._on_un_hold()
        else:
            self._on_hold()

    def on_mode_changed(self, mode):
        """
        Triggered when the selected mode changed in the view.

        Args:
            mode (str): new mode
        """
        self._update_spectrum_slider()
        self._update_plot()
        figure = self._canvas.figure
        axes = figure.gca()
        axes.relim()
        axes.autoscale()

    def on_workspace_deleted(self, ws_name):
        """
        Triggered when the model reports a workspace deletion.

        Args:
            ws_name (str): name of the workspace
        """
        selection = self._view.get_selection()
        if ws_name in selection:
            del selection[ws_name]
        self._update_list()
        self._view.set_selection(selection)
        self._update_plot()

    def on_workspace_renamed(self, old_name, new_name):
        """
        Triggered when the model reports a workspace renaming.

        Args:
            old_name (str): old name of the workspace
            new_name (str): new name of the workspace
        """
        selection = self._view.get_selection()
        if old_name in selection:
            selection[new_name] = selection[old_name]
            del selection[old_name]
        self._update_list()
        self._view.set_selection(selection)
        self._update_plot()

    def on_workspace_replaced(self, ws_name):
        """
        Triggered when the model reports a workapce replacement.

        Args:
            ws_name (str): name of the workspace
        """
        self._update_plot()

    def on_plot_updated(self):
        """
        Triggered when the plot window is updated (drag and drop only for now).
        This methods redo an init procedure to synchronize the list with the
        plot.
        """
        selection = self._view.get_selection()
        current_index = self._view.get_spectrum_slider_position()
        self._sync_with_current_plot()
        self._update_list()
        self._view.set_spectrum_slider_position(current_index)
        self._view.set_spectrum_spin_box_value(current_index)
        self._view.set_selection(selection)
        self._update_plot()
