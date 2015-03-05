# File: ReduceOneSCD_Run.py
#
# Version 2.0, modified to work with Mantid's new python interface.
#
# This script will reduce one SCD run.  The configuration  is set up in the 
# first few lines in the method runTest.  This script will load, find peaks,
# index and integrate either found or predicted peaks for the specified run.  
# Either sphere integration or the Mantid PeakIntegration algorithms are 
# currently supported, but it may be updated to support other integration 
# methods.  Users should make a directory to hold the output of this script, 
# and must specify that output directory with the other configuration 
#information.
#
#
import os
import sys
import shutil
import time

import stresstesting
import numpy


from mantid.api import *
#sys.path.append("/home/ruth/GIT_MantidBuild/bin/")
from mantid.simpleapi import *

class ReduceOneSCD_Run( stresstesting.MantidStressTest):

   
   def requiredMemoryMB(self):
       """ Require about 12GB free """
       return 6000
      
   def runTest(self):
      start_time = time.time()


      instrument_name           = "TOPAZ"
      calibration_file_1        = "TOPAZ_2011_02_16.DetCal"
      calibration_file_2        = None
 
      self.output_directory          =  config["defaultsave.directory"]
      
      min_tof                   = "400"
      max_tof                   = "16666"
      min_monitor_tof           = "1000"
      max_monitor_tof           = "12500"
      monitor_index             = "0"
      cell_type                 = "Orthorhombic"
      centering                 = "P"
      num_peaks_to_find         = "150"
      min_d                     = "4"
      max_d                     = "12"
      tolerance                 = ".12"
      integrate_predicted_peaks = False
      min_pred_wl               = ".25"
      max_pred_wl               = "3.5"
      min_pred_dspacing         = ".2"
      max_pred_dspacing         = "2.5"
      use_sphere_integration    = True
      use_fit_peaks_integration = False
      peak_radius               = ".2"
      bkg_inner_radius          = ".2"
      bkg_outer_radius          = ".25"
      integrate_if_edge_peak    = False
      rebin_step                = "-.004"
      preserve_events           = True
      use_ikeda_carpenter       = False
      n_bad_edge_pixels         = "10"

      rebin_params = min_tof+ ","+ rebin_step +"," +max_tof
      run                      = "3132"
      self.saved=False;
#
# Get the fully qualified input run file name, either from a specified data 
# directory or from findnexus
#
      
      full_name = instrument_name + "_" + (run) + "_event.nxs"
      
      print "\nProcessing File: " + full_name + " ......\n"

#
# Name the files to write for this run
#
      run_niggli_matrix_file = self.output_directory + "/" + run + "_Niggli.mat"
      run_niggli_integrate_file = self.output_directory + "/" + run + "_Niggli.integrate"
      

#
# Load the run data and find the total monitor counts
#
      event_ws = LoadEventNexus( Filename=full_name, 
                           FilterByTofMin=min_tof, FilterByTofMax=max_tof )

      if (calibration_file_1 is not None) or (calibration_file_2 is not None):
         LoadIsawDetCal( event_ws, 
                  Filename=calibration_file_1)  

      monitor_ws = LoadNexusMonitors( Filename=full_name )

      integrated_monitor_ws = Integration( InputWorkspace=monitor_ws, 
                                     RangeLower=min_monitor_tof, RangeUpper=max_monitor_tof, 
                                     StartWorkspaceIndex=monitor_index, EndWorkspaceIndex=monitor_index )

      monitor_count = integrated_monitor_ws.dataY(0)[0]
      print "\n", run, " has calculated monitor count", monitor_count, "\n"

#
# Make MD workspace using Lorentz correction, to find peaks 
#
      MDEW = ConvertToMD( InputWorkspace=event_ws, QDimensions="Q3D",
                    dEAnalysisMode="Elastic", QConversionScales="Q in A^-1",
    LorentzCorrection='1', MinValues="-50,-50,-50", MaxValues="50,50,50",
                    SplitInto='2', SplitThreshold='50',MaxRecursionDepth='11' )
#
# Find the requested number of peaks.  Once the peaks are found, we no longer
# need the weighted MD event workspace, so delete it.
#
      distance_threshold = 0.9 * 6.28 / float(max_d)
      peaks_ws = FindPeaksMD( MDEW, MaxPeaks=num_peaks_to_find, 
                        PeakDistanceThreshold=distance_threshold )

      AnalysisDataService.remove( MDEW.getName() )
#      SaveIsawPeaks( InputWorkspace=peaks_ws, AppendFile=False,
#               Filename='A'+run_niggli_integrate_file )
#
# Find a Niggli UB matrix that indexes the peaks in this run
#
      FindUBUsingFFT( PeaksWorkspace=peaks_ws, MinD=min_d, MaxD=max_d, Tolerance=tolerance )
      IndexPeaks( PeaksWorkspace=peaks_ws, Tolerance=tolerance )

#
# Save UB and peaks file, so if something goes wrong latter, we can at least 
# see these partial results
#
#      SaveIsawUB( InputWorkspace=peaks_ws,Filename=run_niggli_matrix_file )
#      SaveIsawPeaks( InputWorkspace=peaks_ws, AppendFile=False,
#               Filename=run_niggli_integrate_file )

