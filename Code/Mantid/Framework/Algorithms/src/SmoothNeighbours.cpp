/*WIKI* 

This algorithm performs a moving-average smoothing of data by summing spectra of nearest neighbours over the face of detectors.
The output workspace has the same number of spectra as the input workspace.
This works on both [[EventWorkspace]]s and [[Workspace2D]]'s.
It has two main modes of operation.

=== Processing Either Generically or Assuming Rectangular Detectors ===

You may either specify properties for the Rectangular Detector Group, or the Non-uniform Detector Group, but not both. If you provide inputs for the Rectangular Detector group,
then the algorithm execution will assume that this is your desired processing route.

=== For All Instruments ===

Going through the input workspace pixel-by-pixel, Mantid finds the nearest-neighbours with the given Radius of each
pixel. The spectra are then summed together, and normalizing to unity (see the weighting section below).

=== For Instruments With Rectangular Detectors ===

The algorithm looks through the [[Instrument]] to find all the [[RectangularDetector]]s defined.
For each pixel in each detector, the AdjX*AdjY neighboring spectra are summed together and saved in the output workspace.

=== WeightedSum parameter ===

A weighting strategy can be applied to control how the weights are calculated. This defaults to a flat weighting strategy. Weights are summed and scaled so that they add up to 1.

==== Flat Weighting ====

All weights are 1. This is completely position in-senitive.

==== Linear Weighting ====

Weights are calculated according to <math>w = 1 - r/R</math>, where w is the weighting factor, r is the distance from the detector and R is the cut-off radius.

==== Parabolic Weighting ====
For rectangular detectors it may be used as follows: The radius must be zero and a AdjX and AdjY parameter must be provided. <math>w = AdjX - abs(x) + AdjY - abs(y) + 1</math>

For non-rectangular detectors, the cut-off radius is used in the calculation. <math>w = R - abs(x) + R - abs(y) + 1</math>

==== Gaussian Weighting ====
This weighting is calculated from the Gaussian distribution

<math>w = e^{-r^2/(2\sigma^2)}</math>

where <math>r^2 = x^2 + y^2 + z^2</math>
and <math>\sigma</math> is the number of standard deviations controlling the width of the distribution curve

Important notes about this algorithm are that:
* Distances are normalised by the radius cut-off to make them dimensionless and scaled to 1 at the boundaries. 

=== For EventWorkspaces ===

Both methods of smoothing will '''significantly''' increase the memory usage of
the workspace. For example, if AdjX=AdjY=1, the algorithm will sum 9 nearest neighbours in most cases.
This increases the memory used by a factor of 9.

=== For Workspace2D's ===

You can use PreserveEvents = false to avoid the memory issues with an EventWorkspace input.
Please note that the algorithm '''does not check''' that the bin X boundaries match.

=== Neighbour Searching ===
<gallery>
File:NNSearchByRadius.jpg|''Fig. 1''. 
File:NNSearchIrregularGrid.jpg|''Fig. 2''. 
File:NNSearchLimitByRadius.jpg|''Fig. 3''
File:NNSearchLimitByNNs.jpg|''Fig. 4''
File:NNSearchXY.jpg|''Fig. 5''
</gallery>

==== Property Values of Examples ====

''Fig. 1'' : Requesting NumberOfNeighbours=36, Radius=3. Algorithm looks for 36 nearest neighbours with a cut-off of 3 detector widths.<br>
''Fig. 2'' : Requesting NumberOfNeighbours=46, Radius=2. Algorithm looks for 46 nearest neighbours with a cut-off of 2 detector widths.<br>
''Fig. 3'' : Requesting NumberOfNeighbours=56, Radius=3. Algorithm looks for 56 nearest neighbours with a cut-off of 3 detector widths.<br>
''Fig. 4'' : Requesting NumberOfNeighbours=8, Radius=3. Algorithm looks for 8 nearest neighbours with a cut-off of 3 detector widths.<br>
''Fig. 5'' : Requesting AdjX=4, AdjY=2, Radius=0. Algorithm fetches neighbours in the specified pattern.

==== How it Works ====

The algorithm will fetch neigbours using the intesection of those inside the radius cut-off and those less than the NumberOfNeighbours specified. ''Fig. 1'' illustrates this process. Searching is relative to the central detector, those constrained by both specified number of neighbours have been highlighted. In this case the radius cut-off and the number of neighbours constrain the same number of detectors. 

Searching via the number of neighbours will not necessarily return the neighbours in a grid with the same number of detectors in each axis. ''Fig. 2'' shows how neighbours might be returned if distances are non-uniform. If RectangularDetectors are available, you may force the searching to occur in rectangular manner (described below).

The SmoothingNeighbours algorithm will only take those neighbours which are in the intersection between those constrained by the cut-off and those constrained by the specified number of neighbours. If the radius cut-off is the limiting factor, then those neighbours outside will not be considered. This is illustrated in ''Fig. 3'' where the blue detectors will not be considered, but will not with this radius cut-off, while the green ones will. Likewise, in ''Fig. 4'' the effect of reducing the NumberOfNeighbours property can be seen.

If the radius is set to 0, the instrument is treated as though it has rectangular detectors. AdjX and AdjY can then be used to control the number of neighbours independently in x and y using the AdjX and AdjY properties. ''Fig. 5'' Shows the effect of this type of searching.

=== Ignore Masks ===

The algorithm will ignore masked detectors if this flag is set.

*WIKI*/


