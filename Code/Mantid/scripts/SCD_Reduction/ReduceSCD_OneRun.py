#pylint: disable=invalid-name
# File: ReduceOneSCD_Run.py
#
# Version 2.0, modified to work with Mantid's new python interface.
#
# This script will reduce one SCD run.  The configuration file name and
# the run to be processed must be specified as the first two command line
# parameters.  This script is intended to be run in parallel using the
# ReduceSCD_Parallel.py script, after this script and configuration file has
# been tested to work properly for one run. This script will load, find peaks,
# index and integrate either found or predicted peaks for the specified run.
# Either sphere integration or the Mantid PeakIntegration algorithms are
# currently supported, but it may be updated to support other integration
# methods.  Users should make a directory to hold the output of this script,
# and must specify that output directory in the configuration file that
# provides the parameters to this script.
#
# NOTE: All of the parameters that the user must specify are listed with
# instructive comments in the sample configuration file: ReduceSCD.config.
#

#
# _v1: December 3rd 2013. Mads Joergensen
# This version now includes the posibility to use the 1D cylindrical integration method
# and the posibility to load a UB matrix which will be used for integration of the individual
# runs and to index the combined file (Code from Xiapoing).
#
# _v2: December 3rd 2013. Mads Joergensen
# Adds the posibility to optimize the loaded UB for each run for a better peak prediction
# It is also possible to find the common UB by using lattice parameters of the first
# run or the loaded matirix instead of the default FFT method
#
# _v3: December 5 2013. A. J. Schultz
# This version includes the Boolean parameter use_monitor_counts to allow
# the use of either monitor counts (True) or proton charge (False) for
# scaling.

import os
import sys
import time
import ReduceDictionary
sys.path.append("/opt/mantidnightly/bin")
#sys.path.append("/opt/Mantid/bin")

from mantid.simpleapi import *
from mantid.api import *

print "API Version"
print apiVersion()

start_time = time.time()

#
# Get the config file name and the run number to process from the command line
#
if len(sys.argv) < 3:
    print "You MUST give the config file name(s) and run number on the command line"
    exit(0)

config_files = sys.argv[1:-1]
run          = sys.argv[-1]

#
# Load the parameter names and values from the specified configuration file
# into a dictionary and set all the required parameters from the dictionary.
#
params_dictionary = ReduceDictionary.LoadDictionary( *config_files )

instrument_name           = params_dictionary[ "instrument_name" ]
calibration_file_1        = params_dictionary.get('calibration_file_1', None)
calibration_file_2        = params_dictionary.get('calibration_file_2', None)
data_directory            = params_dictionary[ "data_directory" ]
output_directory          = params_dictionary[ "output_directory" ]
output_nexus              = params_dictionary.get( "output_nexus", False)
min_tof                   = params_dictionary[ "min_tof" ]
max_tof                   = params_dictionary[ "max_tof" ]
use_monitor_counts        = params_dictionary[ "use_monitor_counts" ]
min_monitor_tof           = params_dictionary[ "min_monitor_tof" ]
max_monitor_tof           = params_dictionary[ "max_monitor_tof" ]
monitor_index             = params_dictionary[ "monitor_index" ]
cell_type                 = params_dictionary[ "cell_type" ]
centering                 = params_dictionary[ "centering" ]
allow_perm                = params_dictionary[ "allow_perm" ]
num_peaks_to_find         = params_dictionary[ "num_peaks_to_find" ]
min_d                     = params_dictionary[ "min_d" ]
max_d                     = params_dictionary[ "max_d" ]
max_Q                     = params_dictionary.get('max_Q', "50")
tolerance                 = params_dictionary[ "tolerance" ]
integrate_predicted_peaks = params_dictionary[ "integrate_predicted_peaks" ]
min_pred_wl               = params_dictionary[ "min_pred_wl" ]
max_pred_wl               = params_dictionary[ "max_pred_wl" ]
min_pred_dspacing         = params_dictionary[ "min_pred_dspacing" ]
max_pred_dspacing         = params_dictionary[ "max_pred_dspacing" ]

use_sphere_integration    = params_dictionary.get('use_sphere_integration', True)
use_ellipse_integration   = params_dictionary.get('use_ellipse_integration', False)
use_fit_peaks_integration = params_dictionary.get('use_fit_peaks_integration', False)
use_cylindrical_integration  = params_dictionary.get('use_cylindrical_integration', False)

