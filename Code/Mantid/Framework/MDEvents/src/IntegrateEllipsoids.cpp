/*WIKI*

=== Overview ===

This algorithm will integrate disjoint single crystal Bragg peaks by summing the number
of raw events in a 3D ellipsoidal peak region in reciprocal space and subtracting an 
estimate of the background obtained from an ellipsoidal shell.  In some ways it is 
similar to the IntegratePeaksMD algorithm, and it would be useful to also see the
documentation for that algorithm.  In particular the size parameters to this algorithm
are also specified in inverse Angstroms and the background subtraction is done in
the same way for both the intensity and the estimated standard deviations. However, 
this algorithm differs from IntegratePeaksMD in several critical ways.

* This algorithm counts raw, un-weighted events while IntegratePeaksMD uses weighted events.
* This algorithm uses 3D ellipsoidal regions with aspect ratios that are adapted to the set of events that are near the peak center, while IntegratePeaksMD uses spherical regions.
* This algorithm includes an option to automatically choose the size of the ellipsoidal regions based on the statistics of the set of events near the peak.
* This algorithm only applies to peaks with integral HKL values and as currently implemented it cannot be used to integrate ellipsoidal regions at other locations in reciprocal space.

The algorithm calculates the three principal axes of the events near a peak, and 
uses the standard deviations in the directions of the principal axes to determine
the aspect ratio of ellipsoids used for the peak and background regions. 

=== Explanation of Inputs ===

* The event data to be integrated is obtained from an ordinary EventWorkspace with an X-axis in time-of-flight, as loaded from a NeXus event file.  This algorithm maps the events to reciprocal space.

* The peaks to be integrated are obtained from a PeaksWorkspace.  The peaks must be indexed, and any peaks indexed as (0,0,0) will be ignored.  The HKL values for valid peaks should all be integers, to make this check for unindexed peaks reliable.

* Only events that are near a peak are considered when constructing the ellipsoids.  The RegionRadius specifies the maximum distance from the peak center to an event in reciprocal space, for that event to used.  See the figure below.  Also, each event will be counted for at most one peak, the one with the nearest HKL value.  The RegionRadius should be specified to be just slightly larger than the expected peak region to avoid overlap with other peaks, and to avoid including excessive background.  As the size of the RegionRadius increases, the ellipsoids will become more spherical and less well adapted to the actual shape of the peak.

[[File:IntegrateEllipsoids.png]]

* If the SpecifySize option is selected, then the user MUST specify the PeakSize, BackgroundInnerSize and BackgroundOuterSize.  In this mode, the algorithm is similar to the IntegratePeaksMD algorithm.  As shown in the figure, these values determine the length of the major axis for the ellipsoidal peak region, and of the inner and outer ellipsoids bounding the background region.  The same major axis lengths are used for all peaks, but the lengths of the other two axes of the ellipsoids are adjusted based on the standard deviations of the events in those directions. If SpecifySize is false, then the major axis length for each peak will be set to include a range of plus or minus three times the standard deviation of the events in that direction.  That is, PeakSize is set to three times the standard deviation in the direction of the first principal axis.  Also, in this case the BackgroundInnerSize is set to the PeakSize and the BackgroundOuterSize is set so that the background ellipsoidal shell has the same volume as the peak ellipsoidal region.  If specified by the user, these parameters must be ordered correctly with: <math>0 < PeakSize \leq BackgroundInnerSize < BackgroundOuterSize \leq RegionRadius</math>

* The integrated intensities will be set in the specified OutputWorkspace.  If this is different from the input PeaksWorkspace, the input peaks workspace will be copied to the OutputWorkspace before setting the integrated intensities.

=== Detailed Algorithm Description ===

This algorithm will integrate a list of indexed single-crystal diffraction peaks from a 
PeaksWorkspace, using events from an EventWorkspace.  The indexed peaks are first 
used to determine a UB matrix.  The inverse of that UB matrix is then used to form
lists of events that are close to peaks in reciprocal space.  An event will be added
to the list of events for a peak provided that the fractional h,k,l value of that
event (obtained by applying UB-inverse to the Q-vector) is closer to the h,k,l of that
peak, than to the h,k,l of any other peak AND the Q-vector for that event is within 
the specified radius of the Q-vector for that peak. 

When the lists of events near the peaks have been built, the three principal axes of  
the set of events near each peak are found, and the standard deviations of the 
projections of the events on each of the three principal axes are calculated.  The
principal axes and standard deviations for the events around a peak in the directions
of the principal axes are used to determine an ellipsoidal region for the peak and an 
ellipsoidal shell region for the background. The number of events in the peak 
ellipsoid and background ellipsoidal shell are counted and used to determine the net
integrated intensity of the peak. 

The ellipsoidal regions used for the peak and background can be obtained in two ways.
First, the user may specify the size of the peak ellipsoid and the inner and outer 
size of the background ellipsoid.  If these are specified, the values will be used
for half the length of the major axis of an ellipsoid centered on the peak.  The 
major axis is in the direction of the principal axis for which the standard deviation
in that direction is largest.  The other two axes for the ellipsoid are in the 
direction of the other two principal axes and are scaled relative to the major axes 
in proportion to their standard deviations.  For example if the standard deviations 
in the direction of the other two princial axes are .8 and .7 times the standard 
deviation in the direction of the major axis, then the ellipse will extend only .8 
and .7 times as far in the direction of those axes, as in the direction of the major 
axis.  Overall, the user specified sizes for the PeakSize, BackgroundInnerSize and
BackgroundOuterSize are similar to the PeakRadius, BackgroundInnerRadius and
BackgrounOuterRadius for the IntegratePeaksMD algorithm.  The difference is that
the regions used in this algorithm are not spherical, but are ellipsoidal with axis
directions obtained from the principal axes of the events near a peak and the 
ellipsoid shape (relative axis lengths) is determined by the standard deviations in 
the directions of the principal axes.

Second, if the user does not specifiy the size of the peak and background ellipsoids, 
then the three axes of the peak ellipsoid are again set to the principal axes of the
set of nearby events but in this case their axis lengths are set to cover a range of
plus or minus three standard deviations in the axis directions.  In this case, the 
background ellipsoidal shell is chosen to have the same volume as the peak ellipsoid
and it's inner surface is the outer surface of the peak ellipsoid.  The outer surface
of the background ellipsoidal shell is an ellipsoidal surface with the same relative 
axis lengths as the inner surface. 

*WIKI*/