#include "MantidAlgorithms/SmoothNeighbours.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using std::map;

typedef std::vector<Mantid::Kernel::Property*> VecProperties;
typedef const VecProperties ConstVecProperties;

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SmoothNeighbours)

// Used in custom GUI. Make sure you change them in SmoothNeighboursDialog.cpp as well.
const std::string SmoothNeighbours::NON_UNIFORM_GROUP = "NonUniform Detectors";
const std::string SmoothNeighbours::RECTANGULAR_GROUP = "Rectangular Detectors";
const std::string SmoothNeighbours::INPUT_WORKSPACE = "InputWorkspace";

SmoothNeighbours::SmoothNeighbours() :
    API::Algorithm() , WeightedSum(new NullWeighting)
{
}

/// Sets documentation strings for this algorithm
void SmoothNeighbours::initDocs()
{
  this->setWikiSummary("Perform a moving-average smoothing by summing spectra of nearest neighbours over the face of detectors.");
  this->setOptionalMessage("Perform a moving-average smoothing by summing spectra of nearest neighbours over the face of detectors.");
}

/** Initialisation method.
 *
 */
void SmoothNeighbours::init()
{
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>(INPUT_WORKSPACE,"",Direction::Input, boost::make_shared<InstrumentValidator>()),
      "The workspace containing the spectra to be averaged." );
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
      "The name of the workspace to be created as the output of the algorithm." );

  //Unsigned double
  auto mustBePositiveDouble = boost::make_shared<BoundedValidator<double> >();
  mustBePositiveDouble->setLower(0.0);

  //Unsigned int.
  auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
  mustBePositive->setLower(0); 

  std::vector<std::string> propOptions;
    propOptions.push_back("Flat");
    propOptions.push_back("Linear");
    propOptions.push_back("Parabolic");
    propOptions.push_back("Gaussian");
    declareProperty("WeightedSum", "Flat",boost::make_shared<StringListValidator>(propOptions),
      "What sort of Weighting scheme to use?\n"
      "  Flat: Effectively no-weighting, all weights are 1.\n"
      "  Linear: Linear weighting 1 - r/R from origin.\n"
      "  Parabolic : Weighting as cutoff - x + cutoff - y + 1."
      "  Gaussian : Uses the absolute distance x^2 + y^2 ... normalised by the cutoff^2"
       );

  declareProperty("Sigma", 0.5, mustBePositiveDouble, "Sigma value for gaussian weighting schemes. Defaults to 0.5. ");
  setPropertySettings("Sigma", new EnabledWhenProperty("WeightedSum", IS_EQUAL_TO, "Gaussian"));

  declareProperty("IgnoreMaskedDetectors", true, "If true, do not consider masked detectors in the NN search.");

  declareProperty("PreserveEvents", true,
     "If the InputWorkspace is an EventWorkspace, this will preserve the full event list (warning: this will use much more memory!).");

  // -- Rectangular properties ----------------------------------------------------------------------

  declareProperty("AdjX", 1, mustBePositive,
    "The number of X (horizontal) adjacent pixels to average together. "
    "Only for instruments with RectangularDetectors. ");

  declareProperty("AdjY", 1, mustBePositive,
    "The number of Y (vertical) adjacent pixels to average together. "
    "Only for instruments with RectangularDetectors. ");

  declareProperty("SumPixelsX", 1, mustBePositive,
    "The total number of X (horizontal) adjacent pixels to sum together. "
    "Only for instruments with RectangularDetectors.  AdjX will be ignored if SumPixelsX > 1.");

  declareProperty("SumPixelsY", 1, mustBePositive,
    "The total number of Y (vertical) adjacent pixels to sum together. "
    "Only for instruments with RectangularDetectors. AdjY will be ignored if SumPixelsY > 1");

  declareProperty("ZeroEdgePixels", 0, mustBePositive,
    "The number of pixels to zero at edges. "
    "Only for instruments with RectangularDetectors. ");

  setPropertyGroup("AdjX", RECTANGULAR_GROUP);
  setPropertyGroup("AdjY", RECTANGULAR_GROUP);
  setPropertyGroup("SumPixelsX", RECTANGULAR_GROUP);
  setPropertyGroup("SumPixelsY", RECTANGULAR_GROUP);
  setPropertyGroup("ZeroEdgePixels", RECTANGULAR_GROUP);

  // -- Non-uniform properties ----------------------------------------------------------------------

  std::vector<std::string> radiusPropOptions;
  radiusPropOptions.push_back("Meters");
  radiusPropOptions.push_back("NumberOfPixels");
  declareProperty("RadiusUnits", "Meters",boost::make_shared<StringListValidator>(radiusPropOptions),
        "Units used to specify the radius?\n"
        "  Meters : Radius is in meters.\n"
        "  NumberOfPixels : Radius is in terms of the number of pixels."
         );

  declareProperty("Radius", 0.0, mustBePositiveDouble,
      "The radius around a pixel to look for nearest neighbours to average. \n"
      "If 0, will use the AdjX and AdjY parameters for rectangular detectors instead." );

    declareProperty("NumberOfNeighbours", 8, mustBePositive, "Number of nearest neighbouring pixels.\n"
      "Alternative to providing the radius. The default is 8.");

    declareProperty("SumNumberOfNeighbours", 1, "Sum nearest neighbouring pixels with same parent.\n"
      "Number of pixels will be reduced. The default is false.");

  setPropertyGroup("RadiusUnits", NON_UNIFORM_GROUP);
  setPropertyGroup("Radius", NON_UNIFORM_GROUP);
  setPropertyGroup("NumberOfNeighbours", NON_UNIFORM_GROUP);
  setPropertyGroup("SumNumberOfNeighbours", NON_UNIFORM_GROUP);
}


