# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
from collections.abc import Iterable
import copy
from functools import wraps

import numpy as np
import re

from cycler import cycler
from matplotlib.axes import Axes
from matplotlib.cbook import safe_masked_invalid
from matplotlib.collections import Collection, PolyCollection
from matplotlib.colors import Colormap
from matplotlib.container import Container, ErrorbarContainer
from matplotlib.image import AxesImage
from matplotlib.lines import Line2D
from matplotlib.patches import Patch
from matplotlib.table import Table
from matplotlib.ticker import NullLocator
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

from mantid import logger
from mantid.api import AnalysisDataService as ads
from mantid.plots import datafunctions, axesfunctions, axesfunctions3D
from mantid.plots.legend import LegendProperties
from mantid.plots.datafunctions import get_normalize_by_bin_width
from mantid.plots.utility import artists_hidden, autoscale_on_update, legend_set_draggable, MantidAxType, get_plot_specific_properties


WATERFALL_XOFFSET_DEFAULT, WATERFALL_YOFFSET_DEFAULT = 10, 20

# -----------------------------------------------------------------------------
# Decorators
# -----------------------------------------------------------------------------


def plot_decorator(func):
    @wraps(func)
    def wrapper(self, *args, **kwargs):
        func_value = func(self, *args, **kwargs)
        func_name = func.__name__
        # Saves saving it on array objects
        if datafunctions.validate_args(*args, **kwargs):
            # Fill out kwargs with the values of args
            kwargs["workspaces"] = args[0].name()
            kwargs["function"] = func_name

            if "wkspIndex" not in kwargs and "specNum" not in kwargs:
                kwargs["specNum"] = MantidAxes.get_spec_number_or_bin(args[0], kwargs)
            if "cmap" in kwargs and isinstance(kwargs["cmap"], Colormap):
                kwargs["cmap"] = kwargs["cmap"].name
            self.creation_args.append(kwargs)
        elif func_name == "axhline" or func_name == "axvline":
            self.creation_args.append({"function": func_name, "args": args, "kwargs": kwargs})

        return func_value

    return wrapper


# -----------------------------------------------------------------------------
# MantidAxes
# -----------------------------------------------------------------------------


