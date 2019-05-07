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

from mantid.kernel import logger
from mantid.plots import helperfunctions, plotfunctions
from mantid.plots import plotfunctions3D
from mantid.plots.scales import PowerScale, SquareScale
from matplotlib import cbook
from matplotlib.axes import Axes
from matplotlib.collections import Collection
from matplotlib.colors import Colormap
from matplotlib.container import Container
from matplotlib.image import AxesImage
from matplotlib.lines import Line2D
from matplotlib.patches import Patch
from matplotlib.projections import register_projection
from matplotlib.scale import register_scale
from matplotlib.table import Table

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

from mantid.plots.helperfunctions import get_plot_as_distribution


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
    def __init__(self, artists, data_replace_cb, is_distribution,
                 spec_num=None):
        """
        Initialize an instance
        :param artists: A reference to a list of artists "attached" to a workspace
        :param data_replace_cb: A reference to a callable with signature (artists, workspace) -> new_artists
        :param is_distribution: bool specifying whether the line being plotted is a distribution
        :param spec_num: The spectrum number of the spectrum used to plot the artist
        """
        self._set_artists(artists)
        self._data_replace_cb = data_replace_cb
        self.spec_num = spec_num
        self.is_distribution = is_distribution

    def remove(self, axes):
        """
        Remove the tracked artists from the given axes
        :param axes: A reference to the axes instance the artists are attached to
        """
        # delete the artists from the axes
        for artist in self._artists:
            artist.remove()
            # Remove doesn't catch removing the container for errorbars etc
            if isinstance(artist, Container):
                try:
                    axes.containers.remove(artist)
                except ValueError:
                    pass

        if (not axes.is_empty(axes)) and axes.legend_ is not None:
            axes.legend().draggable()

    def replace_data(self, workspace):
        """Replace or replot artists based on a new workspace
        :param workspace: The new workspace containing the data
        """
        self._set_artists(self._data_replace_cb(self._artists, workspace))

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

    def _get_spec_number(self, workspace, kwargs):
        if kwargs.get('specNum', None) is not None:
            return kwargs['specNum']
        elif kwargs.get('wkspIndex', None) is not None:
            return self.get_spec_num_from_wksp_index(workspace,
                                                     kwargs['wkspIndex'])
        else:
            return None

    def track_workspace_artist(self, workspace, artists, is_distribution,
                               data_replace_cb=None, spec_num=None):
        """
        Add the given workspace's name to the list of workspaces
        displayed on this Axes instance
        :param workspace: A Workspace object. If empty then no tracking takes place
        :param artists: A single artist or iterable of artists containing the data for the workspace
        :param data_replace_cb: A function to call when the data is replaced to update
        the artist (optional)
        :param spec_num: The spectrum number associated with the artist (optional)
        :param is_distribution: bool. The line being plotted is a distribution.
            This can be from either a distribution workspace or a workspace being
            plotted as a distribution
        :returns: The artists variable as it was passed in.
        """
        name = workspace.name()
        if name:
            if data_replace_cb is None:
                def data_replace_cb(_, __):
                    logger.warning("Updating data on this plot type is not yet supported")
            artist_info = self.tracked_workspaces.setdefault(name, [])
            self.check_axes_distribution_consistency(is_distribution)
            artist_info.append(_WorkspaceArtists(artists, data_replace_cb,
                                                 is_distribution,
                                                 spec_num))
        return artists

    @staticmethod
    def _on_off_to_bool(on_or_off):
        if on_or_off.lower() == 'on':
            return True
        elif on_or_off.lower() == 'off':
            return False
        raise ValueError("Argument must be 'On' or 'Off'!")

    def check_axes_distribution_consistency(self, is_distribution):
        """
        Checks if new workspace to be plotted is consistent with current
        workspaces on axes in regard to being plotted as distributions.
        Displays a warning if not.
        """
        tracked_ws_distributions = []
        for artists in self.tracked_workspaces.values():
            for artist in artists:
                tracked_ws_distributions.append(artist.is_distribution)
        if len(tracked_ws_distributions) > 0:
            if any(tracked_ws_distributions) != is_distribution:
                logger.warning("You are overlaying distribution and "
                               "non-distribution data!")

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

    def remove_workspace_artist(self, workspace_name, artist):
        artist.remove(self)
        try:
            self.tracked_workspaces[workspace_name].remove(artist)
            if not self.tracked_workspaces[workspace_name]:
                self.tracked_workspaces.pop(workspace_name)
        except ValueError:
            return False
        return True

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

            def _data_update(artists, workspace):
                # It's only possible to plot 1 line at a time from a workspace
                x, y, _, __ = plotfunctions._plot_impl(self, workspace, args,
                                                       kwargs)
                artists[0].set_data(x, y)
                self.relim()
                self.autoscale()
                return artists

            workspace = args[0]
            spec_num = self._get_spec_number(workspace, kwargs)
            plot_as_dist, _ = get_plot_as_distribution(workspace, pop=False,
                                                       **kwargs)
            is_distribution = workspace.isDistribution() or plot_as_dist
            return self.track_workspace_artist(
                workspace, plotfunctions.plot(self, *args, **kwargs),
                is_distribution, _data_update, spec_num)
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

            def _data_update(artists, workspace):
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
                container_new = plotfunctions.errorbar(self, workspace, **kwargs)
                self.containers.insert(orig_idx, container_new)
                self.containers.pop()
                # update line properties to match original
                orig_flat, new_flat = cbook.flatten(container_orig), cbook.flatten(container_new)
                for artist_orig, artist_new in zip(orig_flat, new_flat):
                    artist_new.update_from(artist_orig)
                # ax.relim does not support collections...
                self._update_line_limits(container_new[0])
                self.autoscale()
                return container_new

            workspace = args[0]
            spec_num = self._get_spec_number(workspace, kwargs)
            plot_as_dist, _ = get_plot_as_distribution(workspace, pop=False,
                                                       **kwargs)
            return self.track_workspace_artist(
                workspace, plotfunctions.plot(self, *args, **kwargs),
                plot_as_dist, _data_update, spec_num)
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

            def _update_data(artists, workspace):
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