//--------------------------------------------------------------------------------------------
/** Fill the neighbours list given the AdjX AdjY parameters and an
 * instrument with rectangular detectors.
 */
void SmoothNeighbours::findNeighboursRectangular()
{
  g_log.debug("SmoothNeighbours processing assuming rectangular detectors.");

  m_prog->resetNumSteps(inWS->getNumberHistograms(), 0.2, 0.5);

  Instrument_const_sptr inst = inWS->getInstrument();

  //To get the workspace index from the detector ID
  const detid2index_map pixel_to_wi = inWS->getDetectorIDToWorkspaceIndexMap(true);

  //std::cout << " inst->nelements() " << inst->nelements() << "\n";
  Progress prog(this,0.0,1.0,inst->nelements());

  //Build a list of Rectangular Detectors
  std::vector<boost::shared_ptr<RectangularDetector> > detList;
  for (int i=0; i < inst->nelements(); i++)
  {
    boost::shared_ptr<RectangularDetector> det;
    boost::shared_ptr<ICompAssembly> assem;
    boost::shared_ptr<ICompAssembly> assem2;

    det = boost::dynamic_pointer_cast<RectangularDetector>( (*inst)[i] );
    if (det)
    {
      detList.push_back(det);
    }
    else
    {
      //Also, look in the first sub-level for RectangularDetectors (e.g. PG3).
      // We are not doing a full recursive search since that will be very long for lots of pixels.
      assem = boost::dynamic_pointer_cast<ICompAssembly>( (*inst)[i] );
      if (assem)
      {
        for (int j=0; j < assem->nelements(); j++)
        {
          det = boost::dynamic_pointer_cast<RectangularDetector>( (*assem)[j] );
          if (det)
          {
            detList.push_back(det);

          }
          else
          {
            //Also, look in the second sub-level for RectangularDetectors (e.g. PG3).
            // We are not doing a full recursive search since that will be very long for lots of pixels.
            assem2 = boost::dynamic_pointer_cast<ICompAssembly>( (*assem)[j] );
            if (assem2)
            {
              for (int k=0; k < assem2->nelements(); k++)
              {
                det = boost::dynamic_pointer_cast<RectangularDetector>( (*assem2)[k] );
                if (det)
                {
                  detList.push_back(det);
                }
              }
            }
          }
        }
      }
    }
  }

  if (detList.empty())
  {
    // Not rectangular so use Nearest Neighbours
    Radius = translateToMeters("NumberOfPixels", std::max(AdjX,AdjY));
    setWeightingStrategy("Flat", Radius);
    nNeighbours = AdjX * AdjY - 1;
    findNeighboursUbiqutious();
    return;
  }

  // Resize the vector we are setting
  m_neighbours.resize(inWS->getNumberHistograms());
  int StartX = -AdjX;
  int StartY = -AdjY;
  int EndX = AdjX;
  int EndY = AdjY;
  int SumX = getProperty("SumPixelsX");
  int SumY = getProperty("SumPixelsY");
  bool sum = ( SumX*SumY > 1 ) ? true : false;
  if (sum)
  {
    StartX = 0;
    StartY = 0;
    EndX = SumX - 1;
    EndY = SumY - 1;
  }

  outWI = 0;
  // Build a map to sort by the detectorID
  std::vector <std::pair<int, int> > v1;
  for (int i = 0; i<static_cast<int>(detList.size()); i++)
    v1.push_back(std::pair<int, int>(detList[i]->getAtXY(0,0)->getID(), i));

  // To sort in descending order
  if(sum) stable_sort(v1.begin(), v1.end() );

  std::vector <std::pair<int, int> >::iterator Iter1;

  //Loop through the RectangularDetector's we listed before.
  for ( Iter1 = v1.begin() ; Iter1 != v1.end() ; ++Iter1 )
  {
    int i = (*Iter1).second;
    boost::shared_ptr<RectangularDetector> det = detList[i];
    std::string det_name = det->getName();
    if (det)
    {
      for (int j=Edge; j < det->xpixels()-Edge; j += SumX)
      {
        for (int k=Edge; k < det->ypixels()-Edge; k += SumY)
        {
          double totalWeight = 0;
          // Neighbours and weights
          std::vector< weightedNeighbour > neighbours;

          for (int ix=StartX; ix <= EndX; ix++)
            for (int iy=StartY; iy <= EndY; iy++)
            {
              //Weights for corners=1; higher for center and adjacent pixels
              double smweight = WeightedSum->weightAt(AdjX, ix, AdjY, iy);

              //Find the pixel ID at that XY position on the rectangular detector
              if(j+ix >= det->xpixels() || j+ix < 0) continue;
              if(k+iy >= det->ypixels() || k+iy < 0) continue;
              int pixelID = det->getAtXY(j+ix,k+iy)->getID();

              //Find the corresponding workspace index, if any
              auto mapEntry = pixel_to_wi.find(pixelID);
              if (mapEntry != pixel_to_wi.end())
              {
                size_t wi = mapEntry->second;
                neighbours.push_back( weightedNeighbour(wi, smweight) );
                // Count the total weight
                totalWeight += smweight;
              }
            }

          // Adjust the weights of each neighbour to normalize to unity
          if (!sum) for (size_t q=0; q<neighbours.size(); q++)
            neighbours[q].second /= totalWeight;

          // Save the list of neighbours for this output workspace index.
          m_neighbours[outWI] = neighbours;
          outWI ++;

          m_prog->report("Finding Neighbours");
        }
      }
    }
    prog.report(det_name);
  }

}