peak_radius               = params_dictionary[ "peak_radius" ]
bkg_inner_radius          = params_dictionary[ "bkg_inner_radius" ]
bkg_outer_radius          = params_dictionary[ "bkg_outer_radius" ]
integrate_if_edge_peak    = params_dictionary[ "integrate_if_edge_peak" ]

rebin_step                = params_dictionary[ "rebin_step" ]
preserve_events           = params_dictionary[ "preserve_events" ]
use_ikeda_carpenter       = params_dictionary[ "use_ikeda_carpenter" ]
n_bad_edge_pixels         = params_dictionary[ "n_bad_edge_pixels" ]

rebin_params = min_tof + "," + rebin_step + "," + max_tof

ellipse_region_radius     = params_dictionary[ "ellipse_region_radius" ]
ellipse_size_specified    = params_dictionary[ "ellipse_size_specified" ]

cylinder_radius           = params_dictionary[ "cylinder_radius" ]
cylinder_length           = params_dictionary[ "cylinder_length" ]

read_UB                   = params_dictionary[ "read_UB" ]
UB_filename               = params_dictionary[ "UB_filename" ]
optimize_UB               = params_dictionary[ "optimize_UB" ]


#
# Get the fully qualified input run file name, either from a specified data
# directory or from findnexus
#
short_filename = "%s_%s_event.nxs" % (instrument_name, str(run))
if data_directory is not None:
    full_name = data_directory + "/" + short_filename
else:
    candidates = FileFinder.findRuns(short_filename)
    full_name = ""
    for item in candidates:
        if os.path.exists(item):
            full_name = str(item)

    if not full_name.endswith('nxs'):
        print "Exiting since the data_directory was not specified and"
        print "findnexus failed for event NeXus file: " + instrument_name + " " + str(run)
        exit(0)

print "\nProcessing File: " + full_name + " ......\n"

#
# Name the files to write for this run
#
run_niggli_matrix_file = output_directory + "/" + run + "_Niggli.mat"
if output_nexus:
    run_niggli_integrate_file = output_directory + "/" + run + "_Niggli.nxs"
else:
    run_niggli_integrate_file = output_directory + "/" + run + "_Niggli.integrate"

#
# Load the run data and find the total monitor counts
#
event_ws = LoadEventNexus( Filename=full_name,
                           FilterByTofMin=min_tof, FilterByTofMax=max_tof )

#
# Load calibration file(s) if specified.  NOTE: The file name passed in to LoadIsawDetCal
# can not be None.  TOPAZ has one calibration file, but SNAP may have two.
#
if (calibration_file_1 is not None ) or (calibration_file_2 is not None):
    if calibration_file_1 is None :
        calibration_file_1 = ""
    if calibration_file_2 is None :
        calibration_file_2 = ""
    LoadIsawDetCal( event_ws,\
                  Filename=calibration_file_1, Filename2=calibration_file_2 )

monitor_ws = LoadNexusMonitors( Filename=full_name )
proton_charge = monitor_ws.getRun().getProtonCharge() * 1000.0  # get proton charge
print "\n", run, " has integrated proton charge x 1000 of", proton_charge, "\n"

integrated_monitor_ws = Integration( InputWorkspace=monitor_ws,
                                     RangeLower=min_monitor_tof, RangeUpper=max_monitor_tof,
                                     StartWorkspaceIndex=monitor_index, EndWorkspaceIndex=monitor_index )

monitor_count = integrated_monitor_ws.dataY(0)[0]
print "\n", run, " has integrated monitor count", monitor_count, "\n"

minVals= "-"+max_Q +",-"+max_Q +",-"+max_Q
maxVals = max_Q +","+max_Q +","+ max_Q
#
# Make MD workspace using Lorentz correction, to find peaks
#
MDEW = ConvertToMD( InputWorkspace=event_ws, QDimensions="Q3D",
                    dEAnalysisMode="Elastic", QConversionScales="Q in A^-1",\
                   LorentzCorrection='1', MinValues=minVals, MaxValues=maxVals,
                    SplitInto='2', SplitThreshold='50',MaxRecursionDepth='11' )
#
# Find the requested number of peaks.  Once the peaks are found, we no longer
# need the weighted MD event workspace, so delete it.
#
distance_threshold = 0.9 * 6.28 / float(max_d)
peaks_ws = FindPeaksMD( MDEW, MaxPeaks=num_peaks_to_find,
                        PeakDistanceThreshold=distance_threshold )
AnalysisDataService.remove( MDEW.getName() )

