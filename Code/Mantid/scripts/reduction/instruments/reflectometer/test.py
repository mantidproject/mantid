
from MantidFramework import *
from mantidsimple import *
from numpy import zeros
from pylab import *
import wks_utility


filename = '/mnt/hgfs/j35/results/REF_L_66420_event.nxs'
LoadEventNexus(Filename=filename, OutputWorkspace='wp1')
mt = mtd['wp1']

sample = mt.getInstrument().getSample()
source = mt.getInstrument().getSource()
detector = mt.getDetector(38912)

dDS_2d = zeros((256,304))
for x in range(304):
    for y in range(256):
        _index = 256*x+y
        detector = mt.getDetector(_index)
        dDS_2d[y,x] = sample.getDistance(detector)

fig1 = plt.figure(1,(6,6))
fig1.clf()

ax = fig1.add_subplot(2,2,1)
ax.imshow(dDS_2d, origin='lower', aspect='auto')
colorbar()

#I want to be able to change the axis label
ax = fig1.add_subplot(2,2,4)
ax.imshow(dDS_2d, origin='lower', aspect='auto')
colorbar()








show()