//--------------------------------------------------------------------------------------------
/** Use NearestNeighbours to find the neighbours for any instrument
 */
void SmoothNeighbours::findNeighboursUbiqutious()
{
   g_log.debug("SmoothNeighbours processing NOT assuming rectangular detectors.");
  /*
    This will cause the Workspace to rebuild the nearest neighbours map, so that we can pick-up any of the properties specified
    for this algorithm in the constructor for the NearestNeighboursObject.
  */
  inWS->rebuildNearestNeighbours();

  m_prog->resetNumSteps(inWS->getNumberHistograms(), 0.2, 0.5);
  this->progress(0.2, "Building Neighbour Map");

  Instrument_const_sptr inst = inWS->getInstrument();
  const spec2index_map spec2index = inWS->getSpectrumToWorkspaceIndexMap();

  // Resize the vector we are setting
  m_neighbours.resize(inWS->getNumberHistograms());

  bool ignoreMaskedDetectors = getProperty("IgnoreMaskedDetectors");

  //Cull by radius
  RadiusFilter radiusFilter(Radius);

  IDetector_const_sptr det;
  // Go through every input workspace pixel
  outWI = 0;
  int sum = getProperty("SumNumberOfNeighbours");
  boost::shared_ptr<const Geometry::IComponent> parent, neighbParent, grandparent, neighbGParent;
  bool* used = new bool[inWS->getNumberHistograms()];
  if (sum > 1)
  {
    for (size_t wi=0; wi < inWS->getNumberHistograms(); wi++)
      used[wi] = false;
  }
  for (size_t wi=0; wi < inWS->getNumberHistograms(); wi++)
  {
    if (sum > 1 ) if (used[wi]) continue;
    // We want to skip monitors
    try
    {
      det = inWS->getDetector(wi);
      if( det->isMonitor() ) continue; //skip monitor
      if( det->isMasked() )
      {
    	  //Calibration masks many detectors, but there should be 0s after smoothing
    	  if(sum == 1) outWI++;
    	  continue; //skip masked detectors
      }
      if(sum > 1)
      {
        parent = det->getParent();
        grandparent = parent->getParent();
      }
    }
    catch(Kernel::Exception::NotFoundError&)
    {
      continue; //skip missing detector
    }

    specid_t inSpec = inWS->getSpectrum(wi)->getSpectrumNo();

    //Step one - Get the number of specified neighbours
    SpectraDistanceMap insideGrid  = inWS->getNeighboursExact(inSpec, nNeighbours, ignoreMaskedDetectors); 

    //Step two - Filter the results by the radius cut off.
    SpectraDistanceMap neighbSpectra = radiusFilter.apply(insideGrid);
    
    // Force the central pixel to always be there
    // There seems to be a bug in nearestNeighbours, returns distance != 0.0 for the central pixel. So we force distance = 0
    neighbSpectra[inSpec] = V3D(0.0, 0.0, 0.0);

    // Neighbours and weights list
    double totalWeight = 0;
    int noNeigh = 0;
    std::vector< weightedNeighbour > neighbours;

    // Convert from spectrum numbers to workspace indices
    for (SpectraDistanceMap::iterator it = neighbSpectra.begin(); it != neighbSpectra.end(); ++it)
    {
      specid_t spec = it->first;

      //Use the weighting strategy to calculate the weight.
      double weight = WeightedSum->weightAt(it->second); 

      if (weight > 0) 
      {
        // Find the corresponding workspace index
        spec2index_map::const_iterator mapIt = spec2index.find(spec);
        if (mapIt != spec2index.end())
        {
          size_t neighWI = mapIt->second;
          if(sum > 1)
          {
            neighbParent = inWS->getDetector(neighWI)->getParent();
            neighbGParent = neighbParent->getParent();
            if(noNeigh >= sum || neighbParent->getName().compare(parent->getName()) > 0 || neighbGParent->getName().compare(grandparent->getName()) > 0 || used[neighWI])continue;
            noNeigh++;
            used[neighWI] = true;
          }
          neighbours.push_back( weightedNeighbour(neighWI, weight) );
          totalWeight += weight;
        }
      }
    } 

    // Adjust the weights of each neighbour to normalize to unity
    if (sum == 1) for (size_t q=0; q<neighbours.size(); q++)
      neighbours[q].second /= totalWeight;

    // Save the list of neighbours for this output workspace index.
    m_neighbours[outWI] = neighbours;
    outWI++;

    m_prog->report("Finding Neighbours");
  } // each workspace index

  delete [] used;
}

