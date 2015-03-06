# File: EllipsoidIntegr.py
#
#  Integrates a run using the ellipsoid technique

import os
import sys
import shutil
import time

import stresstesting
import numpy


from mantid.api import *
#sys.path.append("/home/ruth/GIT_MantidBuild/bin/")
from mantid.simpleapi import *

class EllipsoidIntegr( stresstesting.MantidStressTest):
   
  def requiredMemoryMB(self):
       """ Require about 12GB free """
       return 2000
       
  def runTest(self):
                                          # expected results with size determined
                                           # automatically from projected event sigmas
    inti_auto = [ 88, 99, 23, 33, 8, 8, 4 ]
    sigi_auto = [ 13.784, 18.1384, 13.1529, 9.94987, 5.83095, 10.2956, 10.2956]
                                            # expected results with fixed size 
                                             # ellipsoids
    inti_fixed = [ 87.541, 95.3934, 21.3607, 33.4262, 7.36066, 9.68852, 3.54098 ]
    sigi_fixed = [ 13.9656, 18.4523, 13.4335, 10.1106, 5.94223, 10.5231, 10.5375 ]

                                              # first, load peaks into a peaks workspace
  
                                    
    peaks_file = "TOPAZ_3007.peaks"
    peaks_ws_name="TOPAZ_3007_peaks"
    LoadIsawPeaks( Filename=peaks_file,OutputWorkspace = peaks_ws_name)
    

                                               # next, load events into an event workspace
    event_file="TOPAZ_3007_bank_37_20_sec.nxs"
    event_ws_name="TOPAZ_3007_events"

    LoadNexus(Filename=event_file, OutputWorkspace=event_ws_name)
                                              # configure and test the algorithm
                                              # using automatically determined
                                              # ellipsoid sizes
    IntegrateEllipsoids(event_ws_name, peaks_ws_name,".25","0",".2",".2",".25",OutputWorkspace=peaks_ws_name)
    
    peaks_ws = mtd[peaks_ws_name]
    for  i   in range( 13, 20) :
    
      self.assertDelta( peaks_ws.getPeak(i).getIntensity(), inti_auto[i-13], 0.1 )
      self.assertDelta( peaks_ws.getPeak(i).getSigmaIntensity(), sigi_auto[i-13], 0.1 )
   
                                              # configure and test the algorithm
                                              # using fixed ellipsoid sizes
    peaks_ws=IntegrateEllipsoids( event_ws_name,peaks_ws_name,.25,1,.2,.2,.25,OutputWorkspace=peaks_ws_name)
    peaks_ws = mtd[peaks_ws_name]
   
    for  i in range( 13,20 ):
      self.assertDelta(peaks_ws.getPeak(i).getIntensity(), inti_fixed[i-13], 0.1 )
      self.assertDelta( peaks_ws.getPeak(i).getSigmaIntensity(), sigi_fixed[i-13], 0.1 )
    
  def validate(self): 
      return True
          
  def requiredFiles(self):
   
      return ["TOPAZ_3007_bank_37_20_sec.nxs","TOPAZ_3007.peaks"]