class MantidAxes(Axes):
    """
    This class defines the **mantid** projection for 2d plotting. One chooses
    this projection using::

        import matplotlib.pyplot as plt
        from mantid import plots
        fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})

    or::

        import matplotlib.pyplot as plt
        from mantid import plots
        fig = plt.figure()
        ax = fig.add_subplot(111,projection='mantid')

    The mantid projection allows replacing the array objects with mantid workspaces.
    """

    # Required by Axes base class
    name = "mantid"

    # Store information for any workspaces attached to this axes instance
    tracked_workspaces = None

    def __init__(self, *args, **kwargs):
        super(MantidAxes, self).__init__(*args, **kwargs)
        self.tracked_workspaces = dict()
        self.creation_args = []
        self.interactive_markers = []
        # flag to indicate if a function has been set to replace data
        self.data_replaced = False

        self.waterfall_x_offset = 0
        self.waterfall_y_offset = 0

    def add_artist_correctly(self, artist):
        """
        Add an artist via the correct function.
        MantidAxes will not correctly track artists added via :code:`add_artist`.
        They must be added via the correct function for features like
        autoscaling to work.

        :param artist: A Matplotlib Artist object
        """
        artist.set_transform(self.transData)
        if artist.axes:
            artist.remove()
        if isinstance(artist, Line2D):
            self.add_line(artist)
        elif isinstance(artist, Collection):
            self.add_collection(artist)
        elif isinstance(artist, Container):
            self.add_container(artist)
        elif isinstance(artist, AxesImage):
            self.add_image(artist)
        elif isinstance(artist, Patch):
            self.add_patch(artist)
        elif isinstance(artist, Table):
            self.add_table(artist)
        else:
            self.add_artist(artist)

    @staticmethod
    def from_mpl_axes(ax, ignore_artists=None):
        """
        Returns a MantidAxes from an Axes object.
        Transfers all transferable artists from a Matplotlib.Axes
        instance to a MantidAxes instance on the same figure. Then
        removes the Matplotlib.Axes.

        :param ax: An Axes object
        :param ignore_artists: List of Artist types to ignore
        :returns: A MantidAxes object
        """
        if not ignore_artists:
            ignore_artists = []

        default_cycle_colours = plt.rcParams["axes.prop_cycle"].by_key()["color"]
        number_of_mpl_lines = len(ax.get_lines())
        artists = ax.get_children()
        mantid_axes = ax.figure.add_subplot(111, projection="mantid", label="mantid")
        for artist in artists:
            if not any(isinstance(artist, artist_type) for artist_type in ignore_artists):
                try:
                    mantid_axes.add_artist_correctly(artist)
                except NotImplementedError:
                    pass
        mantid_axes.set_title(ax.get_title())

        if number_of_mpl_lines > 0:
            used_colors = default_cycle_colours[: (number_of_mpl_lines % len(default_cycle_colours)) + 1]
            remaining_colors = default_cycle_colours[(number_of_mpl_lines % len(default_cycle_colours)) + 1 :]
            default_cycle_colours = remaining_colors + used_colors

        mantid_axes.set_prop_cycle(cycler(color=default_cycle_colours))

        ax.remove()
        return mantid_axes

    def set_title(self, label, fontdict=None, *args, **kwargs):
        if not fontdict:
            font_props = self.title.get_fontproperties()
            fontdict = {"fontsize": font_props.get_size(), "fontweight": font_props.get_weight(), "color": self.title.get_color()}

        super().set_title(label, fontdict=fontdict, *args, **kwargs)

    @staticmethod
    def is_axis_of_type(axis_type, kwargs):
        if kwargs.get("axis", None) is not None:
            return kwargs.get("axis", None) == axis_type
        return axis_type == MantidAxType.SPECTRUM

    @staticmethod
    def get_spec_num_from_wksp_index(workspace, wksp_index):
        return workspace.getSpectrum(wksp_index).getSpectrumNo()

    @staticmethod
    def get_spec_number_or_bin(workspace, kwargs):
        if kwargs.get("specNum", None) is not None:
            return kwargs["specNum"]
        elif kwargs.get("wkspIndex", None) is not None:
            # If wanting to plot a bin
            if MantidAxes.is_axis_of_type(MantidAxType.BIN, kwargs):
                return kwargs["wkspIndex"]
            # If wanting to plot a spectrum
            elif MantidAxes.is_axis_of_type(MantidAxType.SPECTRUM, kwargs):
                return MantidAxes.get_spec_num_from_wksp_index(workspace, kwargs["wkspIndex"])
        elif kwargs.get("LogName", None) is not None:
            return None
        elif getattr(workspace, "getNumberHistograms", lambda: -1)() == 1:
            # If the workspace has one histogram, just plot that
            if MantidAxes.is_axis_of_type(MantidAxType.BIN, kwargs):
                kwargs["specNum"] = 0
            else:
                kwargs["specNum"] = MantidAxes.get_spec_num_from_wksp_index(workspace, 0)
            return kwargs["specNum"]
        else:
            return None

    def get_workspace_name_from_artist(self, artist):
        for ws_name, ws_artists_list in self.tracked_workspaces.items():
            for ws_artists in ws_artists_list:
                for ws_artist in ws_artists._artists:
                    if artist == ws_artist:
                        return ws_name

    def get_artists_workspace_and_spec_num(self, artist):
        """Retrieve the workspace and spec num of the given artist"""
        for ws_name, ws_artists_list in self.tracked_workspaces.items():
            for ws_artists in ws_artists_list:
                for ws_artist in ws_artists._artists:
                    if artist == ws_artist:
                        return ads.retrieve(ws_name), ws_artists.spec_num
        raise ValueError("Artist: '{}' not tracked by axes.".format(artist))

    def get_artists_workspace_and_workspace_index(self, artist):
        """Retrieve the workspace and spec num of the given artist"""
        for ws_name, ws_artists_list in self.tracked_workspaces.items():
            for ws_artists in ws_artists_list:
                for ws_artist in ws_artists._artists:
                    if artist == ws_artist:
                        return ads.retrieve(ws_name), ws_artists.workspace_index
        raise ValueError("Artist: '{}' not tracked by axes.".format(artist))

    def get_artists_sample_log_plot_details(self, artist):
        """Retrieve the sample log plot details of the given artist"""
        for ws_name, ws_artists_list in self.tracked_workspaces.items():
            for ws_artists in ws_artists_list:
                for ws_artist in ws_artists._artists:
                    if artist == ws_artist:
                        if ws_artists.log_name is not None:
                            return (ws_artists.log_name, ws_artists.filtered, ws_artists.expt_info_index)
                        else:
                            return None
        raise ValueError("Artist: '{}' not tracked by axes.".format(artist))

    def get_artist_normalization_state(self, artist):
        for ws_name, ws_artists_list in self.tracked_workspaces.items():
            for ws_artists in ws_artists_list:
                for ws_artist in ws_artists._artists:
                    if artist == ws_artist:
                        return ws_artists.is_normalized

    def get_is_mdhisto_workspace_for_artist(self, artist) -> bool:
        workspace, _ = self.get_artists_workspace_and_workspace_index(artist)
        return (workspace is not None) and workspace.isMDHistoWorkspace()

    def track_workspace_artist(
        self,
        workspace,
        artists,
        data_replace_cb=None,
        spec_num=None,
        is_normalized=None,
        is_spec=True,
        log_name=None,
        filtered=True,
        expt_info_index=None,
    ):
        """
        Add the given workspace's name to the list of workspaces
        displayed on this Axes instance
        :param workspace: A Workspace object. If empty then no tracking takes place
        :param artists: A single artist or iterable of artists containing the data for the workspace
        :param data_replace_cb: A function to call when the data is replaced to update
        the artist (optional)
        :param spec_num: The spectrum number associated with the artist (optional)
        :param is_normalized: bool. The line being plotted is normalized by bin width
            This can be from either a distribution workspace or a workspace being
            plotted as a distribution
        :param is_spec: bool. True if spec_num represents a spectrum, and False if it is a bin index
        :param log_name: string. The name of the plotted log
        :param filtered: bool. True if log plotted was filtered, and False if unfiltered.
            This only has meaning if log_name is not None.
        :param expt_info_index: Integer. The index of the experiment info for this plotted log.
            This only has meaning if log_name is not None.
        :returns: The artists variable as it was passed in.
        """
        name = workspace.name()
        if name:
            if data_replace_cb is None:

                def data_replace_cb(*args):
                    logger.warning("Updating data on this plot type is not yet supported")

            else:
                self.data_replaced = True

            artist_info = self.tracked_workspaces.setdefault(name, [])

            artist_info.append(
                _WorkspaceArtists(artists, data_replace_cb, is_normalized, name, spec_num, is_spec, log_name, filtered, expt_info_index)
            )
            self.check_axes_distribution_consistency()
        return artists

    def check_axes_distribution_consistency(self):
        """
        Checks if the curves on the axes are all normalized or all
        non-normalized and displays a warning if not.
        """
        tracked_ws_distributions = []
        for artists in self.tracked_workspaces.values():
            for artist in artists:
                if artist.is_normalized is not None:
                    tracked_ws_distributions.append(artist.is_normalized)

        if len(tracked_ws_distributions) > 0:
            num_normalized = sum(tracked_ws_distributions)
            if not (num_normalized == 0 or num_normalized == len(tracked_ws_distributions)):
                logger.warning("You are overlaying distribution and non-distribution data!")

    def artists_workspace_has_errors(self, artist):
        """Check if the given artist's workspace has errors"""
        if artist not in self.get_tracked_artists():
            raise ValueError("Artist '{}' is not tracked and so does not have " "an associated workspace.".format(artist))
        workspace, spec_num = self.get_artists_workspace_and_spec_num(artist)
        if artist.axes.creation_args[0].get("axis", None) == MantidAxType.BIN:
            if any([workspace.readE(i)[spec_num] != 0 for i in range(0, workspace.getNumberHistograms())]):
                return True
        elif spec_num is not None:
            workspace_index = workspace.getIndexFromSpectrumNumber(spec_num)
            if any(workspace.readE(workspace_index) != 0):
                return True
        return False

    def get_tracked_artists(self):
        """Get the Matplotlib artist objects that are tracked"""
        tracked_artists = []
        for ws_artists_list in self.tracked_workspaces.values():
            for ws_artists in ws_artists_list:
                for artist in ws_artists._artists:
                    tracked_artists.append(artist)
        return tracked_artists

    def remove_workspace_artists(self, workspace):
        if self.is_waterfall():
            return self._remove_workspace_artists_waterfall(workspace=workspace)
        else:
            return self._remove_workspace_artists(workspace)

    def remove_artists_if(self, unary_predicate):
        if self.is_waterfall():
            return self._remove_workspace_artists_waterfall(predicate=unary_predicate)
        else:
            return self._remove_artists_if(unary_predicate)

    def _remove_workspace_artists_waterfall(self, workspace=None, predicate=None):
        """
        Perform the steps necessary to maintain waterfall plot settings before removing
        the artists. Output is based on the inner function.
        If workspace is set, uses _remove_workspace_artists()
        otherwise if predicate is set, uses _remove_artists_if()
        otherwise raises a RuntimeError.
        :param workspace: A Workspace object
        :param predicate: A unary predicate used to select artists.
        :return: The output of the inner function.
        """
        if workspace is not None and workspace.name() not in self.tracked_workspaces:
            return False
        waterfall_x_offset = copy.copy(self.waterfall_x_offset)
        waterfall_y_offset = copy.copy(self.waterfall_y_offset)
        has_fills = self.waterfall_has_fill()

        self.update_waterfall(0, 0)

        if workspace is not None:
            output = self._remove_workspace_artists(workspace)
        elif predicate is not None:
            output = self._remove_artists_if(predicate)
        else:
            raise RuntimeError("A workspace or predicate is required.")

        self.update_waterfall(waterfall_x_offset, waterfall_y_offset)

        if len(self.lines) == 1:  # Can't have waterfall plots with only one line.
            self.set_waterfall(False)
        elif has_fills:
            datafunctions.waterfall_update_fill(self)

        return output

    def _remove_workspace_artists(self, workspace):
        """
        Remove the artists reference by this workspace (if any) and return True
        if the axes is then empty
        :param workspace: A Workspace object
        :return: True is an artist was removed False if one was not
        """
        try:
            # pop to ensure we don't hold onto an artist reference
            artist_info = self.tracked_workspaces.pop(workspace.name())
        except KeyError:
            return False

        for workspace_artist in artist_info:
            workspace_artist.remove(self)

        return True

    def _remove_artists_if(self, unary_predicate):
        """
        Remove any artists which satisfy the predicate and return True
        if the axes is then empty
        :param unary_predicate: A predicate taking a single matplotlib artist object
        :return: True if the axes is empty, false if artists remain
        """
        is_empty_list = []
        for workspace_name, artist_info in self.tracked_workspaces.items():
            is_empty = self._remove_artist_info_if(artist_info, unary_predicate)
            if is_empty:
                is_empty_list.append(workspace_name)

        for workspace_name in is_empty_list:
            self.tracked_workspaces.pop(workspace_name)

        # Catch any artists that are not tracked
        for artist in self.artists + self.lines + self.containers + self.images:
            if unary_predicate(artist):
                artist.remove()
                if isinstance(artist, ErrorbarContainer):
                    self.containers.remove(artist)

        return self.is_empty(self)

    def _remove_artist_info_if(self, artist_info, unary_predicate):
        """
        Remove any artists which satisfy the predicate from the artist_info_list
        :param artist_info: A list of _WorkspaceArtists objects
        :param unary_predicate: A predicate taking a single matplotlib artist object
        :return: True if the artist_info is empty, false if artist_info remain
        """
        is_empty_list = [workspace_artist.remove_if(self, unary_predicate) for workspace_artist in artist_info]

        for index, empty in reversed(list(enumerate(is_empty_list))):
            if empty:
                artist_info.pop(index)

        return len(artist_info) == 0

    def replace_workspace_artists(self, workspace):
        """
        Replace the data of any artists relating to this workspace.
        The axes are NOT redrawn
        :param workspace: The workspace containing the new data
        :return : True if data was replaced, false otherwise
        """
        try:
            artist_info = self.tracked_workspaces[workspace.name()]
        except KeyError:
            return False

        is_empty_list = [workspace_artist.replace_data(workspace) for workspace_artist in artist_info]

        for index, empty in reversed(list(enumerate(is_empty_list))):
            if empty:
                artist_info.pop(index)

        return True

    def rename_workspace(self, new_name, old_name):
        """
        Rename a workspace, and update the artists, creation arguments and tracked workspaces accordingly
        :param new_name : the new name of workspace
        :param old_name : the old name of workspace
        """
        for cargs in self.creation_args:
            if cargs["workspaces"] == old_name:
                cargs["workspaces"] = new_name
        for ws_name, ws_artist_list in list(self.tracked_workspaces.items()):
            for ws_artist in ws_artist_list:
                if ws_artist.workspace_name == old_name:
                    ws_artist.rename_data(new_name)
            if ws_name == old_name:
                self.tracked_workspaces[new_name] = self.tracked_workspaces.pop(old_name)

    def replot_artist(self, artist, errorbars=False, **kwargs):
        """
        Replot an artist with a new set of kwargs via 'plot' or 'errorbar'
        :param artist: The artist to replace
        :param errorbars: Plot with or without errorbars
        :returns: The new artist that has been plotted
        For keywords related to workspaces, see :func:`plotfunctions.plot` or
        :func:`plotfunctions.errorbar`
        """
        kwargs["distribution"] = not self.get_artist_normalization_state(artist)
        workspace, spec_num = self.get_artists_workspace_and_spec_num(artist)

        # deal with MDHisto workspace
        if workspace.isMDHistoWorkspace():
            # the MDHisto does not have the distribution concept.
            # This is available only for Workspace2D
            if "distribution" in kwargs.keys():
                del kwargs["distribution"]
        # check if it is a sample log plot
        elif spec_num is None:
            sample_log_plot_details = self.get_artists_sample_log_plot_details(artist)
            # we plot MDHisto workspaces, Workspace2D spectra, and Sample Logs
            # if you get here, the LogName is valid and not None
            kwargs["LogName"] = sample_log_plot_details[0]
            if sample_log_plot_details[1] is not None:
                kwargs["Filtered"] = sample_log_plot_details[1]
            if sample_log_plot_details[2] is not None:
                kwargs["ExperimentInfo"] = sample_log_plot_details[2]
            # error bar plots do not make sense for log plots
            errorbars = False
            # neither does distribution
            if "distribution" in kwargs.keys():
                del kwargs["distribution"]
        else:
            if kwargs.get("axis", None) == MantidAxType.BIN:
                workspace_index = spec_num
            else:
                workspace_index = workspace.getIndexFromSpectrumNumber(spec_num)

            self._remove_matching_curve_from_creation_args(workspace.name(), workspace_index, spec_num)
            kwargs["wkspIndex"] = workspace_index

        self.remove_artists_if(lambda art: art == artist)
        if errorbars:
            new_artist = self.errorbar(workspace, **kwargs)
        else:
            new_artist = self.plot(workspace, **kwargs)[0]
        return new_artist

    def relim(self, visible_only=True):
        # Hiding the markers during the relim ensures they are not factored
        # in (assuming that visible_only is True)
        with artists_hidden(self.interactive_markers):
            Axes.relim(self, visible_only)  # relim on any non-errorbar objects
            lower_xlim, lower_ylim = self.dataLim.get_points()[0]
            upper_xlim, upper_ylim = self.dataLim.get_points()[1]
            for container in self.containers:
                if isinstance(container, ErrorbarContainer) and (
                    (visible_only and not datafunctions.errorbars_hidden(container)) or not visible_only
                ):
                    min_x, max_x, min_y, max_y = datafunctions.get_errorbar_bounds(container)
                    lower_xlim = min(lower_xlim, min_x) if min_x else lower_xlim
                    upper_xlim = max(upper_xlim, max_x) if max_x else upper_xlim
                    lower_ylim = min(lower_ylim, min_y) if min_y else lower_ylim
                    upper_ylim = max(upper_ylim, max_y) if max_y else upper_ylim

            xys = [[lower_xlim, lower_ylim], [upper_xlim, upper_ylim]]
            # update_datalim will update limits with union of current lims and xys
            self.update_datalim(xys)

    def make_legend(self):
        props = LegendProperties.from_legend(self.legend_)
        LegendProperties.create_legend(props, self)

    @staticmethod
    def is_empty(axes):
        """
        Checks the known artist containers to see if anything exists within them
        :return: True if no artists exist, false otherwise
        """

        def _empty(container):
            return len(container) == 0

        return _empty(axes.lines) and _empty(axes.images) and _empty(axes.collections) and _empty(axes.containers)

    def twinx(self):
        """
        Create a twin Axes sharing the xaxis

        Create a new Axes instance with an invisible x-axis and an independent
        y-axis positioned opposite to the original one (i.e. at right). The
        x-axis autoscale setting will be inherited from the original Axes.
        To ensure that the tick marks of both y-axes align, see
        `~matplotlib.ticker.LinearLocator`

        Returns
        -------
        ax_twin : Axes
            The newly created Axes instance

        Notes
        -----
        For those who are 'picking' artists while using twinx, pick
        events are only called for the artists in the top-most axes.
        """
        ax2 = self._make_twin_axes(sharex=self, projection="mantid")
        ax2.yaxis.tick_right()
        ax2.yaxis.set_label_position("right")
        ax2.yaxis.set_offset_position("right")
        ax2.set_autoscalex_on(self.get_autoscalex_on())
        self.yaxis.tick_left()
        ax2.xaxis.set_visible(False)
        ax2.patch.set_visible(False)
        return ax2

    def twiny(self):
        """
        Create a twin Axes sharing the yaxis

        Create a new Axes instance with an invisible y-axis and an independent
        x-axis positioned opposite to the original one (i.e. at top). The
        y-axis autoscale setting will be inherited from the original Axes.
        To ensure that the tick marks of both x-axes align, see
        `~matplotlib.ticker.LinearLocator`

        Returns
        -------
        ax_twin : Axes
            The newly created Axes instance

        Notes
        -----
        For those who are 'picking' artists while using twiny, pick
        events are only called for the artists in the top-most axes.
        """
        ax2 = self._make_twin_axes(sharey=self, projection="mantid")
        ax2.xaxis.tick_top()
        ax2.xaxis.set_label_position("top")
        ax2.set_autoscaley_on(self.get_autoscaley_on())
        self.xaxis.tick_bottom()
        ax2.yaxis.set_visible(False)
        ax2.patch.set_visible(False)
        return ax2

    @plot_decorator
    def plot(self, *args, **kwargs):
        """
        If the **mantid** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes.plot` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
            ax.plot(workspace,'rs',specNum=1) #for workspaces
            ax.plot(x,y,'bo')                 #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions.plot`.
        """
        if datafunctions.validate_args(*args, **kwargs):
            logger.debug("using plotfunctions")

            autoscale_on = kwargs.pop("autoscale_on_update", self.get_autoscale_on())

            def _data_update(artists, workspace, new_kwargs=None):
                # It's only possible to plot 1 line at a time from a workspace
                try:
                    if new_kwargs:
                        x, y, _, _ = axesfunctions._plot_impl(self, workspace, args, new_kwargs)
                    else:
                        x, y, _, _ = axesfunctions._plot_impl(self, workspace, args, kwargs)
                    artists[0].set_data(x, y)
                except RuntimeError as ex:
                    # if curve couldn't be plotted then remove it - can happen if the workspace doesn't contain the
                    # spectrum any more following execution of an algorithm
                    logger.information("Curve not plotted: {0}".format(ex.args[0]))

                    # remove the curve using similar to logic that in _WorkspaceArtists._remove
                    artists[0].remove()

                    # blank out list that will be returned
                    artists = []

                    # also remove the curve from the legend
                    if (not self.is_empty(self)) and self.legend_ is not None:
                        legend_set_draggable(self.legend(), True)

                if new_kwargs:
                    _autoscale_on = new_kwargs.pop("autoscale_on_update", self.get_autoscale_on())
                else:
                    _autoscale_on = self.get_autoscale_on()

                if _autoscale_on:
                    self.relim()
                    self.autoscale()
                return artists

            workspace = args[0]
            spec_num = self.get_spec_number_or_bin(workspace, kwargs)
            normalize_by_bin_width, kwargs = get_normalize_by_bin_width(workspace, self, **kwargs)
            is_normalized = normalize_by_bin_width or (hasattr(workspace, "isDistribution") and workspace.isDistribution())
            kwargs = get_plot_specific_properties(workspace, workspace.getPlotType(), kwargs)
            with autoscale_on_update(self, autoscale_on):
                artist = self.track_workspace_artist(
                    workspace,
                    axesfunctions.plot(self, normalize_by_bin_width=is_normalized, *args, **kwargs),
                    _data_update,
                    spec_num,
                    is_normalized,
                    MantidAxes.is_axis_of_type(MantidAxType.SPECTRUM, kwargs),
                    kwargs.get("LogName", None),
                    kwargs.get("Filtered", None),
                    kwargs.get("ExperimentInfo", None),
                )
            return artist
        else:
            lines = Axes.plot(self, *args, **kwargs)
            # matplotlib 3.5.0 operates a lazy process for updating the view limits where they get updated
            # on call to ax.draw instead of ax.plot. This doesn't work nicely with the Mantid requirement
            # to toggle autoscale on and off so access the view limits here to update immediately
            self.viewLim
            return lines

    @plot_decorator
    def scatter(self, *args, **kwargs):
        """
        If the **mantid** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes.scatter` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
            ax.scatter(workspace,'rs',specNum=1) #for workspaces
            ax.scatter(x,y,'bo')                 #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions.scatter`
        """
        if datafunctions.validate_args(*args):
            logger.debug("using plotfunctions")
        else:
            return Axes.scatter(self, *args, **kwargs)

    @plot_decorator
    def errorbar(self, *args, **kwargs):
        """
        If the **mantid** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes.errorbar` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
            ax.errorbar(workspace,'rs',specNum=1) #for workspaces
            ax.errorbar(x,y,yerr,'bo')            #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions.errorbar`
        """
        if datafunctions.validate_args(*args):
            logger.debug("using plotfunctions")

            autoscale_on = kwargs.pop("autoscale_on_update", self.get_autoscale_on())

            def _data_update(artists, workspace, new_kwargs=None):
                if new_kwargs:
                    _autoscale_on = new_kwargs.pop("autoscale_on_update", self.get_autoscale_on())
                else:
                    _autoscale_on = self.get_autoscale_on()
                # errorbar with workspaces can only return a single container
                container_orig = artists[0]
                # It is not possible to simply reset the error bars so
                # we have to plot new lines but ensure we don't reorder them on the plot!
                orig_idx = self.containers.index(container_orig)
                container_orig.remove()
                # The container does not remove itself from the containers list
                # but protect this just in case matplotlib starts doing this
                try:
                    self.containers.remove(container_orig)
                except ValueError:
                    pass
                # this gets pushed back onto the containers list
                try:
                    with autoscale_on_update(self, _autoscale_on):
                        if new_kwargs:
                            container_new = axesfunctions.errorbar(self, workspace, **new_kwargs)
                        else:
                            container_new = axesfunctions.errorbar(self, workspace, **kwargs)

                    self.containers.insert(orig_idx, container_new)
                    self.containers.pop()
                    # Update joining line
                    if container_new[0] and container_orig[0]:
                        container_new[0].update_from(container_orig[0])
                    # Update caps
                    for orig_caps, new_caps in zip(container_orig[1], container_new[1]):
                        new_caps.update_from(orig_caps)
                    # Update bars
                    for orig_bars, new_bars in zip(container_orig[2], container_new[2]):
                        new_bars.update_from(orig_bars)
                    # Re-plotting in the config dialog will assign this attr
                    if hasattr(container_orig, "errorevery"):
                        setattr(container_new, "errorevery", container_orig.errorevery)

                    # ax.relim does not support collections...
                    self._update_line_limits(container_new[0])
                except RuntimeError as ex:
                    logger.information("Error bar not plotted: {0}".format(ex.args[0]))
                    container_new = []
                    # also remove the curve from the legend
                    if (not self.is_empty(self)) and self.legend_ is not None:
                        legend_set_draggable(self.legend(), True)

                return container_new

            workspace = args[0]
            spec_num = self.get_spec_number_or_bin(workspace, kwargs)
            normalize_by_bin_width, kwargs = get_normalize_by_bin_width(workspace, self, **kwargs)
            is_normalized = normalize_by_bin_width or (hasattr(workspace, "isDistribution") and workspace.isDistribution())
            with autoscale_on_update(self, autoscale_on):
                artist = self.track_workspace_artist(
                    workspace,
                    axesfunctions.errorbar(self, normalize_by_bin_width=is_normalized, *args, **kwargs),
                    _data_update,
                    spec_num,
                    is_normalized,
                    MantidAxes.is_axis_of_type(MantidAxType.SPECTRUM, kwargs),
                )
            return artist
        else:
            errorbar_container = Axes.errorbar(self, *args, **kwargs)
            # matplotlib 3.5.0 operates a lazy process for updating the view limits where they get updated
            # on call to ax.draw instead of ax.errorbar. This doesn't work nicely with the Mantid requirement
            # to toggle autoscale on and off so access the view limits here to update immediately
            self.viewLim
            return errorbar_container

    @plot_decorator
    def axhline(self, *args, **kwargs):
        return Axes.axhline(self, *args, **kwargs)

    @plot_decorator
    def axvline(self, *args, **kwargs):
        return Axes.axvline(self, *args, **kwargs)

    @plot_decorator
    def pcolor(self, *args, **kwargs):
        """
        If the **mantid** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes.pcolor` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
            ax.pcolor(workspace) #for workspaces
            ax.pcolor(x,y,C)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions.pcolor`
        """
        return self._plot_2d_func("pcolor", *args, **kwargs)

    @plot_decorator
    def pcolorfast(self, *args, **kwargs):
        """
        If the **mantid** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes.pcolorfast` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matpolotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
            ax.pcolorfast(workspace) #for workspaces
            ax.pcolorfast(x,y,C)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions.pcolorfast`
        """
        return self._plot_2d_func("pcolorfast", *args, **kwargs)

    @plot_decorator
    def pcolormesh(self, *args, **kwargs):
        """
        If the **mantid** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes.pcolormesh` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
            ax.pcolormesh(workspace) #for workspaces
            ax.pcolormesh(x,y,C)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions.pcolormesh`
        """
        return self._plot_2d_func("pcolormesh", *args, **kwargs)

    @plot_decorator
    def imshow(self, *args, **kwargs):
        """
        If the **mantid** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes.imshow` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
            ax.imshow(workspace) #for workspaces
            ax.imshow(C)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions.imshow`
        """
        return self._plot_2d_func("imshow", *args, **kwargs)

    def _plot_2d_func(self, name, *args, **kwargs):
        """
        Implementation of pcolor-style methods
        :param name: The name of the method
        :param args: The args passed from the user
        :param kwargs: The kwargs passed from the use
        :return: The return value of the pcolor* function
        """
        plotfunctions_func = getattr(axesfunctions, name)
        if datafunctions.validate_args(*args):
            logger.debug("using plotfunctions")

            def _update_data(artists, workspace, new_kwargs=None):
                if new_kwargs:
                    return self._redraw_colorplot(plotfunctions_func, artists, workspace, **new_kwargs)
                return self._redraw_colorplot(plotfunctions_func, artists, workspace, **kwargs)

            workspace = args[0]
            normalize_by_bin_width, _ = get_normalize_by_bin_width(workspace, self, **kwargs)
            is_normalized = normalize_by_bin_width or (hasattr(workspace, "isDistribution") and workspace.isDistribution())
            # We return the last mesh so the return type is a single artist like the standard Axes
            artists = self.track_workspace_artist(
                workspace, plotfunctions_func(self, *args, **kwargs), _update_data, is_normalized=is_normalized
            )
            try:
                return artists[-1]
            except TypeError:
                return artists
        else:
            return getattr(Axes, name)(self, *args, **kwargs)

    def _redraw_colorplot(self, colorfunc, artists_orig, workspace, **kwargs):
        """
        Redraw a pcolor*, imshow or contour type plot based on a new workspace
        :param colorfunc: The Axes function to use to draw the new artist
        :param artists_orig: A reference to an iterable of existing artists
        :param workspace: A reference to the workspace object
        :param kwargs: Any kwargs passed to the original call
        """
        for artist_orig in artists_orig:
            if hasattr(artist_orig, "remove"):
                artist_orig.remove()
            else:  # for contour plots remove the collections
                for col in artist_orig.collections:
                    col.remove()
            if hasattr(artist_orig, "colorbar_cid"):
                artist_orig.callbacks.disconnect(artist_orig.colorbar_cid)
        # If the colormap has been overridden then it needs to be passed in at
        # creation time
        if "colors" not in kwargs:
            kwargs["cmap"] = artists_orig[-1].cmap
        artists_new = colorfunc(self, workspace, **kwargs)
        # Copy properties from old to new
        if not isinstance(artists_new, Iterable):
            artists_new = [artists_new]
        # assume 1:1 match between old/new artist lists
        # and update relevant properties
        for src, dest in zip(artists_orig, artists_new):
            if hasattr(dest, "update_from"):
                dest.update_from(src)
            if hasattr(dest, "set_interpolation"):
                dest.set_interpolation(src.get_interpolation())
            dest.autoscale()
            dest.set_norm(src.norm)

        try:
            axesfunctions.update_colorplot_datalimits(self, artists_new)
        except ValueError:
            pass
        # the type of plot can mutate back to single image from a multi collection
        if len(artists_orig) == len(artists_new):
            for artist_orig, artist_new in zip(artists_orig, artists_new):
                if artist_orig.colorbar is not None:
                    self._attach_colorbar(artist_new, artist_orig.colorbar)
        else:
            # pick up the colorbar from the first one we find
            for artist_orig in artists_orig:
                if artist_orig.colorbar is not None:
                    self._attach_colorbar(artists_new[-1], artist_orig.colorbar)
                    break
            self.set_aspect("auto")
        return artists_new

    @plot_decorator
    def contour(self, *args, **kwargs):
        """
        If the **mantid** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes.contour` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
            ax.contour(workspace) #for workspaces
            ax.contour(x,y,z)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions.contour`
        """
        return self._plot_2d_func("contour", *args, **kwargs)

    @plot_decorator
    def contourf(self, *args, **kwargs):
        """
        If the **mantid** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes.contourf` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
            ax.contourf(workspace) #for workspaces
            ax.contourf(x,y,z)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions.contourf`
        """
        return self._plot_2d_func("contourf", *args, **kwargs)

    @plot_decorator
    def tripcolor(self, *args, **kwargs):
        """
        If the **mantid** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes.tripcolor` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
            ax.tripcolor(workspace) #for workspaces
            ax.tripcolor(x,y,C)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions.tripcolor`
        """
        return self._plot_2d_func("tripcolor", *args, **kwargs)

    @plot_decorator
    def tricontour(self, *args, **kwargs):
        """
        If the **mantid** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes.tricontour` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
            ax.tricontour(workspace) #for workspaces
            ax.tricontour(x,y,z)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions.tricontour`
        """
        return self._plot_2d_func("tricontour", *args, **kwargs)

    @plot_decorator
    def tricontourf(self, *args, **kwargs):
        """
        If the **mantid** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes.tricontourf` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
            ax.tricontourf(workspace) #for workspaces
            ax.tricontourf(x,y,z)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions.tricontourf`
        """
        return self._plot_2d_func("tricontourf", *args, **kwargs)

    def is_waterfall(self):
        return self.waterfall_x_offset != 0 or self.waterfall_y_offset != 0

    def update_waterfall(self, x_offset, y_offset):
        """
        Changes the offset of a waterfall plot.
        :param x_offset: The amount by which each line is shifted in the x axis.
        :param y_offset: The amount by which each line is shifted in the y axis.
        """
        x_offset = int(x_offset)
        y_offset = int(y_offset)

        errorbar_cap_lines = datafunctions.remove_and_return_errorbar_cap_lines(self)

        for i in range(len(self.get_lines())):
            datafunctions.convert_single_line_to_waterfall(self, i, x_offset, y_offset)

        if x_offset == 0 and y_offset == 0 and self.is_waterfall():
            self.set_waterfall_fill(False)
            logger.information("x and y offset have been set to zero so the plot is no longer a waterfall plot.")

        if self.waterfall_has_fill():
            datafunctions.waterfall_update_fill(self)

        self.waterfall_x_offset = x_offset
        self.waterfall_y_offset = y_offset

        for cap in errorbar_cap_lines:
            self.add_line(cap)

        datafunctions.set_waterfall_toolbar_options_enabled(self)

        self.relim()
        if not self.waterfall_has_fill():
            # The mpl ax.fill method already makes an autoscale request for us,
            # so we only need to do this if waterfall_update_fill hasn't been called
            self.autoscale()

        self.get_figure().canvas.draw()

    def set_waterfall(self, state, x_offset=None, y_offset=None, fill=False):
        """
        Convert between a normal 1D plot and a waterfall plot.
        :param state: If true convert the plot to a waterfall plot, otherwise convert to a 1D plot.
        :param x_offset: The amount by which each line is shifted in the x axis. Optional, default is 10.
        :param y_offset: The amount by which each line is shifted in the y axis. Optional, default is 20.
        :param fill: If true the area under each line is filled.
        :raises: RuntimeError if state is true but there are less than two lines on the plot, if state is true but
                 x_offset and y_offset are 0, or if state is false but x_offset or y_offset is non-zero or fill is True.
        """
        if state:
            if len(self.get_lines()) < 2:
                raise RuntimeError("Axis must have multiple lines to be converted to a waterfall plot.")

            if x_offset is None:
                x_offset = WATERFALL_XOFFSET_DEFAULT

            if y_offset is None:
                y_offset = WATERFALL_YOFFSET_DEFAULT

            if x_offset == 0 and y_offset == 0:
                raise RuntimeError("You have set waterfall to true but have set the x and y offsets to zero.")

            if self.is_waterfall():
                # If the plot is already a waterfall plot but the provided x or y offset value is different to the
                # current value, the new values are applied but a message is written to the logger to tell the user
                # that they can use the update_waterfall function to do this.
                if x_offset != self.waterfall_x_offset or y_offset != self.waterfall_y_offset:
                    logger.information(
                        "If your plot is already a waterfall plot you can use update_waterfall(x, y) to" " change its offset values."
                    )
                else:
                    # Nothing needs to be changed.
                    logger.information("Plot is already a waterfall plot.")
                    return

            # Set the width and height attributes if they haven't been already.
            if not hasattr(self, "width"):
                datafunctions.set_initial_dimensions(self)
        else:
            if bool(x_offset) or bool(y_offset) or fill:
                raise RuntimeError("You have set waterfall to false but have given a non-zero value for the offset or " "set fill to true.")

            if not self.is_waterfall():
                # Nothing needs to be changed.
                logger.information("Plot is already not a waterfall plot.")
                return

            x_offset = y_offset = 0

        self.update_waterfall(x_offset, y_offset)

        if fill:
            self.set_waterfall_fill(True)

    def waterfall_has_fill(self):
        return any(isinstance(collection, PolyCollection) for collection in self.collections)

    def set_waterfall_fill(self, enable, colour=None):
        """
        Toggle whether the area under each line on a waterfall plot is filled.
        :param enable: If true, the filled areas are created, otherwise they are removed.
        :param colour: Optional string for the colour of the filled areas. If None, the colour of each line is used.
        :raises: RuntimeError if enable is false but colour is not None.
        """
        if not self.is_waterfall():
            raise RuntimeError("Cannot toggle fill on non-waterfall plot.")

        if enable:
            datafunctions.waterfall_create_fill(self)

            if colour is None:
                datafunctions.line_colour_fill(self)
            else:
                datafunctions.solid_colour_fill(self, colour)
        else:
            if bool(colour):
                raise RuntimeError("You have set fill to false but have given a colour.")

            datafunctions.waterfall_remove_fill(self)

    def set_xscale(self, *args, **kwargs):
        is_waterfall = self.is_waterfall()
        if is_waterfall:
            # The waterfall must be turned off before the log can be changed or the offset will not scale correctly
            fill = self.waterfall_has_fill()
            x_offset = self.waterfall_x_offset
            y_offset = self.waterfall_y_offset
            self.set_waterfall(False)
        has_minor_ticks = not isinstance(self.xaxis.minor.locator, NullLocator)

        super().set_xscale(*args, **kwargs)

        if has_minor_ticks:
            self.minorticks_on()
        else:
            self.minorticks_off()
        if is_waterfall:
            self.set_waterfall(True, x_offset=x_offset, y_offset=y_offset, fill=fill)

    def set_yscale(self, *args, **kwargs):
        is_waterfall = self.is_waterfall()
        if is_waterfall:
            fill = self.waterfall_has_fill()
            x_offset = self.waterfall_x_offset
            y_offset = self.waterfall_y_offset
            self.set_waterfall(False)
        has_minor_ticks = not isinstance(self.yaxis.minor.locator, NullLocator)

        super().set_yscale(*args, **kwargs)

        if has_minor_ticks:
            self.minorticks_on()
        else:
            self.minorticks_off()
        if is_waterfall:
            self.set_waterfall(True, x_offset=x_offset, y_offset=y_offset, fill=fill)

    def grid_on(self):
        return self.xaxis._major_tick_kw.get("gridOn", False) and self.yaxis._major_tick_kw.get("gridOn", False)

    # ------------------ Private api --------------------------------------------------------

    def _attach_colorbar(self, mappable, colorbar):
        """
        Attach the given colorbar to the mappable and update the clim values
        :param mappable: An instance of a mappable
        :param colorbar: An instance of a colorbar
        """
        cb = colorbar
        cb.mappable = mappable
        mappable.colorbar = cb
        mappable.colorbar_cid = mappable.callbacks.connect("changed", cb.update_normal)
        cb.update_normal(mappable)

    def _remove_matching_curve_from_creation_args(self, workspace_name, workspace_index, spec_num):
        """
        Finds a curve from the same workspace and index, then removes it from the creation args.

        :param workspace_name: Name of the workspace from which the curve was plotted
        :type workspace_name: str
        :param workspace_index: Index in the workspace that contained the data
        :type workspace_index: int
        :param spec_num: Spectrum number that contained the data. Used if the workspace was plotted using specNum kwarg.
                         Workspace index has priority if both are provided.
        :type spec_num: int
        :raises ValueError: if the curve does not exist in the creation_args of the axis
        :returns: None
        """
        for index, creation_arg in enumerate(self.creation_args):  # type: int, dict
            if workspace_name == creation_arg["workspaces"]:
                if creation_arg.get("wkspIndex", -1) == workspace_index or creation_arg.get("specNum", -1) == spec_num:
                    del self.creation_args[index]
                    return
        raise ValueError("Curve does not have existing creation args")


