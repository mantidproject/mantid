###########################################################################
def PlotWorkspace(name):
	w=mtd.getWorkspace(name)
	print ("histogram count =" + str(w.getNumberHistograms()))
	x = w.readX(0)
	y = w.readY(0)
	#drop the first time bin to make sure the axes match
	tx=x[1:len(x)]
	
	plot(tx,y)
	
	###########################################################################
def PlotAll():
	for name in MantidPythonAPI.getWorkspaceNames():
		PlotWorkspace(name)
	show()

###########################################################################
#Main program
###########################################################################
print ("Loading plotting library - this takes a bit of time")

from pylab import *		