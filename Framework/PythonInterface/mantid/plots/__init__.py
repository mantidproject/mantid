# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
#
#
"""
Functionality for unpacking mantid objects for plotting with matplotlib.
"""

# This file should be left free of PyQt imports to allow quick importing
# of the main package.
from __future__ import (absolute_import, division, print_function)

from collections import Iterable
from matplotlib.axes import Axes
from matplotlib.collections import Collection
from matplotlib.colors import Colormap
from matplotlib.container import Container, ErrorbarContainer
from matplotlib.image import AxesImage
from matplotlib.lines import Line2D
from matplotlib.patches import Patch
from matplotlib.projections import register_projection
from matplotlib.scale import register_scale
from matplotlib.table import Table

from mantid.plots import helperfunctions, plotfunctions
from mantid.plots import plotfunctions3D

try:
    from mpl_toolkits.mplot3d.axes3d import Axes3D
except ImportError:
    # Special case to handle issues with importing mpl_toolkits
    #
    # Matplotlib adds a *nspkg.pth file to the user site packages directory.
    # When that file is processed a fake built-in mpl_toolkits is imported
    # which forces the site packages version to take precidence over our
    # local copy regardless of python sys path settings.
    #
    # Work around by removing the fake built-in module from sys modules,
    # then forcing python to search the path as expected.
    #
    # This is mostly but not necessarily limited to being an issue on OSX
    # where there are multiple versions of matplotlib installed across the
    # system.
    import sys

    del sys.modules['mpl_toolkits']
    from mpl_toolkits.mplot3d.axes3d import Axes3D

from mantid.api import AnalysisDataService as ads
from mantid.kernel import logger
from mantid.plots import helperfunctions, plotfunctions
from mantid.plots.helperfunctions import get_normalize_by_bin_width
from mantid.plots import plotfunctions3D
from mantid.plots.scales import PowerScale, SquareScale


def plot_decorator(func):
    def wrapper(self, *args, **kwargs):
        func_value = func(self, *args, **kwargs)
        # Saves saving it on array objects
        if helperfunctions.validate_args(*args, **kwargs):
            # Fill out kwargs with the values of args
            kwargs["workspaces"] = args[0].name()
            kwargs["function"] = func.__name__
            if "cmap" in kwargs and isinstance(kwargs["cmap"], Colormap):
                kwargs["cmap"] = kwargs["cmap"].name
            self.creation_args.append(kwargs)
        return func_value

    return wrapper