#
# Get complete list of peaks to be integrated and load the UB matrix into
# the predicted peaks workspace, so that information can be used by the
# PeakIntegration algorithm.
#
      if integrate_predicted_peaks:
         print "PREDICTING peaks to integrate...."
         peaks_ws = PredictPeaks( InputWorkspace=peaks_ws,
                WavelengthMin=min_pred_wl, WavelengthMax=max_pred_wl,
                MinDSpacing=min_pred_dspacing, MaxDSpacing=max_pred_dspacing, 
                ReflectionCondition='Primitive' )
      else:
         print "Only integrating FOUND peaks ...."
#
# Set the monitor counts for all the peaks that will be integrated
#
      num_peaks = peaks_ws.getNumberPeaks()
      for i in range(num_peaks):
         peak = peaks_ws.getPeak(i)
         peak.setMonitorCount( monitor_count )
    
      if use_sphere_integration:
#
# Integrate found or predicted peaks in Q space using spheres, and save 
# integrated intensities, with Niggli indexing.  First get an un-weighted 
# workspace to do raw integration (we don't need high resolution or 
# LorentzCorrection to do the raw sphere integration )
#
         MDEW = ConvertToDiffractionMDWorkspace( InputWorkspace=event_ws,
                 LorentzCorrection='0', OutputDimensions='Q (lab frame)',
                 SplitInto='2', SplitThreshold='500', MaxRecursionDepth='5' )

         peaks_ws = IntegratePeaksMD( InputWorkspace=MDEW, PeakRadius=peak_radius,
	          BackgroundOuterRadius=bkg_outer_radius, 
                  BackgroundInnerRadius=bkg_inner_radius,
	          PeaksWorkspace=peaks_ws, 
                  IntegrateIfOnEdge=integrate_if_edge_peak )

      elif use_fit_peaks_integration:
         event_ws = Rebin( InputWorkspace=event_ws,
                    Params=rebin_params, PreserveEvents=preserve_events )
         peaks_ws = PeakIntegration( InPeaksWorkspace=peaks_ws, InputWorkspace=event_ws, 
                              IkedaCarpenterTOF=use_ikeda_carpenter,
                              MatchingRunNo=True,
                              NBadEdgePixels=n_bad_edge_pixels )
#
# Save the final integrated peaks, using the Niggli reduced cell.  
# This is the only file needed, for the driving script to get a combined
# result.(UNComment to get new values if algorithms change)
#
#      SaveIsawPeaks( InputWorkspace=peaks_ws, AppendFile=False, 
#               Filename=run_niggli_integrate_file )

#
# If requested, also switch to the specified conventional cell and save the
# corresponding matrix and integrate file
#
      if (not cell_type is None) and (not centering is None) :
         self.run_conventional_matrix_file = self.output_directory + "/" + run + "_" +    \
                                 cell_type + "_" + centering + ".mat"
         run_conventional_integrate_file = self.output_directory + "/" + run + "_" + \
                                    cell_type + "_" + centering + ".integrate"
         SelectCellOfType( PeaksWorkspace=peaks_ws, 
                    CellType=cell_type, Centering=centering, 
                    Apply=True, Tolerance=tolerance )
         # UNCOMMENT the line below to get new output values if an algorithm changes
         #SaveIsawPeaks( InputWorkspace=peaks_ws, AppendFile=False, Filename=run_conventional_integrate_file )
         SaveIsawUB( InputWorkspace=peaks_ws, Filename=self.run_conventional_matrix_file )
         self.saved = True
        
      end_time = time.time()
      
      CreateSingleValuedWorkspace(OutputWorkspace="XX1",DataValue="3")
      
      
      LoadIsawUB(InputWorkspace="XX1",Filename=self.run_conventional_matrix_file )      
      s1 = mtd["XX1"].sample()
      
      LoadIsawPeaks(OutputWorkspace="PeaksP", Filename="3132_Orthorhombic_P.integrate")
      LoadIsawUB(InputWorkspace=peaks_ws,Filename="3132_Orthorhombic_P.mat")
      IndexPeaks( PeaksWorkspace=peaks_ws, Tolerance=tolerance )
      CreateSingleValuedWorkspace(OutputWorkspace="XX2",DataValue="3")
      LoadIsawUB(InputWorkspace="XX2",Filename="3132_Orthorhombic_P.mat")
      
      s2 = mtd["XX2"].sample()
      ol = s1.getOrientedLattice()
      o2 = s2.getOrientedLattice()
      self.assertDelta( ol.a(), ol.a(), 0.01, "Correct lattice a value not found.")
      self.assertDelta( ol.b(), ol.b(), 0.01, "Correct lattice b value not found.")
      self.assertDelta( ol.c(), ol.c(), 0.01, "Correct lattice c value not found.")
      self.assertDelta( ol.alpha(), ol.alpha(), 0.4, "Correct lattice angle alpha value not found.")
      self.assertDelta( ol.beta(), ol.beta(), 0.4, "Correct lattice angle beta value not found.")
      self.assertDelta( ol.gamma(), ol.gamma(), 0.4, "Correct lattice angle gamma value not found.")
      
      self.__reduced_ws_name = str(peaks_ws)
      
      print '\nReduced run ' + str(run) + ' in ' + str(end_time - start_time) + ' sec'
      print ["output directory=",self.output_directory]

   def cleanup(self):
      if self.saved:
          import os
          os.remove( self.run_conventional_matrix_file)
          
   def validateMethod(self):
      return "ValidateWorkspaceToWorkspace"

   def validate(self):
      return [self.__reduced_ws_name,'PeaksP']
      