/**
Attempts to reset the Weight based on the strategyName provided. Note that if these conditional 
statements fail to override the existing WeightedSum member, it should stay as a NullWeighting, which
will throw during usage.
@param strategyName : The name of the weighting strategy to use
@param cutOff : The cutoff distance
*/
void SmoothNeighbours::setWeightingStrategy(const std::string strategyName, double& cutOff)
{
  if(strategyName == "Flat")
  {
    boost::scoped_ptr<WeightingStrategy> flatStrategy(new FlatWeighting);
    WeightedSum.swap(flatStrategy);
    g_log.information("Smoothing with Flat Weighting");
  }
  else if(strategyName == "Linear")
  {
    boost::scoped_ptr<WeightingStrategy> linearStrategy(new LinearWeighting(cutOff));
    WeightedSum.swap(linearStrategy);
    g_log.information("Smoothing with Linear Weighting");
  }
  else if(strategyName == "Parabolic")
  {
    boost::scoped_ptr<WeightingStrategy>  parabolicStrategy(new ParabolicWeighting(cutOff));
    WeightedSum.swap(parabolicStrategy);
    g_log.information("Smoothing with Parabolic Weighting");
  }
  else if(strategyName == "Gaussian")
  {
    double sigma = getProperty("Sigma");
    boost::scoped_ptr<WeightingStrategy> gaussian1DStrategy(new GaussianWeightingnD(cutOff, sigma));
    WeightedSum.swap(gaussian1DStrategy);
    g_log.information("Smoothing with Gaussian Weighting");
  }
}