#include <iostream>
#include <fstream>
#include <boost/math/special_functions/round.hpp>
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidMDEvents/MDTransfFactory.h"
#include "MantidMDEvents/UnitsConversionHelper.h"
#include "MantidMDEvents/Integrate3DEvents.h"
#include "MantidMDEvents/IntegrateEllipsoids.h"


using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace MDEvents
{
  /** NOTE: This has been adapted from the SaveIsawQvector algorithm.
   */

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM( IntegrateEllipsoids )
  
  /// This only works for diffraction.
  const std::string ELASTIC("Elastic");

  /// Only convert to Q-vector.
  const std::string Q3D("Q3D");

  /// Q-vector is always three dimensional.
  const std::size_t DIMS(3);

  //----------------------------------------------------------------------
  /** Constructor
   */
  IntegrateEllipsoids::IntegrateEllipsoids()
  {
  }
    
  //---------------------------------------------------------------------
  /** Destructor
   */
  IntegrateEllipsoids::~IntegrateEllipsoids()
  {
  }
  

  //---------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string IntegrateEllipsoids::name() const 
  { 
    return "IntegrateEllipsoids";
  }
  
  /// Algorithm's version for identification. @see Algorithm::version
  int IntegrateEllipsoids::version() const 
  { 
    return 1;
  }
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string IntegrateEllipsoids::category() const 
  { 
    return "Crystal";
  }

  //---------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void IntegrateEllipsoids::initDocs()
  {
    std::string text("Integrate SCD Bragg peaks using 3D ellipsoids.");
    this->setWikiSummary(text);
    this->setOptionalMessage(text);
  }

  //---------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void IntegrateEllipsoids::init()
  {
    auto ws_valid = boost::make_shared<CompositeValidator>();
   
    ws_valid->add<InstrumentValidator>();
    // the validator which checks if the workspace has axis and time-of-flight units
    ws_valid->add<WorkspaceUnitValidator>("TOF");

    declareProperty(new WorkspaceProperty<EventWorkspace>( "InputWorkspace", "", Direction::Input,ws_valid),
                    "An input EventWorkspace with time-of-flight units along X-axis and defined instrument with defined sample");

    declareProperty(new WorkspaceProperty<PeaksWorkspace>( "PeaksWorkspace","",Direction::InOut), 
                    "Workspace with Peaks to be integrated. NOTE: The peaks MUST be indexed with integer HKL values.");

    boost::shared_ptr<BoundedValidator<double> > mustBePositive(new BoundedValidator<double>());
    mustBePositive->setLower(0.0);

    declareProperty("RegionRadius",.35, mustBePositive,
                    "Only events at most this distance from a peak will be considered when integrating");

    declareProperty( "SpecifySize", false, "If true, use the following for the major axis sizes, else use 3-sigma");

    declareProperty("PeakSize",.18, mustBePositive, 
                    "Half-length of major axis for peak ellipsoid");

    declareProperty("BackgroundInnerSize", .18, mustBePositive, 
                    "Half-length of major axis for inner ellipsoidal surface of background region");

    declareProperty("BackgroundOuterSize", .23, mustBePositive,
                    "Half-length of major axis for outer ellipsoidal surface of background region");

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output),
                   "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
                   "with the peaks' integrated intensities.");
  }

  //---------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void IntegrateEllipsoids::exec()
  {
    // get the input workspace
    EventWorkspace_sptr wksp = getProperty("InputWorkspace");

    // this only works for unweighted events
    if (wksp->getEventType() != API::TOF)
    {
      throw std::runtime_error("IntegrateEllipsoids only works for raw events");
    }
    // error out if there are not events
    if (wksp->getNumberEvents() <= 0)
    {
      throw std::runtime_error("IntegrateEllipsoids does not work for empty event lists");
    }

    PeaksWorkspace_sptr in_peak_ws;
    in_peak_ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
         AnalysisDataService::Instance().retrieve( getProperty("PeaksWorkspace")) );

    if (!in_peak_ws)
    {
      throw std::runtime_error("Could not read the peaks workspace");
    }

    Mantid::DataObjects::PeaksWorkspace_sptr peak_ws = getProperty("OutputWorkspace");
    if ( peak_ws != in_peak_ws )
    {
      peak_ws = in_peak_ws->clone();
    }
                                                   // get UBinv and the list of
                                                   // peak Q's for the integrator
    std::vector<Peak> & peaks = peak_ws->getPeaks();
    size_t n_peaks            = peak_ws->getNumberPeaks();
    size_t indexed_count      = 0;
    std::vector<V3D> peak_q_list;
    std::vector<V3D> hkl_vectors;
    for ( size_t i = 0; i < n_peaks; i++ )         // Note: we skip un-indexed peaks
    {
      V3D hkl( peaks[i].getH(), peaks[i].getK(), peaks[i].getL() );  
      if ( Geometry::IndexingUtils::ValidIndex( hkl, 1.0 ) )    // use tolerance == 1 to 
                                                                // just check for (0,0,0) 
      {
        peak_q_list.push_back( V3D( peaks[i].getQLabFrame() ) );
        V3D miller_ind( (double)boost::math::iround<double>(hkl[0]), 
                        (double)boost::math::iround<double>(hkl[1]),
                        (double)boost::math::iround<double>(hkl[2]) );
        hkl_vectors.push_back( V3D(miller_ind) );
        indexed_count++;
      }
    }

    if ( indexed_count < 3 )
    {
      throw std::runtime_error(
            "At least three linearly independent indexed peaks are needed.");
    }
                                             // Get UB using indexed peaks and
                                             // lab-Q vectors
    Matrix<double> UB(3,3,false);
    Geometry::IndexingUtils::Optimize_UB( UB, hkl_vectors, peak_q_list );
    Matrix<double> UBinv( UB );
    UBinv.Invert();
    UBinv *= (1.0/(2.0 * M_PI));

    double radius            = getProperty( "RegionRadius" );
    bool   specify_size      = getProperty( "SpecifySize" );
    double peak_radius       = getProperty( "PeakSize" );
    double back_inner_radius = getProperty( "BackgroundInnerSize" );
    double back_outer_radius = getProperty( "BackgroundOuterSize" );
    std::vector<double> PeakRadiusVector(n_peaks,peak_radius);
    std::vector<double> BackgroundInnerRadiusVector(n_peaks,back_inner_radius);
    std::vector<double> BackgroundOuterRadiusVector(n_peaks,back_outer_radius);
    if ( specify_size )
    {
      if ( back_outer_radius > radius )
        throw std::runtime_error("BackgroundOuterSize must be less than or equal to the RegionRadius");

      if ( back_inner_radius >= back_outer_radius )
        throw std::runtime_error("BackgroundInnerSize must be less BackgroundOuterSize");

      if ( peak_radius > back_inner_radius )
        throw std::runtime_error("PeakSize must be less than or equal to the BackgroundInnerSize");
    }
                    
                                                  // make the integrator
    Integrate3DEvents integrator( peak_q_list, UBinv, radius );

                                                  // get the events and add
                                                  // them to the inegrator
    // set up a descripter of where we are going
    this->initTargetWSDescr(wksp);

    // units conersion helper
    UnitsConversionHelper unitConv;
    unitConv.initialize(m_targWSDescr, "Momentum");

    // initialize the MD coordinates conversion class
    MDTransf_sptr q_converter = MDTransfFactory::Instance().create(m_targWSDescr.AlgID);
    q_converter->initialize(m_targWSDescr);

    // set up the progress bar
    const size_t numSpectra = wksp->getNumberHistograms();
    Progress prog(this, 0.5, 1.0, numSpectra);

    // loop through the eventlists
    std::vector<double> buffer(DIMS);

    std::vector<V3D>  event_qs;
    for (std::size_t i = 0; i < numSpectra; ++i)
    {
      // get a reference to the event list
      const EventList& events = wksp->getEventList(i);

      // check to see if the event list is empty
      if (events.empty())
      {
        prog.report();
        continue; // nothing to do
      }

      // update which pixel is being converted
      std::vector<coord_t>locCoord(DIMS, 0.);
      unitConv.updateConversion(i);
      q_converter->calcYDepCoordinates(locCoord, i);

      // loop over the events
      double signal(1.);  // ignorable garbage
      double errorSq(1.); // ignorable garbage
      const std::vector<TofEvent>& raw_events = events.getEvents();
      event_qs.clear();
      for (auto event = raw_events.begin(); event != raw_events.end(); ++event)
      {
        double val = unitConv.convertUnits( event->tof() );
        q_converter->calcMatrixCoord( val, locCoord, signal, errorSq );
        for ( size_t dim = 0; dim < DIMS; ++dim )
        {
          buffer[dim] = locCoord[dim];
        }
        V3D q_vec( buffer[0], buffer[1], buffer[2] );
        event_qs.push_back( q_vec );
      } // end of loop over events in list

      integrator.addEvents( event_qs );

      prog.report();
    } // end of loop over spectra

    double inti;
    double sigi;
    for ( size_t i = 0; i < n_peaks; i++ )
    {
      V3D hkl( peaks[i].getH(), peaks[i].getK(), peaks[i].getL() );
      if ( Geometry::IndexingUtils::ValidIndex( hkl, 1.0 ) ) 
      {
        V3D peak_q( peaks[i].getQLabFrame() );
        integrator.ellipseIntegrateEvents( peak_q, 
          specify_size, peak_radius, back_inner_radius, back_outer_radius,
          inti, sigi );
        peaks[i].setIntensity( inti );
        peaks[i].setSigmaIntensity( sigi );
      }
      else
      {
        peaks[i].setIntensity( 0.0 );
        peaks[i].setSigmaIntensity( 0.0 );
      }
  
    }

    // This flag is used by the PeaksWorkspace to evaluate whether it has been integrated.
    peak_ws->mutableRun().addProperty("PeaksIntegrated", 1, true);
    // These flags are specific to the algorithm.
    peak_ws->mutableRun().addProperty("PeakRadius", PeakRadiusVector, true);
    peak_ws->mutableRun().addProperty("BackgroundInnerRadius", BackgroundInnerRadiusVector, true);
    peak_ws->mutableRun().addProperty("BackgroundOuterRadius", BackgroundOuterRadiusVector, true);

    setProperty("OutputWorkspace", peak_ws);
  }


  /**
   * @brief IntegrateEllipsoids::initTargetWSDescr Initialize the output 
   *        information for the MD conversion framework.
   *
   * @param wksp The workspace to get information from.
   */
  void IntegrateEllipsoids::initTargetWSDescr(EventWorkspace_sptr wksp)
  {
    m_targWSDescr.setMinMax( std::vector<double>(3, -2000.),
                             std::vector<double>(3, 2000.));
    m_targWSDescr.buildFromMatrixWS(wksp, Q3D, ELASTIC);
    m_targWSDescr.setLorentsCorr(false);

    // generate the detectors table
    Mantid::API::Algorithm_sptr childAlg = createChildAlgorithm("PreprocessDetectorsToMD",0.,.5);
    childAlg->setProperty("InputWorkspace",wksp);
    childAlg->executeAsChildAlg();

    DataObjects::TableWorkspace_sptr table = childAlg->getProperty("OutputWorkspace");
    if(!table)
      throw(std::runtime_error("Can not retrieve results of \"PreprocessDetectorsToMD\""));
    else
      m_targWSDescr.m_PreprDetTable = table;
  }

} // namespace MDEvents
} // namespace Mantid
