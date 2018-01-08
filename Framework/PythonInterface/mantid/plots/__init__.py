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

import mantid.plots.functions
from mantid.dataobjects import EventWorkspace,Workspace2D,MDHistoWorkspace
import matplotlib.pyplot as plt
from matplotlib.projections import register_projection
import mantid.kernel

class MantidAxes(plt.Axes):
    '''
    This class defines the **mantid** projection. One chooses
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

    name='mantid'

    def validate_args(self,*args):
        return len(args)>0 and (isinstance(args[0],EventWorkspace) or
                                isinstance(args[0],Workspace2D) or
                                isinstance(args[0],MDHistoWorkspace))

    def plot(self,*args,**kwargs):
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

        For keywords related to workspaces, see :func:`mantid.plots.functions.plot`.
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.functions')     
            return mantid.plots.functions.plot(self,args[0],**kwargs)
        else:
            return plt.Axes.plot(self,*args,**kwargs)

    def scatter(self,*args,**kwargs):
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
        
        For keywords related to workspaces, see :func:`mantid.plots.functions.scatter`             
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.functions')     
            return mantid.plots.functions.scatter(self,args[0],**kwargs)
        else:
            return plt.Axes.scatter(self,*args,**kwargs)

    def errorbar(self,*args,**kwargs):
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
        
        For keywords related to workspaces, see :func:`mantid.plots.functions.errorbar`             
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.functions')     
            return mantid.plots.functions.errorbar(self,args[0],**kwargs)
        else:
            return plt.Axes.errorbar(self,*args,**kwargs)

    def pcolor(self,*args,**kwargs):
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
        
        For keywords related to workspaces, see :func:`mantid.plots.functions.pcolor`             
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.functions')     
            return mantid.plots.functions.pcolor(self,args[0],**kwargs)
        else:
            return plt.Axes.pcolor(self,*args,**kwargs)

    def pcolorfast(self,*args,**kwargs):
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
        
        For keywords related to workspaces, see :func:`mantid.plots.functions.pcolorfast`             
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.functions')     
            return mantid.plots.functions.pcolorfast(self,args[0],**kwargs)
        else:
            return plt.Axes.pcolorfast(self,*args,**kwargs)

    def pcolormesh(self,*args,**kwargs):
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
        
        For keywords related to workspaces, see :func:`mantid.plots.functions.pcolormesh`             
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.functions')     
            return mantid.plots.functions.pcolormesh(self,args[0],**kwargs)
        else:
            return plt.Axes.pcolormesh(self,*args,**kwargs)

    def contour(self,*args,**kwargs):
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
        
        For keywords related to workspaces, see :func:`mantid.plots.functions.contour`             
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.functions')     
            return mantid.plots.functions.contour(self,args[0],**kwargs)
        else:
            return plt.Axes.contour(self,*args,**kwargs)

    def contourf(self,*args,**kwargs):
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
        
        For keywords related to workspaces, see :func:`mantid.plots.functions.contourf`             
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.functions')     
            return mantid.plots.functions.contourf(self,args[0],**kwargs)
        else:
            return plt.Axes.contourf(self,*args,**kwargs)

    def tripcolor(self,*args,**kwargs):
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
        
        For keywords related to workspaces, see :func:`mantid.plots.functions.tripcolor`             
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.functions')     
            return mantid.plots.functions.tripcolor(self,args[0],**kwargs)
        else:
            return plt.Axes.tripcolor(self,*args,**kwargs)

    def tricontour(self,*args,**kwargs):
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
        
        For keywords related to workspaces, see :func:`mantid.plots.functions.tricontour`             
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.functions')     
            return mantid.plots.functions.tricontour(self,args[0],**kwargs)
        else:
            return plt.Axes.tricontour(self,*args,**kwargs)

    def tricontourf(self,*args,**kwargs):
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
        
        For keywords related to workspaces, see :func:`mantid.plots.functions.tricontourf`             
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.functions')     
            return mantid.plots.functions.tricontourf(self,args[0],**kwargs)
        else:
            return plt.Axes.tricontourf(self,*args,**kwargs)


register_projection(MantidAxes)



