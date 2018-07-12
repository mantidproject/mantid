#  This file is part of the mantid package
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""
Functionality for unpacking mantid objects for plotting with matplotlib.
"""

# This file should be left free of PyQt imports to allow quick importing
# of the main package.
from __future__ import (absolute_import, division, print_function)

import mantid.kernel
import mantid.plots.plotfunctions
import mantid.plots.plotfunctions3D
from matplotlib.axes import Axes
from matplotlib.projections import register_projection
from mpl_toolkits.mplot3d.axes3d import Axes3D


class MantidAxes(Axes):
    '''
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
    '''

    name = 'mantid'

    def plot(self, *args, **kwargs):
        '''
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

        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions.plot`.
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions')
            return mantid.plots.plotfunctions.plot(self, *args, **kwargs)
        else:
            return Axes.plot(self, *args, **kwargs)

    def scatter(self, *args, **kwargs):
        '''
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
        
        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions.scatter`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions')
            return mantid.plots.plotfunctions.scatter(self, *args, **kwargs)
        else:
            return Axes.scatter(self, *args, **kwargs)

    def errorbar(self, *args, **kwargs):
        '''
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
        
        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions.errorbar`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions')
            return mantid.plots.plotfunctions.errorbar(self, *args, **kwargs)
        else:
            return Axes.errorbar(self, *args, **kwargs)

    def pcolor(self, *args, **kwargs):
        '''
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
        
        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions.pcolor`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions')
            return mantid.plots.plotfunctions.pcolor(self, *args, **kwargs)
        else:
            return Axes.pcolor(self, *args, **kwargs)

    def pcolorfast(self, *args, **kwargs):
        '''
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
        
        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions.pcolorfast`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions')
            return mantid.plots.plotfunctions.pcolorfast(self, *args, **kwargs)
        else:
            return Axes.pcolorfast(self, *args, **kwargs)

    def pcolormesh(self, *args, **kwargs):
        '''
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
        
        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions.pcolormesh`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions')
            return mantid.plots.plotfunctions.pcolormesh(self, *args, **kwargs)
        else:
            return Axes.pcolormesh(self, *args, **kwargs)

    def contour(self, *args, **kwargs):
        '''
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
        
        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions.contour`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions')
            return mantid.plots.plotfunctions.contour(self, *args, **kwargs)
        else:
            return Axes.contour(self, *args, **kwargs)

    def contourf(self, *args, **kwargs):
        '''
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
        
        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions.contourf`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions')
            return mantid.plots.plotfunctions.contourf(self, *args, **kwargs)
        else:
            return Axes.contourf(self, *args, **kwargs)

    def tripcolor(self, *args, **kwargs):
        '''
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
        
        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions.tripcolor`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions')
            return mantid.plots.plotfunctions.tripcolor(self, *args, **kwargs)
        else:
            return Axes.tripcolor(self, *args, **kwargs)

    def tricontour(self, *args, **kwargs):
        '''
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
        
        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions.tricontour`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions')
            return mantid.plots.plotfunctions.tricontour(self, *args, **kwargs)
        else:
            return Axes.tricontour(self, *args, **kwargs)

    def tricontourf(self, *args, **kwargs):
        '''
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
        
        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions.tricontourf`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions')
            return mantid.plots.plotfunctions.tricontourf(self, *args, **kwargs)
        else:
            return Axes.tricontourf(self, *args, **kwargs)


class MantidAxes3D(Axes3D):
    '''
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
    '''

    name = 'mantid3d'

    def plot(self, *args, **kwargs):
        '''
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

        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions3D.plot3D`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions3D')
            return mantid.plots.plotfunctions3D.plot(self, *args, **kwargs)
        else:
            return Axes3D.plot(self, *args, **kwargs)

    def scatter(self, *args, **kwargs):
        '''
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

        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions3D.scatter`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions3D')
            return mantid.plots.plotfunctions3D.scatter(self, *args, **kwargs)
        else:
            return Axes3D.scatter(self, *args, **kwargs)

    def plot_wireframe(self, *args, **kwargs):
        '''
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

        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions3D.wireframe`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions3D')
            return mantid.plots.plotfunctions3D.plot_wireframe(self, *args, **kwargs)
        else:
            return Axes3D.plot_wireframe(self, *args, **kwargs)

    def plot_surface(self, *args, **kwargs):
        '''
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

        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions3D.plot_surface`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions3D')
            return mantid.plots.plotfunctions3D.plot_surface(self, *args, **kwargs)
        else:
            return Axes3D.plot_surface(self, *args, **kwargs)

    def contour(self, *args, **kwargs):
        '''
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

        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions3D.contour`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions3D')
            return mantid.plots.plotfunctions3D.contour(self, *args, **kwargs)
        else:
            return Axes3D.contour(self, *args, **kwargs)

    def contourf(self, *args, **kwargs):
        '''
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

        For keywords related to workspaces, see :func:`mantid.plots.plotfunctions3D.contourf`
        '''
        if mantid.plots.helperfunctions.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.plotfunctions3D')
            return mantid.plots.plotfunctions3D.contourf(self, *args, **kwargs)
        else:
            return Axes3D.contourf(self, *args, **kwargs)


register_projection(MantidAxes)
register_projection(MantidAxes3D)