# Read or find UB for the run
if read_UB:
  # Read orientation matrix from file
    LoadIsawUB(InputWorkspace=peaks_ws, Filename=UB_filename)
    if optimize_UB:
    # Optimize the specifiec UB for better peak prediction
        uc_a = peaks_ws.sample().getOrientedLattice().a()
        uc_b = peaks_ws.sample().getOrientedLattice().b()
        uc_c = peaks_ws.sample().getOrientedLattice().c()
        uc_alpha = peaks_ws.sample().getOrientedLattice().alpha()
        uc_beta = peaks_ws.sample().getOrientedLattice().beta()
        uc_gamma = peaks_ws.sample().getOrientedLattice().gamma()
        FindUBUsingLatticeParameters(PeaksWorkspace= peaks_ws,a=uc_a,b=uc_b,c=uc_c,alpha=uc_alpha,beta=uc_beta,
                                     gamma=uc_gamma,NumInitial=num_peaks_to_find,Tolerance=tolerance)
else:
  # Find a Niggli UB matrix that indexes the peaks in this run
    FindUBUsingFFT( PeaksWorkspace=peaks_ws, MinD=min_d, MaxD=max_d, Tolerance=tolerance )

IndexPeaks( PeaksWorkspace=peaks_ws, Tolerance=tolerance)


#
# Save UB and peaks file, so if something goes wrong latter, we can at least
# see these partial results
#
SaveIsawUB( InputWorkspace=peaks_ws,Filename=run_niggli_matrix_file )
if output_nexus:
    SaveNexus( InputWorkspace=peaks_ws, Filename=run_niggli_integrate_file )
else:
    SaveIsawPeaks(InputWorkspace=peaks_ws, AppendFile=False,
                  Filename=run_niggli_integrate_file )

#
# Get complete list of peaks to be integrated and load the UB matrix into
# the predicted peaks workspace, so that information can be used by the
# PeakIntegration algorithm.
#
if integrate_predicted_peaks:
    print "PREDICTING peaks to integrate...."
    peaks_ws = PredictPeaks( InputWorkspace=peaks_ws,\
                WavelengthMin=min_pred_wl, WavelengthMax=max_pred_wl,\
                MinDSpacing=min_pred_dspacing, MaxDSpacing=max_pred_dspacing,\
                ReflectionCondition='Primitive' )
else:
    print "Only integrating FOUND peaks ...."
#
# Set the monitor counts for all the peaks that will be integrated
#
num_peaks = peaks_ws.getNumberPeaks()
for i in range(num_peaks):
    peak = peaks_ws.getPeak(i)
    if use_monitor_counts:
        peak.setMonitorCount( monitor_count )
    else:
        peak.setMonitorCount( proton_charge )
if use_monitor_counts:
    print '\n*** Beam monitor counts used for scaling.'
else:
    print '\n*** Proton charge x 1000 used for scaling.\n'

if use_sphere_integration:
#
# Integrate found or predicted peaks in Q space using spheres, and save
# integrated intensities, with Niggli indexing.  First get an un-weighted
# workspace to do raw integration (we don't need high resolution or
# LorentzCorrection to do the raw sphere integration )
#
    MDEW = ConvertToMD( InputWorkspace=event_ws, QDimensions="Q3D",\
                    dEAnalysisMode="Elastic", QConversionScales="Q in A^-1",\
                    LorentzCorrection='0', MinValues=minVals, MaxValues=maxVals,\
                    SplitInto='2', SplitThreshold='500',MaxRecursionDepth='10' )

    peaks_ws = IntegratePeaksMD( InputWorkspace=MDEW, PeakRadius=peak_radius,\
                  CoordinatesToUse="Q (sample frame)",\
              BackgroundOuterRadius=bkg_outer_radius,\
                  BackgroundInnerRadius=bkg_inner_radius,\
              PeaksWorkspace=peaks_ws,\
                  IntegrateIfOnEdge=integrate_if_edge_peak )
elif use_cylindrical_integration:
#
# Integrate found or predicted peaks in Q space using spheres, and save
# integrated intensities, with Niggli indexing.  First get an un-weighted
# workspace to do raw integration (we don't need high resolution or
# LorentzCorrection to do the raw sphere integration )
#
    MDEW = ConvertToMD( InputWorkspace=event_ws, QDimensions="Q3D",\
                    dEAnalysisMode="Elastic", QConversionScales="Q in A^-1",\
                    LorentzCorrection='0', MinValues=minVals, MaxValues=maxVals,\
                    SplitInto='2', SplitThreshold='500',MaxRecursionDepth='10' )

    peaks_ws = IntegratePeaksMD( InputWorkspace=MDEW, PeakRadius=peak_radius,\
                  CoordinatesToUse="Q (sample frame)",\
                  BackgroundOuterRadius=bkg_outer_radius,\
                  BackgroundInnerRadius=bkg_inner_radius,\
                  PeaksWorkspace=peaks_ws,\
                  IntegrateIfOnEdge=integrate_if_edge_peak,\
                  Cylinder=use_cylindrical_integration,CylinderLength=cylinder_length,\
                  PercentBackground=cylinder_percent_bkg,\
                  IntegrationOption=cylinder_int_option,\
                  ProfileFunction=cylinder_profile_fit)

