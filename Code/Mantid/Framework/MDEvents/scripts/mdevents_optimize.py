"""Experimental script used to find the optimal parameters
when building and MDEventWorkspace, comparing
memory use, building time, and dense histogram binning times.

@author: Janik Zikovsky
"""

#This line has to be first for some reason.
#from enthought.mayavi import mlab

from pylab import *
import os
import sys
sys.path.append("/home/8oz/Code/Mantid/Code/Mantid/bin")
from MantidFramework import mtd
mtd.initialise()
from mantidsimple import *
import time
import numpy as np
import pickle

class Params:
    def __init__(self):
        self.SplitInto = 1
        self.MaxRecursionDepth = 12
        self.SplitThresholdBase = 1
        self.SplitThreshold = 1
        self.MakeTime = 0
        self.CoarseBinTime = 0
        self.MediumBinTime = 0
        self.FineBinTime = 0
        self.MemoryUsed = 0


results = []        


######################################################################

# Initial loading
print "Loading the source event nexus file..."
LoadEventNexus(Filename="/home/8oz/data/TOPAZ_2511_event.nxs",OutputWorkspace="topaz",SingleBankPixelsOnly="0",Precount="1")

SplitInto_list = [2,3,4,5,6,8,7,8,9,10,12,15,20,30]
SplitThresholdBase_list = [10, 20, 50, 100, 200, 500, 1000]

SplitInto_list = [4,5]
SplitThresholdBase_list = [10, 20]

SplitInto_list = [2,3,4,5,6,8,10,12,15]
SplitThresholdBase_list = [10, 20, 50, 100]

SplitInto_list = [1]
SplitThresholdBase_list = [10, 20, 50, 100]

for SplitInto in SplitInto_list:
    for SplitThresholdBase in SplitThresholdBase_list:
        par = Params()
        print
        print "######################################################################"
        print "######################################################################"
        # Split at 50 events per outgoing box
        SplitThreshold = SplitThresholdBase * SplitInto ** 3
        print "SplitInto:", SplitInto 
        print "SplitThresholdBase", SplitThresholdBase
        print "SplitThreshold:", SplitThreshold 
        print

        par.SplitInto = SplitInto
        par.SplitThresholdBase = SplitThresholdBase
        par.SplitThreshold = SplitThreshold

        
        if not mtd["mdew"] is None:
            DeleteWorkspace("mdew")
        
        CreateMDEventWorkspace(Dimensions="3",Extents="-6,6,-6,6,-6,6",Names="Qx,Qy,Qz",Units="Ang-1,Ang-1,Ang-1", 
                               SplitInto=str(SplitInto),SplitThreshold=str(SplitThreshold),
                               BinarySplit="1",
                               MaxRecursionDepth="30",OutputWorkspace="mdew")
        
        start = time.time()
        MakeDiffractionMDEventWorkspace(InputWorkspace="topaz",OutputWorkspace="mdew")
        print time.time()-start, " secs to make MDEW."
        par.MakeTime = time.time()-start

        aa = mtd["mdew"]
        a = aa._getHeldObject()
        par.MemoryUsed = a.getMemorySize()

        start = time.time()
        bin_str = "-6.0, 6.0, 100"
        alg = BinToMDHistoWorkspace(InputWorkspace="mdew", OutputWorkspace="mdhisto", DimX="Qx,%s" % bin_str, DimY="Qy,%s" % bin_str, DimZ="Qz,%s" % bin_str, DimT="NONE,0,10,1")
        print time.time()-start, " secs to bin medium: %s." % bin_str
        par.MediumBinTime = time.time()-start

        bin_str = "-1.0, 1.0, 200"
        alg = BinToMDHistoWorkspace(InputWorkspace="mdew", OutputWorkspace="mdhisto", DimX="Qx,%s" % bin_str, DimY="Qy,%s" % bin_str, DimZ="Qz,%s" % bin_str, DimT="NONE,0,10,1")
        print time.time()-start, " secs to bin fine, close up: %s." % bin_str
        par.FineBinTime = time.time()-start
    
        start = time.time()
        bin_str = "-6.0, 6.0, 20"
        alg = BinToMDHistoWorkspace(InputWorkspace="mdew", OutputWorkspace="mdhisto", DimX="Qx,%s" % bin_str, DimY="Qy,%s" % bin_str, DimZ="Qz,%s" % bin_str, DimT="NONE,0,10,1")
        print time.time()-start, " secs to bin coarse: %s." % bin_str
        par.CoarseBinTime = time.time()-start
        
        # Add to all the results
        results.append(par)

        # Save the results to file for later
        f = open("optimize_results.dat", 'w')
        pickle.dump(results, f)
        f.close()


# Load back the results
file_list = ["optimize_results_1.dat", "optimize_results_2.dat"]
file_list = ["optimize_results.dat"]
results = []
for filename in file_list:
    f = open(filename, 'r')
    these_results = pickle.load(f)
    results += these_results
    f.close()


def plot_results(results, x_field, y_field, other_field):
    """ Function to plot Y vs X of anything. It accesses the members of "results" to plot them.
    other_field is used to separate by another field, and make separate line plots for each"""
    others = set()
    for par in results:
        others.add( eval('par.%s' % other_field) )
    others = list(others)
    others.sort()
        
    figure()
    
    for other in others:
        data = []
        for par in results:   
            this_other = eval('par.%s' % other_field)
            if this_other == other: 
                x = eval('par.%s' % x_field)      
                y = eval('par.%s' % y_field)
                data.append( (x,y) )      
        data.sort()
        xs = [x for (x,y) in data]
        ys = [y for (x,y) in data]
        p = plot(xs,ys, marker='.', label="%s = %f" % (other_field, other))
        
    title("%s vs %s" % (y_field, x_field));
    xlabel(x_field)
    ylabel(y_field)
    legend(loc='best')
    savefig("%s_vs_%s.png" % (y_field, x_field));

plot_results(results, "SplitInto", "MakeTime", "SplitThreshold")
plot_results(results, "SplitInto", "MemoryUsed", "SplitThreshold")
plot_results(results, "SplitInto", "CoarseBinTime", "SplitThreshold")
plot_results(results, "SplitInto", "MediumBinTime", "SplitThreshold")
plot_results(results, "SplitInto", "FineBinTime", "SplitThreshold")

#plot_results(results, "SplitThresholdBase", "CoarseBinTime")
#plot_results(results, "SplitThresholdBase", "CoarseBinTime")
#plot_results(results, "SplitThresholdBase", "MediumBinTime")
#plot_results(results, "SplitThresholdBase", "FineBinTime")


show()

#
#bb = mtd["mdhisto"]
#b = bb._getHeldObject()
#
#data = b.getSignalDataVector()
#data = np.array(data)
#data = data.reshape( (xbins, ybins, zbins) )
#f = open("tempdata.dat")
#data.tofile(f)
#f.close()
#
#
#for i in xrange(4):
#    mlab.figure(i)
#    contour_val = 10**i
#    mlab.gcf().name = "Threshold = %f" % contour_val
#    mlab.contour3d(data, contours=[ contour_val  ])