/*
Fetch the instrument associated with the workspace
@return const shared pointer to the instrument,
*/
Instrument_const_sptr SmoothNeighbours::fetchInstrument() const
{
    Instrument_const_sptr instrument;
    EventWorkspace_sptr wsEvent = boost::dynamic_pointer_cast<EventWorkspace>(inWS);
    MatrixWorkspace_sptr wsMatrix = boost::dynamic_pointer_cast<MatrixWorkspace>(inWS);
    if(wsEvent)
    {
      instrument = boost::dynamic_pointer_cast<EventWorkspace>(inWS)->getInstrument();
    }
    else if(wsMatrix)
    {
      instrument = boost::dynamic_pointer_cast<MatrixWorkspace>(inWS)->getInstrument();
    }
    else
    {
      throw std::invalid_argument("Neither a Matrix Workspace or an EventWorkpace provided to SmoothNeighbours.");
    }
    return instrument;
}

/**
Translate the radius into meters.
@param radiusUnits : The name of the radius units
@param enteredRadius : The numerical value of the radius in whatever units have been specified
*/
double SmoothNeighbours::translateToMeters(const std::string radiusUnits, const double& enteredRadius)
{
  double translatedRadius = 0;
  if(radiusUnits == "Meters")
  {
    // Nothing more to do.
    translatedRadius = enteredRadius; 
  }
  else if(radiusUnits == "NumberOfPixels")
  {
    // Fetch the instrument.
    Instrument_const_sptr instrument = fetchInstrument();

    // Get the first idetector from the workspace index 0.
    IDetector_const_sptr firstDet = inWS->getDetector(0);
    // Find the bounding box of that detector
    BoundingBox bbox;
    firstDet->getBoundingBox(bbox);
    // Multiply (meters/pixels) by number of pixels, note that enteredRadius takes on meaning of the number of pixels.
    translatedRadius = bbox.width().norm() * enteredRadius;
  }
  else
  {
    std::string message = "SmoothNeighbours::translateToMeters, Unknown Unit: " + radiusUnits;
    throw std::invalid_argument(message);
  }
  return translatedRadius;
}

/*
Check whether the properties provided are all in their default state.
@param properties : Vector of mantid property pointers
@return True only if they are all default, otherwise False.
*/
bool areAllDefault(ConstVecProperties &properties)
{
  bool areAllDefault = false;
  for(ConstVecProperties::const_iterator it = properties.begin(); it != properties.end(); ++it)
  {
    if(!(*it)->isDefault())
    {
      return areAllDefault;
    }
  }
  areAllDefault = true;
  return areAllDefault;
}