# -----------------------------------------------------------------------------
# MantidAxes3D
# -----------------------------------------------------------------------------


class MantidAxes3D(Axes3D):
    """
    This class defines the **mantid3d** projection for 3d plotting. One chooses
    this projection using::

        import matplotlib.pyplot as plt
        from mantid import plots
        fig, ax = plt.subplots(subplot_kw={'projection':'mantid3d'})

    or::

        import matplotlib.pyplot as plt
        from mantid import plots
        fig = plt.figure()
        ax = fig.add_subplot(111,projection='mantid3d')

    The mantid3d projection allows replacing the array objects with mantid workspaces.
    """

    name = "mantid3d"

    def __init__(self, *args, **kwargs):
        kwargs["auto_add_to_figure"] = False
        super().__init__(*args, **kwargs)
        # By default, when right click is held the plot will zoom in and out.
        # For Mantid plots right click will open a context menu instead
        # Unassigning the zoom button avoids zoom becoming toggled after leaving the context menu
        self.mouse_init(zoom_btn=None)

    def set_title(self, *args, **kwargs):
        # The set_title function in Axes3D also moves the title downwards for some reason so the Axes function is called
        # instead.
        return Axes.set_title(self, *args, **kwargs)

    def set_xlim3d(self, *args, **kwargs):
        super().set_xlim3d(*args, **kwargs)
        self._set_overflowing_data_to_nan(0)

    def set_ylim3d(self, *args, **kwargs):
        super().set_ylim3d(*args, **kwargs)
        self._set_overflowing_data_to_nan(1)

    def set_zlim3d(self, *args, **kwargs):
        super().set_zlim3d(*args, **kwargs)
        self._set_overflowing_data_to_nan(2)

    def autoscale(self, *args, **kwargs):
        super().autoscale(*args, **kwargs)
        self._set_overflowing_data_to_nan()

    def grid_on(self):
        return self._draw_grid

    def _set_overflowing_data_to_nan(self, axis_index=None):
        """
        Sets any data for the given axis that is less than min[axis_index] or greater than max[axis_index]
        to nan so only the parts of the plot that are within the axes are visible.
        :param axis_index: the index of the axis being edited, 0 for x, 1 for y, 2 for z.
        """

        min_vals, max_vals = zip(self.get_xlim3d(), self.get_ylim3d(), self.get_zlim3d())
        if hasattr(self, "original_data_surface"):
            if axis_index is None:
                axis_index_list = [0, 1, 2]
            else:
                axis_index_list = [axis_index]

            # if original_data_surface does not match current collections then we shouldn't add it back,
            # delete attribute and skip adding back.
            if self.collections[0]._vec[axis_index_list[0]].size != self.original_data_surface[axis_index_list[0]].size:
                delattr(self, "original_data_surface")
            else:
                for axis_index in axis_index_list:
                    axis_data = self.original_data_surface[axis_index].copy()
                    axis_data[np.less(axis_data, min_vals[axis_index], where=~np.isnan(axis_data))] = np.nan
                    axis_data[np.greater(axis_data, max_vals[axis_index], where=~np.isnan(axis_data))] = np.nan
                    self.collections[0]._vec[axis_index] = axis_data

        if hasattr(self, "original_data_wireframe"):
            all_data = copy.deepcopy(self.original_data_wireframe)

            for spectrum in range(len(all_data)):
                spectrum_data = all_data[spectrum]
                for point in range(len(spectrum_data)):
                    for axis in range(3):
                        if np.less(spectrum_data[point][axis], min_vals[axis]) or np.greater(spectrum_data[point][axis], max_vals[axis]):
                            all_data[spectrum][point] = np.repeat(np.nan, 3)

            self.collections[0].set_segments(all_data)

    def plot(self, *args, **kwargs):
        """
        If the **mantid3d** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes3D.plot` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid3d'})
            ax.plot(workspace) #for workspaces
            ax.plot(x,y,z)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions3D.plot3D`
        """
        if datafunctions.validate_args(*args):
            logger.debug("using plotfunctions3D")
            return axesfunctions3D.plot(self, *args, **kwargs)
        else:
            return Axes3D.plot(self, *args, **kwargs)

    def scatter(self, *args, **kwargs):
        """
        If the **mantid3d** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes3D.scatter` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid3d'})
            ax.scatter(workspace) #for workspaces
            ax.scatter(x,y,z)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions3D.scatter`
        """
        if datafunctions.validate_args(*args):
            logger.debug("using plotfunctions3D")
            return axesfunctions3D.scatter(self, *args, **kwargs)
        else:
            return Axes3D.scatter(self, *args, **kwargs)

    def plot_wireframe(self, *args, **kwargs):
        """
        If the **mantid3d** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes3D.plot_wireframe` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid3d'})
            ax.plot_wireframe(workspace) #for workspaces
            ax.plot_wireframe(x,y,z)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions3D.wireframe`
        """
        if datafunctions.validate_args(*args):
            logger.debug("using plotfunctions3D")
            line_c = axesfunctions3D.plot_wireframe(self, *args, **kwargs)
        else:
            line_c = Axes3D.plot_wireframe(self, *args, **kwargs)

        # Create a copy of the original data points because data are set to nan when the axis limits are changed.
        self.original_data_wireframe = copy.deepcopy(line_c._segments3d)

        return line_c

    def plot_surface(self, *args, **kwargs):
        """
        If the **mantid3d** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes3D.plot_surface` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid3d'})
            ax.plot_surface(workspace) #for workspaces
            ax.plot_surface(x,y,z)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions3D.plot_surface`
        """
        if datafunctions.validate_args(*args):
            logger.debug("using plotfunctions3D")
            poly_c = axesfunctions3D.plot_surface(self, *args, **kwargs)
        else:
            poly_c = Axes3D.plot_surface(self, *args, **kwargs)

            # This is a bit of a hack, should be able to remove
            # when matplotlib supports plotting masked arrays
            if poly_c._A is not None:
                poly_c._A = safe_masked_invalid(poly_c._A)

        # Create a copy of the original data points because data are set to nan when the axis limits are changed.
        self.original_data_surface = copy.deepcopy(poly_c._vec)

        return poly_c

    def contour(self, *args, **kwargs):
        """
        If the **mantid3d** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes3D.contour` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid3d'})
            ax.contour(workspace) #for workspaces
            ax.contour(x,y,z)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions3D.contour`
        """
        if datafunctions.validate_args(*args):
            logger.debug("using plotfunctions3D")
            return axesfunctions3D.contour(self, *args, **kwargs)
        else:
            return Axes3D.contour(self, *args, **kwargs)

    def contourf(self, *args, **kwargs):
        """
        If the **mantid3d** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes3D.contourf` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid3d'})
            ax.contourf(workspace) #for workspaces
            ax.contourf(x,y,z)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions3D.contourf`
        """
        if datafunctions.validate_args(*args):
            logger.debug("using plotfunctions3D")
            return axesfunctions3D.contourf(self, *args, **kwargs)
        else:
            return Axes3D.contourf(self, *args, **kwargs)

    def set_mesh_axes_equal(self, mesh):
        """Set the 3D axes centred on the provided mesh and with the aspect ratio equal"""

        ax_limits = mesh.flatten()
        self.auto_scale_xyz(ax_limits[0::3], ax_limits[1::3], ax_limits[2::3])
        x_limits = self.get_xlim3d()
        y_limits = self.get_ylim3d()
        z_limits = self.get_zlim3d()

        x_range = abs(x_limits[1] - x_limits[0])
        x_middle = np.mean(x_limits)
        y_range = abs(y_limits[1] - y_limits[0])
        y_middle = np.mean(y_limits)
        z_range = abs(z_limits[1] - z_limits[0])
        z_middle = np.mean(z_limits)
        plot_radius = 0.5 * max([x_range, y_range, z_range])

        self.set_xlim3d([x_middle - plot_radius, x_middle + plot_radius])
        self.set_ylim3d([y_middle - plot_radius, y_middle + plot_radius])
        self.set_zlim3d([z_middle - plot_radius, z_middle + plot_radius])