elif use_fit_peaks_integration:
    event_ws = Rebin( InputWorkspace=event_ws,\
                    Params=rebin_params, PreserveEvents=preserve_events )
    peaks_ws = PeakIntegration( InPeaksWorkspace=peaks_ws, InputWorkspace=event_ws,\
                              IkedaCarpenterTOF=use_ikeda_carpenter,\
                              MatchingRunNo=True,\
                              NBadEdgePixels=n_bad_edge_pixels )

elif use_ellipse_integration:
    peaks_ws= IntegrateEllipsoids( InputWorkspace=event_ws, PeaksWorkspace = peaks_ws,\
                                 RegionRadius = ellipse_region_radius,\
                                 SpecifySize = ellipse_size_specified,\
                                 PeakSize = peak_radius,\
                                 BackgroundOuterSize = bkg_outer_radius,\
                                 BackgroundInnerSize = bkg_inner_radius )

elif use_cylindrical_integration:
    profiles_filename = output_directory + "/" + instrument_name + '_' + run + '.profiles'
    MDEW = ConvertToMD( InputWorkspace=event_ws, QDimensions="Q3D",\
                    dEAnalysisMode="Elastic", QConversionScales="Q in A^-1",\
                    LorentzCorrection='0', MinValues=minVals, MaxValues=maxVals,\
                    SplitInto='2', SplitThreshold='500',MaxRecursionDepth='10' )

    peaks_ws = IntegratePeaksMD( InputWorkspace=MDEW, PeakRadius=cylinder_radius,\
                  CoordinatesToUse="Q (sample frame)",\
                  Cylinder='1', CylinderLength = cylinder_length,\
                  PercentBackground = '20', ProfileFunction = 'NoFit',\
                  ProfilesFile = profiles_filename,\
              PeaksWorkspace=peaks_ws,\
                  )

#
# Save the final integrated peaks, using the Niggli reduced cell.
# This is the only file needed, for the driving script to get a combined
# result.
#
if output_nexus:
    SaveNexus( InputWorkspace=peaks_ws, Filename=run_niggli_integrate_file )
else:
    SaveIsawPeaks(InputWorkspace=peaks_ws, AppendFile=False,
                  Filename=run_niggli_integrate_file )

# Print warning if user is trying to integrate using the cylindrical method and transorm the cell
if use_cylindrical_integration:
    if (not cell_type is None) or (not centering is None):
        print "WARNING: Cylindrical profiles are NOT transformed!!!"
#
# If requested, also switch to the specified conventional cell and save the
# corresponding matrix and integrate file
#
else:
    if (not cell_type is None) and (not centering is None) :
        run_conventional_matrix_file = output_directory + "/" + run + "_" +        \
                                   cell_type + "_" + centering + ".mat"
        if output_nexus:
            run_conventional_integrate_file = output_directory + "/" + run + "_" + \
                                      cell_type + "_" + centering + ".nxs"
        else:
            run_conventional_integrate_file = output_directory + "/" + run + "_" + \
                                      cell_type + "_" + centering + ".integrate"
        SelectCellOfType( PeaksWorkspace=peaks_ws,\
                      CellType=cell_type, Centering=centering,\
                      AllowPermutations=allow_perm,\
                      Apply=True, Tolerance=tolerance )
    if output_nexus:
        SaveNexus(InputWorkspace=peaks_ws, Filename=run_conventional_integrate_file )
    else:
        SaveIsawPeaks(InputWorkspace=peaks_ws, AppendFile=False,\
                      Filename=run_conventional_integrate_file )
        SaveIsawUB(InputWorkspace=peaks_ws, Filename=run_conventional_matrix_file )

end_time = time.time()
print '\nReduced run ' + str(run) + ' in ' + str(end_time - start_time) + ' sec'
print 'using config file(s) ' + ", ".join(config_files)

#
# Try to get this to terminate when run by ReduceSCD_Parallel.py, from NX session
#
sys.exit(0)

