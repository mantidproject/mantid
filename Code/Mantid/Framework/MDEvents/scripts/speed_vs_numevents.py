"""Experimental script used to find the optimal parameters
when building and MDEventWorkspace, comparing
memory use, building time, and dense histogram binning times.

This script compares the speed of operations vs the number of events in the MDEventWorkspace.

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
        self.NumberEvents = 0
        self.MaxRecursionDepth = 10
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

if not mtd["mdew"] is None:
    DeleteWorkspace("mdew")

# Create the MDEW once
CreateMDEventWorkspace(Dimensions="3",Extents="-6,6,-6,6,-6,6",Names="Qx,Qy,Qz",Units="Ang-1,Ang-1,Ang-1", 
                       SplitInto="5",SplitThreshold="1500",
                       BinarySplit="0", MaxRecursionDepth="10", OutputWorkspace="mdew")

for numTimes in xrange(1,11):
        par = Params()
        
        print
        print "######################################################################"
        print "######################################################################"
        print "numTimes:", numTimes 
        print
        
        start = time.time()
        MakeDiffractionMDEventWorkspace(InputWorkspace="topaz",OutputWorkspace="mdew")
        print time.time()-start, " secs to make MDEW."
        par.MakeTime = time.time()-start

        aa = mtd["mdew"]
        a = aa._getHeldObject()
        par.MemoryUsed = a.getMemorySize()
        par.NumberEvents = a.getNPoints()

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

import analysis

analysis.do_analysis( ["optimize_results.dat"], 2)