class _WorkspaceArtists(object):
    """Captures information regarding an artist that has been plotted
    from a workspace. It allows for removal and replacement of said artists

    """

    def __init__(
        self,
        artists,
        data_replace_cb,
        is_normalized,
        workspace_name=None,
        spec_num=None,
        is_spec=True,
        log_name=None,
        filtered=True,
        expt_info_index=None,
    ):
        """
        Initialize an instance
        :param artists: A reference to a list of artists "attached" to a workspace
        :param data_replace_cb: A reference to a callable with signature (artists, workspace) -> new_artists
        :param is_normalized: bool specifying whether the line being plotted is a distribution
        :param workspace_name: String. The name of the associated workspace
        :param spec_num: The spectrum number of the spectrum used to plot the artist
        :param is_spec: True if spec_num represents a spectrum rather than a bin
        :param log_name: string. The name of the plotted log
        :param filtered: bool. True if log plotted was filtered, and False if unfiltered.
            This only has meaning if log_name is not None.
        :param expt_info_index: Integer. The index of the experiment info for this plotted log.
            This only has meaning if log_name is not None.
        """
        self._set_artists(artists)
        self._data_replace_cb = data_replace_cb
        self.workspace_name = workspace_name
        self.spec_num = spec_num
        self.is_spec = is_spec
        self.workspace_index = self._get_workspace_index()
        self.is_normalized = is_normalized
        self.log_name = log_name
        self.filtered = filtered
        self.expt_info_index = expt_info_index

    def _get_workspace_index(self):
        """Get the workspace index (spectrum or bin index) of the workspace artist"""
        if self.spec_num is None or self.workspace_name is None:
            return None
        try:
            if self.is_spec:
                return ads.retrieve(self.workspace_name).getIndexFromSpectrumNumber(self.spec_num)
            else:
                return self.spec_num
        except KeyError:  # Return None if the workspace is not in the ADS
            return None

    def remove(self, axes):
        """
        Remove the tracked artists from the given axes
        :param axes: A reference to the axes instance the artists are attached to
        """
        # delete the artists from the axes
        self._remove(axes, self._artists)

    def remove_if(self, axes, predicate):
        """
        Remove the tracked artists from the given axes if they return true from predicate
        :param axes: A reference to the axes instance the artists are attached to
        :param predicate: A function which takes a matplotlib artist object and returns a boolean
        :returns: Returns a bool specifying whether the class is now empty
        """
        artists_to_remove = []
        artists_to_keep = []
        for artist in self._artists:
            if predicate(artist):
                artists_to_remove.append(artist)
            else:
                artists_to_keep.append(artist)

        self._remove(axes, artists_to_remove)
        self._artists = artists_to_keep

        return len(self._artists) == 0

    def _remove(self, axes, artists):
        # delete the artists from the axes
        for artist in artists:
            artist.remove()
            # Remove doesn't catch removing the container for errorbars etc
            if isinstance(artist, Container):
                try:
                    axes.containers.remove(artist)
                except ValueError:
                    pass

        if (not axes.is_empty(axes)) and axes.legend_ is not None:
            axes.make_legend()

    def replace_data(self, workspace, plot_kwargs=None):
        """Replace or replot artists based on a new workspace
        :param workspace: The new workspace containing the data
        :param plot_kwargs: Key word args to pass to plotting function
        """
        if plot_kwargs:
            new_artists = self._data_replace_cb(self._artists, workspace, plot_kwargs)
        else:
            new_artists = self._data_replace_cb(self._artists, workspace)
        self._set_artists(new_artists)
        return len(self._artists) == 0

    def rename_data(self, new_name):
        """
        Rename a workspace and update the artists label
        """
        self._update_artist_label_with_new_workspace_name(new_name)
        self.workspace_name = new_name

    def _set_artists(self, artists):
        """Ensure the stored artists is an iterable"""
        if isinstance(artists, Container) or not isinstance(artists, Iterable):
            self._artists = [artists]
        else:
            self._artists = artists

    def _update_artist_label_with_new_workspace_name(self, new_workspace_name):
        for artist in self._artists:
            old_workspace_name = self.workspace_name
            prev_label = artist.get_label()
            artist.set_label(re.sub(rf"\b{old_workspace_name}\b", new_workspace_name, prev_label))
