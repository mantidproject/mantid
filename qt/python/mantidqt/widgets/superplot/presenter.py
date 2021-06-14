# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from .view import SuperplotView
from .model import SuperplotModel

from mantid.api import mtd
from mantid.plots.utility import MantidAxType

import matplotlib


class SuperplotPresenter:

    BIN_MODE_TEXT = "Bin"
    SPECTRUM_MODE_TEXT = "Spectrum"
    _view = None
    _model = None
    _canvas = None
    _plot_function = None
    _matplotlib_version = None

    def __init__(self, canvas, parent=None):
        self._view = SuperplotView(self, parent)
        self._model = SuperplotModel()
        self._canvas = canvas
        self.parent = parent
        self._matplotlib_version = matplotlib.__version__

        if self.parent:
            self.parent.plot_updated.connect(self.on_plot_updated)

        self._model.sig_workspace_deleted.connect(self.on_workspace_deleted)
        self._model.sig_workspace_renamed.connect(self.on_workspace_renamed)
        self._model.sig_workspace_replaced.connect(self.on_workspace_replaced)

        #initial state
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

    def get_side_view(self):
        return self._view.get_side_widget()

    def get_bottom_view(self):
        return self._view.get_bottom_widget()

    def close(self):
        if self.parent:
            try:
                self.parent.plot_updated.disconnect()
            except:
                pass
        self._view.close()
        del self._model

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
            self._view.set_available_modes([self.SPECTRUM_MODE_TEXT,
                                            self.BIN_MODE_TEXT])
        else:
            try:
                args = axes.creation_args
            except:
                args = [{}]
            if "axis" in args[0]:
                if (args[0]["axis"] == MantidAxType.BIN
                   or args[0]["axis"] == MantidAxType.BIN.value):
                    for arg in args:
                        if ("axis" not in arg
                            or (arg["axis"] != MantidAxType.BIN
                                and arg["axis"] != MantidAxType.BIN.value)):
                            return
                    self._model.set_bin_mode()
                    self._view.set_available_modes([self.BIN_MODE_TEXT])
                else:
                    for arg in args:
                        if ("axis" in arg
                            and (arg["axis"] == MantidAxType.BIN
                                 or arg["axis"] == MantidAxType.BIN.value)):
                            return
                    self._model.set_spectrum_mode()
                    self._view.set_available_modes([self.SPECTRUM_MODE_TEXT])
            else:
                for arg in args:
                    if ("axis" in arg
                        and (arg["axis"] != MantidAxType.SPECTRUM
                             and arg["axis"] != MantidAxType.SPECTRUM.value)):
                        return
                self._model.set_spectrum_mode()
                self._view.set_available_modes([self.SPECTRUM_MODE_TEXT])
            if "function" in args[0]:
                self._plot_function = args[0]["function"]

        for artist in artists:
            ws, spec_index = \
                    axes.get_artists_workspace_and_workspace_index(artist)
            if spec_index is None:
                i = artists.index(artist)
                if i >= len(args):
                    i = 0
                specIndex = args[i]["wkspIndex"]
            ws_name = ws.name()
            self._model.add_workspace(ws_name)
            self._model.add_data(ws_name, spec_index)

    def on_visibility_changed(self, state):
        """
        Triggered when the visibility of the superplot widget changed. This
        funcion rescale the figure to be sure that the axis and labels are not
        hidden behind the dockwidgets.

        Args:
            state (bool): True if the widget is now visible
        """
        if state:
            try:
                self._canvas.figure.tight_layout()
            except:
                pass
            self._canvas.draw_idle()

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
            self._view.set_available_modes([self.SPECTRUM_MODE_TEXT,
                                            self.BIN_MODE_TEXT])
            self._view.set_mode(mode)
        self._view.set_selection(selection)
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
            self._view.set_spectrum_disabled(True)
            return
        maximum = None
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
                    self._view.set_spectrum_disabled(True)
                    return
            if mode == self.SPECTRUM_MODE_TEXT:
                value = ws.getNumberHistograms()
            else:
                value = ws.blocksize()
            if maximum is None:
                maximum = value
            elif value < maximum:
                maximum = value
        if position is None or position >= maximum:
            position = 0
        self._view.set_spectrum_disabled(False)
        self._view.set_spectrum_slider_max(maximum - 1)
        self._view.set_spectrum_slider_position(position)
        self._view.set_spectrum_spin_box_max(maximum - 1)
        self._view.set_spectrum_spin_box_value(position)

    def _update_hold_button(self):
        """
        Update the hold button state based on the selection.
        """
        selection = self._view.get_selection()
        if not selection:
            self._view.check_hold_button(False)
            return
        index = self._view.get_spectrum_slider_position()
        plotted_data = self._model.get_plotted_data()
        for ws in selection:
            if (ws, index) not in plotted_data:
                self._view.check_hold_button(False)
                return
        self._view.check_hold_button(True)

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

    def _update_plot(self):
        """
        Update the plot. This function overplots the memorized data with the
        currently selected workspace and spectrum index. It keeps a memory of
        the last plot and removes it if is not part of the memorised data.
        """
        selection = self._view.get_selection()
        current_spectrum_index = self._view.get_spectrum_slider_position()
        plotted_data = self._model.get_plotted_data()
        mode = self._view.get_mode()

        figure = self._canvas.figure
        axes = figure.gca()
        artists = axes.get_tracked_artists()

        # remove curves not in plotted_data
        for artist in artists:
            ws, sp = axes.get_artists_workspace_and_workspace_index(artist)
            ws_name = ws.name()
            if (ws_name, sp) not in plotted_data:
                axes.remove_artists_if(lambda a: a==artist)
            else:
                label = artist.get_label()
                try:
                    color = artist.get_color()
                except:
                    color = artist.lines[0].get_color()
                self._view.modify_spectrum_label(ws_name, sp, label, color)

        # add selection to plot
        for ws_name, spectra in selection.items():
            if (current_spectrum_index not in spectra
               and not self._view.is_spectrum_disabled()):
                spectra.append(current_spectrum_index)
            for sp in spectra:
                if sp == -1:
                    continue
                if (ws_name, sp) not in plotted_data:
                    ws = mtd[ws_name]
                    kwargs = {}
                    if mode == self.SPECTRUM_MODE_TEXT:
                        kwargs["axis"] = MantidAxType.SPECTRUM
                        kwargs["specNum"] = ws.getSpectrumNumbers()[sp]
                    else:
                        kwargs["axis"] = MantidAxType.BIN
                        kwargs["wkspIndex"] = sp

                    if self._plot_function == "errorbar":
                        lines = axes.errorbar(ws, **kwargs)
                        label = lines.get_label()
                        color = lines.lines[0].get_color()
                    else:
                        lines = axes.plot(ws, **kwargs)
                        label = lines[0].get_label()
                        color = lines[0].get_color()
                    self._view.modify_spectrum_label(ws_name, sp, label, color)

        if selection or plotted_data:
            axes.set_axis_on()
            figure.tight_layout()
            legend = axes.legend()
            if legend:
                # Legend.draggable() deprecated since v3.0.0
                # https://matplotlib.org/stable/api/prev_api_changes/api_changes_3.0.0.html
                if self._matplotlib_version >= "3.0.0":
                    legend.set_draggable(True)
                else:
                    legend.draggable()
        else:
            legend = axes.get_legend()
            if legend:
                legend.remove()
            axes.set_axis_off()
            axes.set_title("")
        self._canvas.draw_idle()

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
            self._view.set_available_modes([self.SPECTRUM_MODE_TEXT,
                                            self.BIN_MODE_TEXT])
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
        if self._view.is_spectrum_disabled():
            return
        selection = self._view.get_selection()
        spectrum_index = self._view.get_spectrum_slider_position()
        mode = self._view.get_mode()
        for ws_name in selection:
            self._model.add_data(ws_name, spectrum_index)
        if mode == self.SPECTRUM_MODE_TEXT:
            self._model.set_spectrum_mode()
            self._view.set_available_modes([self.SPECTRUM_MODE_TEXT])
        else:
            self._model.set_bin_mode()
            self._view.set_available_modes([self.BIN_MODE_TEXT])
        self._view.check_hold_button(False)
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
            if not self._view.is_spectrum_disabled():
                self._model.remove_data(ws_name, spectrum_index)
            else:
                for spectrum in selection[ws_name]:
                    self._model.remove_data(ws_name, spectrum)
        if not self._model.is_bin_mode() and not self._model.is_spectrum_mode():
            self._view.set_available_modes([self.SPECTRUM_MODE_TEXT,
                                            self.BIN_MODE_TEXT])
            self._view.set_mode(mode)
        self._update_list()
        self._update_spectrum_slider()
        self._update_plot()

    def on_hold_button_toggled(self, state):
        """
        Triggered when the hold button state changed.

        Args:
            state (bool): status of the two state button
        """
        if state:
            self._on_hold()
        else:
            self._on_un_hold()

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
