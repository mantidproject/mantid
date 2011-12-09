
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

print 'distance sample-source:'
print sample.getDistance(source)
print
print 'distance detector(#38912) - source'
detector = mt.getDetector(38912)
print detector.getDistance(source)

dDS_2d = zeros((256,304))
for x in range(304):
    for y in range(256):
        _index = 256*x+y
        detector = mt.getDetector(_index)
        dDS_2d[y,x] = sample.getDistance(detector)

plt.imshow(dDS_2d)
colorbar()
show()
