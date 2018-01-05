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

import mantid.plots.pfunctions
from mantid.dataobjects import EventWorkspace,Workspace2D,MDHistoWorkspace
import matplotlib.pyplot as plt
from matplotlib.projections import register_projection
import mantid.kernel

class MantidAxes(plt.Axes):

    name='mantid'

    def validate_args(self,*args):
        return len(args)>0 and (isinstance(args[0],EventWorkspace) or
                                isinstance(args[0],Workspace2D) or
                                isinstance(args[0],MDHistoWorkspace))

    def plot(self,*args,**kwargs):
        '''
        mantid plot
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.pfunctions')     
            return mantid.plots.pfunctions.plot(self,args[0],**kwargs)
        else:
            return plt.Axes.plot(self,*args,**kwargs)

    def scatter(self,*args,**kwargs):
        '''
        mantid scatter
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.pfunctions')     
            return mantid.plots.pfunctions.scatter(self,args[0],**kwargs)
        else:
            return plt.Axes.scatter(self,*args,**kwargs)

    def errorbar(self,*args,**kwargs):
        '''
        mantid errorbar
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.pfunctions')     
            return mantid.plots.pfunctions.errorbar(self,args[0],**kwargs)
        else:
            return plt.Axes.errorbar(self,*args,**kwargs)

    def pcolor(self,*args,**kwargs):
        '''
        mantid pcolor
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.pfunctions')     
            return mantid.plots.pfunctions.pcolor(self,args[0],**kwargs)
        else:
            return plt.Axes.pcolor(self,*args,**kwargs)

    def pcolorfast(self,*args,**kwargs):
        '''
        mantid pcolorfast
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.pfunctions')     
            return mantid.plots.pfunctions.pcolorfast(self,args[0],**kwargs)
        else:
            return plt.Axes.pcolorfast(self,*args,**kwargs)

    def pcolormesh(self,*args,**kwargs):
        '''
        mantid pcolormesh
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.pfunctions')     
            return mantid.plots.pfunctions.pcolormesh(self,args[0],**kwargs)
        else:
            return plt.Axes.pcolormesh(self,*args,**kwargs)

    def contour(self,*args,**kwargs):
        '''
        mantid contour
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.pfunctions')     
            return mantid.plots.pfunctions.contour(self,args[0],**kwargs)
        else:
            return plt.Axes.contour(self,*args,**kwargs)

    def contourf(self,*args,**kwargs):
        '''
        mantid contourf
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.pfunctions')     
            return mantid.plots.pfunctions.contourf(self,args[0],**kwargs)
        else:
            return plt.Axes.contourf(self,*args,**kwargs)

    def tripcolor(self,*args,**kwargs):
        '''
        mantid tripcolor
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.pfunctions')     
            return mantid.plots.pfunctions.tripcolor(self,args[0],**kwargs)
        else:
            return plt.Axes.tripcolor(self,*args,**kwargs)

    def tricontour(self,*args,**kwargs):
        '''
        mantid tricontour
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.pfunctions')     
            return mantid.plots.pfunctions.tricontour(self,args[0],**kwargs)
        else:
            return plt.Axes.tricontour(self,*args,**kwargs)

    def tricontourf(self,*args,**kwargs):
        '''
        mantid tricontourf
        '''
        if self.validate_args(*args):
            mantid.kernel.logger.debug('using mantid.plots.pfunctions')     
            return mantid.plots.pfunctions.tricontourf(self,args[0],**kwargs)
        else:
            return plt.Axes.tricontourf(self,*args,**kwargs)


register_projection(MantidAxes)



