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

from mantid.plots._functions import *
from mantid.dataobjects import EventWorkspace,Workspace2D,MDHistoWorkspace

import matplotlib.pyplot as plt
from matplotlib.projections import register_projection
import matplotlib.colors as colors
import mantid
import numpy as np


def extract_data(ws):
    ws_data=[]
    ws_x=[]
    ws_y=[]
    nhist=ws.getNumberHistograms()
    yvals=ws.getAxis(1).extractValues()
    if len(yvals)==(nhist):
        yvals=boundaries_from_points(yvals)
    for index in range(nhist):
        x=ws.readX(index)
        intensity=ws.readY(index)
        error=ws.readE(index)
        if len(x)==len(intensity):
            x=boundaries_from_points(x)
        ws_data.append(intensity)
        ws_x.append(x)
        ws_y.append([yvals[index],yvals[index+1]])
    return(ws_data,ws_x,ws_y)

def mypcolormesh(ax,ws,**kwargs):
    I,x,y=extract_data(ws)
    mini=np.min([np.min(i) for i in I])
    maxi=np.max([np.max(i) for i in I])
    if 'vmin' in kwargs:
        mini=kwargs['vmin']
    if 'vmax' in kwargs:
        maxi=kwargs['vmax']
    if 'norm' not in kwargs:
        kwargs['norm']=colors.Normalize(vmin=mini, vmax=maxi)
    else:
        if kwargs['norm'].vmin==None:
            kwargs['norm'].vmin=mini
        if kwargs['norm'].vmax==None:
            kwargs['norm'].vmax=maxi

    for inti,xi,yi in zip(I,x,y):
        XX,YY=np.meshgrid(xi,yi,indexing='ij')
        cm=ax.pcolormesh(XX,YY,inti.reshape(-1,1),**kwargs)
    return cm
    
class MantidAxes(plt.Axes):
    name='mantid'
    def pcolormesh(self,*args,**kwargs):
        if len(args)==1 and (isinstance(args[0],EventWorkspace) or isinstance(args[0],Workspace2D)):
            return mypcolormesh(self,args[0],**kwargs)
        else:
            return plt.Axes.pcolormesh(self,*args,**kwargs)

register_projection(MantidAxes)



