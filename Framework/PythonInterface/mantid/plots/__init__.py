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
from matplotlib import cbook
from matplotlib.axes import Axes
from matplotlib.container import Container
from matplotlib.projections import register_projection

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


def plot_decorator(func):
    def wrapper(self, *args, **kwargs):
        func_value = func(self, *args, **kwargs)
        # Saves saving it on array objects
        if mantid.plots.helperfunctions.validate_args(*args, **kwargs):
            # Fill out kwargs with the values of args
            for index, arg in enumerate(args):
                if index is 0:
                    kwargs["workspaces"] = args[0].name()
                if index is 1:
                    kwargs["spectrum_nums"] = args[1]
                if index is 2:
                    kwargs["wksp_indices"] = args[2]
                if index is 3:
                    kwargs["errors"] = args[3]
                if index is 4:
                    kwargs["overplot"] = arg[4]
                # ignore 5 as no need to save the fig object
                if index is 6:
                    kwargs["plot_kwargs"] = arg[6]
            if hasattr(self, "creation_args"):
                self.creation_args.append(kwargs)
            else:
                self.creation_args = [kwargs]
        return func_value
    return wrapper


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

    def track_workspace_artist(self, name, artist, replace_handler=None):
        """
        Add the given workspace name to the list of workspaces
        displayed on this Axes instance
        :param name: The name of the workspace
        :param artists: A single artist or list/tuple of length 1 containing the data for the workspace
        :param replace_handler: A function to call when the data is replaced to update
        the artist (optional)
        :returns: The artists variable as it was passed in.
        """
        if name:
            artist_info = self.tracked_workspaces.setdefault(name, [])
            if isinstance(artist, Iterable) and not isinstance(artist, Container):
                artist = artist[0]
            if replace_handler is None:
                def replace_handler(_, __):
                    logger.warning("Updating data on this plot type is not supported")
            artist_info.append((artist, replace_handler))

        return artist

    def remove_workspace_artists(self, name):
        """
        Remove the artists reference by this workspace (if any) and return True
        if the axes is then empty
        :param name: The name of the workspace
        :return: True if the axes is empty, false if artists remain or this workspace is not associated here
        """
        try:
            # pop to ensure we don't hold onto an artist reference
            artist_info = self.tracked_workspaces.pop(name)
        except KeyError:
            return False

        # delete the artists from the figure
        for artist, _ in artist_info:
            artist.remove()
            # Remove doesn't catch removing the container for errorbars etc
            if isinstance(artist, Container):
                try:
                    self.containers.remove(artist)
                except ValueError:
                    pass

        axes_empty = self.is_empty()
        if (not axes_empty) and self.legend_ is not None:
            self.legend()

        return axes_empty

    def replace_workspace_artists(self, name, workspace):
        """
        Replace the data of any artists relating to this workspace.
        The axes are NOT redrawn
        :param name: The name of the workspace being replaced
        :param workspace: The workspace containing the new data
        :return : True if data was replace, false otherwise
        """
        try:
            artist_info = self.tracked_workspaces[name]
        except KeyError:
            return False

        for artist, handler in artist_info:
            handler(artist, workspace)
        if self.legend_:
            self.legend()
        return True

    def is_empty(self):
        """
        Checks the known artist containers to see if anything exists within them
        :return: True if no artists exist, false otherwise
        """
        def _empty(container):
            return len(container) == 0
        return _empty(self.lines) and _empty(self.images) and _empty(self.collections)\
               and _empty(self.containers)

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

            def _data_update(line2d, workspace):
                x, y, _ = plotfunctions._plot_impl(self, workspace, kwargs)
                line2d.set_data(x, y)
                self.relim()
                self.autoscale()

            return self.track_workspace_artist(args[0].name(),
                                               plotfunctions.plot(self, *args, **kwargs),
                                               _data_update)
        else:
            return Axes.plot(self, *args, **kwargs)

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

            def _data_update(container_orig, workspace):
                # It is not possible to simply reset the error bars so
                # we just plot new lines
                container_orig.remove()
                self.containers.remove(container_orig)
                container_new = plotfunctions.errorbar(self, workspace, **kwargs)
                orig_flat, new_flat = cbook.flatten(container_orig), cbook.flatten(container_new)
                for artist_orig, artist_new in zip(orig_flat, new_flat):
                     artist_new.update_from(artist_orig)

                # ax.relim does not support collections...
                self._update_line_limits(container_new[0])
                self.autoscale()

            return self.track_workspace_artist(args[0].name(),
                                               plotfunctions.errorbar(self, *args, **kwargs),
                                               _data_update)
        else:
            return Axes.errorbar(self, *args, **kwargs)

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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions')

            def _update_data(artist, workspace):
                self._update_colorplot_data(plotfunctions.pcolor,
                                            artist, workspace, **kwargs)
            return self.track_workspace_artist(args[0].name(),
                                               plotfunctions.pcolor(self, *args, **kwargs),
                                               _update_data)
        else:
            return Axes.pcolor(self, *args, **kwargs)

    def pcolorfast(self, *args, **kwargs):
        """
        If the **mantid** projection is chosen, it can be
        used the same as :py:meth:`matplotlib.axes.Axes.pcolorfast` for arrays,
        or it can be used to plot :class:`mantid.api.MatrixWorkspace`
        or :class:`mantid.api.IMDHistoWorkspace`. You can have something like::

            import matplotlib.pyplot as plt
            from mantid import plots

            ...

            fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
            ax.pcolorfast(workspace) #for workspaces
            ax.pcolorfast(x,y,C)     #for arrays
            fig.show()

        For keywords related to workspaces, see :func:`plotfunctions.pcolorfast`
        """
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions')

            def _update_data(artist, workspace):
                self._update_colorplot_data(plotfunctions.pcolorfast,
                                            artist, workspace, **kwargs)

            return self.track_workspace_artist(args[0].name(),
                                               plotfunctions.pcolorfast(self, *args, **kwargs),
                                               _update_data)
        else:
            return Axes.pcolorfast(self, *args, **kwargs)

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
        if helperfunctions.validate_args(*args):
            logger.debug('using plotfunctions')

            def _update_data(artist, workspace):
                self._update_colorplot_data(plotfunctions.pcolormesh,
                                            artist, workspace, **kwargs)

            # def _update_data(artist, workspace):
            #     artist.remove()
            #     if hasattr(artist, 'colorbar_cid'):
            #         artist.callbacksSM.disconnect(artist.colorbar_cid)
            #     mesh = plotfunctions.pcolormesh(self, workspace, **kwargs)
            #     plotfunctions.update_colorplot_datalimits(self, mesh)
            #     if artist.colorbar is not None:
            #         self._attach_colorbar(mesh, artist.colorbar)

            return self.track_workspace_artist(args[0].name(),
                                               plotfunctions.pcolormesh(self, *args, **kwargs),
                                               _update_data)
        else:
            return Axes.pcolormesh(self, *args, **kwargs)

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

            def _update_data(artist, workspace):
                self._update_colorplot_data(plotfunctions.imshow,
                                            artist, workspace, **kwargs)

            return self.track_workspace_artist(args[0].name(),
                                               plotfunctions.imshow(self, *args, **kwargs),
                                               _update_data)
        else:
            return Axes.imshow(self, *args, **kwargs)

    def _update_colorplot_data(self, colorfunc, artist, workspace, **kwargs):
        artist.remove()
        if hasattr(artist, 'colorbar_cid'):
            artist.callbacksSM.disconnect(artist.colorbar_cid)
        im = colorfunc(self, workspace, **kwargs)
        plotfunctions.update_colorplot_datalimits(self, im)
        if artist.colorbar is not None:
            self._attach_colorbar(im, artist.colorbar)

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
            return self.track_workspace_artist(args[0].name(), plotfunctions.contour(self, *args, **kwargs))
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
            return self.track_workspace_artist(args[0].name(), plotfunctions.contourf(self, *args, **kwargs))
        else:
            return Axes.contourf(self, *args, **kwargs)

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
            return self.track_workspace_artist(args[0].name(), plotfunctions.tripcolor(self, *args, **kwargs))
        else:
            return Axes.tripcolor(self, *args, **kwargs)

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
            return self.track_workspace_artist(args[0].name(), plotfunctions.tricontour(self, *args, **kwargs))
        else:
            return Axes.tricontour(self, *args, **kwargs)

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
            return self.track_workspace_artist(args[0].name(), plotfunctions.tricontourf(self, *args, **kwargs))
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