//--------------------------------------------------------------------------------------------
/** Executes the algorithm
 *
 */
void SmoothNeighbours::exec()
{
  inWS = getProperty("InputWorkspace");

  PreserveEvents = getProperty("PreserveEvents");

  // Use the unit type to translate the entered radius into meters.
  Radius = translateToMeters(getProperty("RadiusUnits"), getProperty("Radius"));

  setWeightingStrategy(getProperty("WeightedSum"), Radius);

  AdjX = getProperty("AdjX");
  AdjY = getProperty("AdjY");
  Edge = getProperty("ZeroEdgePixels");

  nNeighbours = getProperty("NumberOfNeighbours");

  // Progress reporting, first for the sorting
  m_prog = new Progress(this, 0.0, 0.2, inWS->getNumberHistograms());

  // Run the appropriate method depending on the type of the instrument
  if(inWS->getInstrument()->containsRectDetectors() == Instrument::ContainsState::Full)
    findNeighboursRectangular();
  else
    findNeighboursUbiqutious();

  EventWorkspace_sptr wsEvent = boost::dynamic_pointer_cast<EventWorkspace>(inWS);
  if (wsEvent)
    wsEvent->sortAll(TOF_SORT, m_prog);

  if (!wsEvent || !PreserveEvents)
    this->execWorkspace2D();
  else if (wsEvent)
    this->execEvent(wsEvent);
  else
    throw std::runtime_error("This algorithm requires a Workspace2D or EventWorkspace as its input.");
}