class _WorkspaceArtists(object):
    """Captures information regarding an artist that has been plotted
    from a workspace. It allows for removal and replacement of said artists

    """
    def __init__(self, artists, data_replace_cb, is_normalized, workspace_name=None,
                 spec_num=None, is_spec=True):
        """
        Initialize an instance
        :param artists: A reference to a list of artists "attached" to a workspace
        :param data_replace_cb: A reference to a callable with signature (artists, workspace) -> new_artists
        :param is_normalized: bool specifying whether the line being plotted is a distribution
        :param workspace_name: String. The name of the associated workspace
        :param spec_num: The spectrum number of the spectrum used to plot the artist
        :param is_spec: True if spec_num represents a spectrum rather than a bin
        """
        self._set_artists(artists)
        self._data_replace_cb = data_replace_cb
        self.workspace_name = workspace_name
        self.spec_num = spec_num
        self.is_spec = is_spec
        self.workspace_index = self._get_workspace_index()
        self.is_normalized = is_normalized

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
            axes.legend().draggable()

    def replace_data(self, workspace, plot_kwargs=None):
        """Replace or replot artists based on a new workspace
        :param workspace: The new workspace containing the data
        :param plot_kwargs: Key word args to pass to plotting function
        """
        if plot_kwargs:
            new_artists = self._data_replace_cb(self._artists, workspace,
                                                plot_kwargs)
        else:
            new_artists = self._data_replace_cb(self._artists, workspace)
        self._set_artists(new_artists)

    def _set_artists(self, artists):
        """Ensure the stored artists is an iterable"""
        if isinstance(artists, Container) or not isinstance(artists, Iterable):
            self._artists = [artists]
        else:
            self._artists = artists


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
    name = 'mantid'

    # Enumerators for plotting directions
    HORIZONTAL = BIN = 0
    VERTICAL = SPECTRUM = 1

    # Store information for any workspaces attached to this axes instance
    tracked_workspaces = None

    def __init__(self, *args, **kwargs):
        super(MantidAxes, self).__init__(*args, **kwargs)
        self.tracked_workspaces = dict()
        self.creation_args = []

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
        prop_cycler = ax._get_lines.prop_cycler  # tracks line color cycle
        artists = ax.get_children()
        mantid_axes = ax.figure.add_subplot(111, projection='mantid',
                                            label='mantid')
        for artist in artists:
            if not any(isinstance(artist, artist_type) for artist_type in
                       ignore_artists):
                try:
                    mantid_axes.add_artist_correctly(artist)
                except NotImplementedError:
                    pass
        mantid_axes.set_title(ax.get_title())
        mantid_axes._get_lines.prop_cycler = prop_cycler
        ax.remove()
        return mantid_axes

    @staticmethod
    def get_spec_num_from_wksp_index(workspace, wksp_index):
        return workspace.getSpectrum(wksp_index).getSpectrumNo()

    @staticmethod
    def get_spec_number_or_bin(workspace, kwargs):
        if kwargs.get('specNum', None) is not None:
            return kwargs['specNum']
        elif kwargs.get('wkspIndex', None) is not None:
            # If wanting to plot a bin
            if kwargs.get('axis', None) is not None and kwargs.get('axis', None) == 0:
                return kwargs['wkspIndex'], False
            # If wanting to plot a spectrum
            else:
                return MantidAxes.get_spec_num_from_wksp_index(workspace, kwargs['wkspIndex']), True
        else:
            return None

    def get_artists_workspace_and_spec_num(self, artist):
        """Retrieve the workspace and spec num of the given artist"""
        for ws_name, ws_artists_list in self.tracked_workspaces.items():
            for ws_artists in ws_artists_list:
                for ws_artist in ws_artists._artists:
                    if artist == ws_artist:
                        return ads.retrieve(ws_name), ws_artists.spec_num
        raise ValueError("Artist: '{}' not tracked by axes.".format(artist))

    def get_artist_normalization_state(self, artist):
        for ws_name, ws_artists_list in self.tracked_workspaces.items():
            for ws_artists in ws_artists_list:
                for ws_artist in ws_artists._artists:
                    if artist == ws_artist:
                        return ws_artists.is_normalized

    def track_workspace_artist(self, workspace, artists, data_replace_cb=None,
                               spec_num=None, is_normalized=None, is_spec=True):
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
        :returns: The artists variable as it was passed in.
        """
        name = workspace.name()
        if name:
            if data_replace_cb is None:
                def data_replace_cb(_, __):
                    logger.warning("Updating data on this plot type is not yet supported")
            artist_info = self.tracked_workspaces.setdefault(name, [])

            artist_info.append(_WorkspaceArtists(artists, data_replace_cb,
                                                 is_normalized, name,
                                                 spec_num, is_spec))
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
                logger.warning("You are overlaying distribution and "
                               "non-distribution data!")

    def artists_workspace_has_errors(self, artist):
        """Check if the given artist's workspace has errors"""
        if artist not in self.get_tracked_artists():
            raise ValueError("Artist '{}' is not tracked and so does not have "
                             "an associated workspace.".format(artist))
        workspace, spec_num = self.get_artists_workspace_and_spec_num(artist)
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
        """
        Remove the artists reference by this workspace (if any) and return True
        if the axes is then empty
        :param workspace: A Workspace object
        :return: True if the axes is empty, false if artists remain or this workspace is not associated here
        """
        try:
            # pop to ensure we don't hold onto an artist reference
            artist_info = self.tracked_workspaces.pop(workspace.name())
        except KeyError:
            return False

        for workspace_artist in artist_info:
            workspace_artist.remove(self)

        return self.is_empty(self)

    def remove_artists_if(self, unary_predicate):
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
        is_empty_list = []
        for workspace_artist in artist_info:
            empty = workspace_artist.remove_if(self, unary_predicate)
            is_empty_list.append(empty)

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

        for workspace_artist in artist_info:
            workspace_artist.replace_data(workspace)
        return True

    def replot_artist(self, artist, errorbars=False, **kwargs):
        """
        Replot an artist with a new set of kwargs via 'plot' or 'errorbar'
        :param artist: The artist to replace
        :param errorbars: Plot with or without errorbars
        :returns: The new artist that has been plotted
        For keywords related to workspaces, see :func:`plotfunctions.plot` or
        :func:`plotfunctions.errorbar`
        """
        kwargs['distribution'] = not self.get_artist_normalization_state(artist)

        workspace, spec_num = self.get_artists_workspace_and_spec_num(artist)
        self.remove_artists_if(lambda art: art == artist)
        workspace_index = workspace.getIndexFromSpectrumNumber(spec_num)
        self._remove_matching_curve_from_creation_args(workspace.name(), workspace_index, spec_num)

        if errorbars:
            new_artist = self.errorbar(workspace, wkspIndex=workspace_index, **kwargs)
        else:
            new_artist = self.plot(workspace, wkspIndex=workspace_index, **kwargs)
        return new_artist

    def relim(self, visible_only=True):
        Axes.relim(self, visible_only)  # relim on any non-errorbar objects
        lower_xlim, lower_ylim = self.dataLim.get_points()[0]
        upper_xlim, upper_ylim = self.dataLim.get_points()[1]
        for container in self.containers:
            if isinstance(container, ErrorbarContainer) and (
                    (visible_only and not helperfunctions.errorbars_hidden(container)) or
                    not visible_only):
                min_x, max_x, min_y, max_y = helperfunctions.get_errorbar_bounds(container)
                lower_xlim = min(lower_xlim, min_x) if min_x else lower_xlim
                upper_xlim = max(upper_xlim, max_x) if max_x else upper_xlim
                lower_ylim = min(lower_ylim, min_y) if min_y else lower_ylim
                upper_ylim = max(upper_ylim, max_y) if max_y else upper_ylim

        xys = [[lower_xlim, lower_ylim], [upper_xlim, upper_ylim]]
        # update_datalim will update limits with union of current lims and xys
        self.update_datalim(xys)

    @staticmethod
    def is_empty(axes):
        """
        Checks the known artist containers to see if anything exists within them
        :return: True if no artists exist, false otherwise
        """

        def _empty(container):
            return len(container) == 0

        return (_empty(axes.lines) and _empty(axes.images) and
                _empty(axes.collections) and _empty(axes.containers))

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
        ax2 = self._make_twin_axes(sharex=self, projection='mantid')
        ax2.yaxis.tick_right()
        ax2.yaxis.set_label_position('right')
        ax2.yaxis.set_offset_position('right')
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
        ax2 = self._make_twin_axes(sharey=self, projection='mantid')
        ax2.xaxis.tick_top()
        ax2.xaxis.set_label_position('top')
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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions')

            autoscale_on_update = kwargs.pop("autoscale_on_update", True)

            def _data_update(artists, workspace, new_kwargs=None):
                # It's only possible to plot 1 line at a time from a workspace
                if new_kwargs:
                    x, y, _, __ = plotfunctions._plot_impl(self, workspace, args,
                                                           new_kwargs)
                else:
                    x, y, _, __ = plotfunctions._plot_impl(self, workspace, args,
                                                           kwargs)
                artists[0].set_data(x, y)
                self.relim()
                if autoscale_on_update:
                    self.autoscale()
                return artists

            workspace = args[0]
            spec_num, is_spec = self.get_spec_number_or_bin(workspace, kwargs)
            normalize_by_bin_width, kwargs = get_normalize_by_bin_width(
                workspace, self, **kwargs)
            is_normalized = normalize_by_bin_width or workspace.isDistribution()

            # If we are making the first plot on an axes object
            # i.e. self.lines is empty, axes has default ylim values.
            # Therefore we need to autoscale regardless of autoscale_on_update.
            if self.lines:
                # Otherwise set autoscale to autoscale_on_update.
                self.set_autoscaley_on(autoscale_on_update)

            artist = self.track_workspace_artist(
                workspace, plotfunctions.plot(self, *args, **kwargs),
                _data_update, spec_num, is_normalized, is_spec)

            self.set_autoscaley_on(True)
            return artist
        else:
            return Axes.plot(self, *args, **kwargs)

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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions')
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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions')

            autoscale_on_update = kwargs.pop("autoscale_on_update", True)

            def _data_update(artists, workspace, new_kwargs=None):
                if self.lines:
                    self.set_autoscaley_on(autoscale_on_update)

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
                if new_kwargs:
                    container_new = plotfunctions.errorbar(self, workspace,
                                                           **new_kwargs)
                else:
                    container_new = plotfunctions.errorbar(self, workspace,
                                                           **kwargs)
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
                if hasattr(container_orig, 'errorevery'):
                    setattr(container_new, 'errorevery', container_orig.errorevery)

                # ax.relim does not support collections...
                self._update_line_limits(container_new[0])
                self.set_autoscaley_on(True)
                return container_new

            workspace = args[0]
            spec_num, is_spec = self.get_spec_number_or_bin(workspace, kwargs)
            is_normalized, kwargs = get_normalize_by_bin_width(workspace, self,
                                                               **kwargs)

            if self.lines:
                self.set_autoscaley_on(autoscale_on_update)

            artist = self.track_workspace_artist(
                workspace, plotfunctions.errorbar(self, *args, **kwargs),
                _data_update, spec_num, is_normalized, is_spec)

            self.set_autoscaley_on(True)
            return artist
        else:
            return Axes.errorbar(self, *args, **kwargs)

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
        return self._pcolor_func('pcolor', *args, **kwargs)

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
        return self._pcolor_func('pcolorfast', *args, **kwargs)

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
        return self._pcolor_func('pcolormesh', *args, **kwargs)

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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions')

            def _update_data(artists, workspace):
                return self._redraw_colorplot(plotfunctions.imshow,
                                              artists, workspace, **kwargs)

            workspace = args[0]
            return self.track_workspace_artist(workspace,
                                               plotfunctions.imshow(self, *args, **kwargs),
                                               _update_data)
        else:
            return Axes.imshow(self, *args, **kwargs)

    def _pcolor_func(self, name, *args, **kwargs):
        """
        Implementation of pcolor-style methods
        :param name: The name of the method
        :param args: The args passed from the user
        :param kwargs: The kwargs passed from the use
        :return: The return value of the pcolor* function
        """
        plotfunctions_func = getattr(plotfunctions, name)
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions')

            def _update_data(artists, workspace, new_kwargs=None):
                if new_kwargs:
                    return self._redraw_colorplot(plotfunctions_func,
                                                  artists, workspace, **new_kwargs)
                return self._redraw_colorplot(plotfunctions_func,
                                              artists, workspace, **kwargs)

            workspace = args[0]
            # We return the last mesh so the return type is a single artist like the standard Axes
            artists = self.track_workspace_artist(workspace,
                                                  plotfunctions_func(self, *args, **kwargs),
                                                  _update_data)
            try:
                return artists[-1]
            except TypeError:
                return artists
        else:
            return getattr(Axes, name)(self, *args, **kwargs)

    def _redraw_colorplot(self, colorfunc, artists_orig, workspace,
                          **kwargs):
        """
        Redraw a pcolor* or imshow type plot bsaed on a new workspace
        :param colorfunc: The Axes function to use to draw the new artist
        :param artists_orig: A reference to an iterable of existing artists
        :param workspace: A reference to the workspace object
        :param kwargs: Any kwargs passed to the original call
        """
        for artist_orig in artists_orig:
            artist_orig.remove()
            if hasattr(artist_orig, 'colorbar_cid'):
                artist_orig.callbacksSM.disconnect(artist_orig.colorbar_cid)
        artists_new = colorfunc(self, workspace, **kwargs)
        if not isinstance(artists_new, Iterable):
            artists_new = [artists_new]
        plotfunctions.update_colorplot_datalimits(self, artists_new)
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
            self.set_aspect('auto')
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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions')
            workspace = args[0]
            return self.track_workspace_artist(workspace,
                                               plotfunctions.contour(self, *args, **kwargs))
        else:
            return Axes.contour(self, *args, **kwargs)

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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions')
            workspace = args[0]
            return self.track_workspace_artist(workspace,
                                               plotfunctions.contourf(self, *args, **kwargs))
        else:
            return Axes.contourf(self, *args, **kwargs)

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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions')
            workspace = args[0]
            return self.track_workspace_artist(workspace,
                                               plotfunctions.tripcolor(self, *args, **kwargs))
        else:
            return Axes.tripcolor(self, *args, **kwargs)

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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions')
            workspace = args[0]
            return self.track_workspace_artist(workspace,
                                               plotfunctions.tricontour(self, *args, **kwargs))
        else:
            return Axes.tricontour(self, *args, **kwargs)

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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions')
            workspace = args[0]
            return self.track_workspace_artist(workspace,
                                               plotfunctions.tricontourf(self, *args, **kwargs))
        else:
            return Axes.tricontourf(self, *args, **kwargs)

    # ------------------ Private api --------------------------------------------------------

    def _attach_colorbar(self, mappable, colorbar):
        """
        Attach the given colorbar to the mappable and update the clim values
        :param mappable: An instance of a mappable
        :param colorbar: An instance of a colorbar
        """
        cb = colorbar
        cb.mappable = mappable
        cb.set_clim(mappable.get_clim())
        mappable.colorbar = cb
        mappable.colorbar_cid = mappable.callbacksSM.connect('changed', cb.on_mappable_changed)
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

    name = 'mantid3d'

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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions3D')
            return plotfunctions3D.plot(self, *args, **kwargs)
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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions3D')
            return plotfunctions3D.scatter(self, *args, **kwargs)
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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions3D')
            return plotfunctions3D.plot_wireframe(self, *args, **kwargs)
        else:
            return Axes3D.plot_wireframe(self, *args, **kwargs)

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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions3D')
            return plotfunctions3D.plot_surface(self, *args, **kwargs)
        else:
            return Axes3D.plot_surface(self, *args, **kwargs)

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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions3D')
            return plotfunctions3D.contour(self, *args, **kwargs)
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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions3D')
            return plotfunctions3D.contourf(self, *args, **kwargs)
        else:
            return Axes3D.contourf(self, *args, **kwargs)


register_projection(MantidAxes)
register_projection(MantidAxes3D)
register_scale(PowerScale)
register_scale(SquareScale)
