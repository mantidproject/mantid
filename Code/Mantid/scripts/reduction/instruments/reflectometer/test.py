from MantidFramework import *
from mantidsimple import *
from numpy import zeros
from pylab import *
import wks_utility

#I'm testing the divide algorithm
x_axis = [0,1,2,3,4]
y_axis = [10,20,30,40,50,110,120,130,140,150]
CreateWorkspace('wp1', DataX=x_axis, DataY=y_axis, Nspec=2)

x_axis = [0,1,2,3,4]
y_axis = [2,2,2,2,2,4,4,4,4,4]
CreateWorkspace('wp2', DataX=x_axis, DataY=y_axis, Nspec=2)

#divide
Divide('wp1','wp2','wp3')
mt3 = mtd['wp3']
print mt3.readX(0)[:]
print mt3.readY(0)[:]
print mt3.readY(1)[:]