//--------------------------------------------------------------------------------------------
/** Execute the algorithm for a Workspace2D/don't preserve events input */
void SmoothNeighbours::execWorkspace2D()
{
  m_prog->resetNumSteps(inWS->getNumberHistograms(), 0.5, 1.0);

  //Get some stuff from the input workspace
  const size_t numberOfSpectra = outWI;
  
  const size_t YLength = inWS->blocksize();

  MatrixWorkspace_sptr outWS;
  //Make a brand new Workspace2D
  if (boost::dynamic_pointer_cast<OffsetsWorkspace>(inWS))
  {
    g_log.information() << "Creating new OffsetsWorkspace\n";
    outWS = MatrixWorkspace_sptr(new OffsetsWorkspace(inWS->getInstrument()));
  }
  else
  {
    outWS = boost::dynamic_pointer_cast<MatrixWorkspace>
        ( API::WorkspaceFactory::Instance().create("Workspace2D", numberOfSpectra, YLength+1, YLength));
  }
  this->setProperty("OutputWorkspace", outWS);

  setupNewInstrument(outWS);

  //Copy geometry over.
  // API::WorkspaceFactory::Instance().initializeFromParent(inWS, outWS, false);

  // Go through all the output workspace
  PARALLEL_FOR2(inWS, outWS)
  for (int outWIi=0; outWIi<int(numberOfSpectra); outWIi++)
  {
    PARALLEL_START_INTERUPT_REGION

    ISpectrum * outSpec = outWS->getSpectrum(outWIi);
    // Reset the Y and E vectors
    outSpec->clearData();
    MantidVec & outY = outSpec->dataY();
    // We will temporarily carry the squared error
    MantidVec & outE = outSpec->dataE();
    // tmp to carry the X Data.
    MantidVec & outX = outSpec->dataX();

    // Which are the neighbours?
    std::vector< weightedNeighbour > & neighbours = m_neighbours[outWIi];
    std::vector< weightedNeighbour >::iterator it;
    for (it = neighbours.begin(); it != neighbours.end(); ++it)
    {
      size_t inWI = it->first;
      double weight = it->second;
      double weightSquared = weight * weight;

      const ISpectrum * inSpec = inWS->getSpectrum(inWI);
      inSpec->lockData();
      const MantidVec & inY = inSpec->readY();
      const MantidVec & inE = inSpec->readE();
      const MantidVec & inX = inSpec->readX();

      for (size_t i=0; i<YLength; i++)
      {
        // Add the weighted signal
        outY[i] += inY[i] * weight;
        // Square the error, scale by weight (which you have to square too), then add in quadrature
        double errorSquared = inE[i];
        errorSquared *= errorSquared;
        errorSquared *= weightSquared;
        outE[i] += errorSquared;
        // Copy the X values as well
        outX[i] = inX[i];
      }
      if(inWS->isHistogramData())
      {
        outX[YLength] = inX[YLength];
      }

      inSpec->unlockData();
    } //(each neighbour)

    // Now un-square the error, since we summed it in quadrature
    for (size_t i=0; i<YLength; i++)
      outE[i] = sqrt(outE[i]);

    //Copy the single detector ID (of the center) and spectrum number from the input workspace
    // outSpec->copyInfoFrom(*inWS->getSpectrum(outWIi));

    m_prog->report("Summing");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

}

//--------------------------------------------------------------------------------------------
/** Build the instrument/detector setup in workspace
  */
void SmoothNeighbours::setupNewInstrument(MatrixWorkspace_sptr outws)
{
  //Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(inWS, outws, false);

  // Go through all the output workspace
  size_t numberOfSpectra = outws->getNumberHistograms();

  for (int outWIi=0; outWIi<int(numberOfSpectra); outWIi++)
  {
    ISpectrum * outSpec = outws->getSpectrum(outWIi);
    /*
    g_log.notice() << "[DBx555] Original spectrum number for wsindex " << outWIi
                   << " = " << outSpec->getSpectrumNo() << std::endl;
    outSpec->setSpectrumNo(outWIi+1);
    */

    // Reset detectors
    outSpec->clearDetectorIDs();;

    // Which are the neighbours?
    std::vector< weightedNeighbour > & neighbours = m_neighbours[outWIi];
    std::vector< weightedNeighbour >::iterator it;
    for (it = neighbours.begin(); it != neighbours.end(); ++it)
    {
      size_t inWI = it->first;

      const ISpectrum * inSpec = inWS->getSpectrum(inWI);

      std::set<detid_t> thesedetids = inSpec->getDetectorIDs();
      outSpec->addDetectorIDs(thesedetids);

    } //(each neighbour)
  }

  return;
}

//--------------------------------------------------------------------------------------------
/** Execute the algorithm for a EventWorkspace input
 * @param ws :: EventWorkspace
 */
void SmoothNeighbours::execEvent(Mantid::DataObjects::EventWorkspace_sptr ws)
{
  m_prog->resetNumSteps(inWS->getNumberHistograms(), 0.5, 1.0);

  //Get some stuff from the input workspace
  const size_t numberOfSpectra = outWI;
  const int YLength = static_cast<int>(inWS->blocksize());

  EventWorkspace_sptr outWS;
  //Make a brand new EventWorkspace
  outWS = boost::dynamic_pointer_cast<EventWorkspace>( API::WorkspaceFactory::Instance().create("EventWorkspace", numberOfSpectra, YLength+1, YLength));
  //Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(ws, outWS, false);
  // Ensure thread-safety
  outWS->sortAll(TOF_SORT, NULL);

  this->setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(outWS));

  // Go through all the output workspace
  PARALLEL_FOR2(ws, outWS)
  for (int outWIi=0; outWIi<int(numberOfSpectra); outWIi++)
  {
    PARALLEL_START_INTERUPT_REGION

    // Create the output event list (empty)
    EventList & outEL = outWS->getOrAddEventList(outWIi);

    // Which are the neighbours?
    std::vector< weightedNeighbour > & neighbours = m_neighbours[outWIi];
    std::vector< weightedNeighbour >::iterator it;
    for (it = neighbours.begin(); it != neighbours.end(); ++it)
    {
      size_t inWI = it->first;
      //if(sum)outEL.copyInfoFrom(*ws->getSpectrum(inWI));
      double weight = it->second;
      // Copy the event list
      EventList tmpEL = ws->getEventList(inWI);
      // Scale it
      tmpEL *= weight;
      // Add it
      outEL += tmpEL;
    }

    //Copy the single detector ID (of the center) and spectrum number from the input workspace
    //if (!sum) outEL.copyInfoFrom(*ws->getSpectrum(outWIi));

    m_prog->report("Summing");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION


  //Give the 0-th X bins to all the output spectra.
  Kernel::cow_ptr<MantidVec> outX = inWS->refX(0);
  outWS->setAllX( outX );
}



} // namespace Algorithms
} // namespace Mantid
