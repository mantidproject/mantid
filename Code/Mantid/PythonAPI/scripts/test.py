setWorkingDirectory("C:/MantidInstall")

#execute a few algorithms
mtd.execute("LoadRaw","Data/MAR11060.RAW;test",-1)
mtd.execute("ConvertUnits","test;converted;dSpacing",-1)
mtd.execute("Rebin","converted;rebinned;0.1,0.001,5",-1)
#clear up intermediate workspaces
mtd.deleteWorkspace("test")
mtd.deleteWorkspace("converted")
#extract the one we want
w=mtd.getMatrixWorkspace('rebinned')
print w.getNumberHistograms()

x = w.readX(450)
y = w.readY(450)
#drop the first time bin to make sure the axes match
tx=x[1:len(x)]


x1 = w.readX(440)
y1 = w.readY(440)
#drop the first time bin to make sure the axes match
tx1=x[1:len(x)]


print('loading plotting library this can take a second or two')
from pylab import *
plot(tx,y)
plot(tx1,y1)
show()

#cleanup
x=0
y=0
tx=0
x1=0
y2=0
tx1=0
