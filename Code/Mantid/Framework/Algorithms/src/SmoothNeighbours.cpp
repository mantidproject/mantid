/*WIKI* 

This algorithm performs a moving-average smoothing of data by summing spectra of nearest neighbours over the face of detectors.
The output workspace has the same number of spectra as the input workspace.
This works on both [[EventWorkspace]]s and [[Workspace2D]]'s.
It has two main modes of operation.

=== For All Instruments ===

Going through the input workspace pixel-by-pixel, Mantid finds the nearest-neighbours with the given Radius of each
pixel. The spectra are then summed together, and normalizing to unity (see the weighting section below).

=== For Instruments With Rectangular Detectors ===

The algorithm looks through the [[Instrument]] to find all the [[RectangularDetector]]s defined.
For each pixel in each detector, the AdjX*AdjY neighboring spectra are summed together and saved in the output workspace.

=== WeightedSum parameter ===

* For All Instruments: A weight of 1.0 is given to the center pixel. Other pixels are given the weight <math>w = 1 - r/Radius</math>, so pixels near the edge are given a weight of ~ 0.
* For Rectangular Detectors: Parabolic weights are applied to each added eventlist with the largest weights near the center.
* Weights are summed and scaled so that they add up to 1.

=== For EventWorkspaces ===

Both methods of smoothing will '''significantly''' increase the memory usage of
the workspace. For example, if AdjX=AdjY=1, the algorithm will sum 9 nearest neighbours in most cases.
This increases the memory used by a factor of 9.

=== For Workspace2D's ===

You can use PreserveEvents = false to avoid the memory issues with an EventWorkspace input.
Please note that the algorithm '''does not check''' that the bin X boundaries match.

*WIKI*/


#include "MantidAlgorithms/SmoothNeighbours.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include <boost/algorithm/string.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using std::map;

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SmoothNeighbours)

SmoothNeighbours::SmoothNeighbours() : API::Algorithm() , WeightedSum(new NullWeighting)
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
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,new InstrumentValidator<>),
                            "The workspace containing the spectra to be averaged." );
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm." );

  std::vector<std::string> radiusPropOptions;
  radiusPropOptions.push_back("Meters");
    radiusPropOptions.push_back("NumberOfPixels");
    declareProperty("RadiusUnits", "Meters",new ListValidator(radiusPropOptions),
      "Units used to specify the radius?\n"
      "  Meters : Radius is in meters.\n"
      "  NumberOfPixels : Radius is in terms of the number of pixels."
       );

  //Unsigned double
  BoundedValidator<double> *mustBePositiveDouble = new BoundedValidator<double>();
  mustBePositiveDouble->setLower(0.0);

  //Unsigned int.
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0); 

  declareProperty("Radius", 0.0, mustBePositiveDouble,
    "The radius around a pixel to look for nearest neighbours to average. \n"
    "If 0, will use the AdjX and AdjY parameters for rectangular detectors instead." );

  declareProperty("NumberOfNeighbours", 8, mustBePositive->clone(), "Number of nearest neighbouring pixels.\n"
    "Alternative to providing the radius. The default is 8.");

  declareProperty("IgnoreMaskedDetectors", true, "If true, do not consider masked detectors in the NN search.");

  std::vector<std::string> propOptions;
    propOptions.push_back("Flat");
    propOptions.push_back("Linear");
    declareProperty("WeightedSum", "Flat",new ListValidator(propOptions),
      "What sort of Weighting scheme to use?\n"
      "  Flat: Effectively no-weighting, all weights are 1.\n"
      "  Linear: Linear weighting 1 - r/R from origin."
       );

  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.

  declareProperty("AdjX", 1, mustBePositive->clone(),
    "The number of X (horizontal) adjacent pixels to average together. Only for instruments with RectangularDetectors. " );
  setPropertySettings("AdjX", new EnabledWhenProperty(this, "Radius", IS_DEFAULT) );

  declareProperty("AdjY", 1, mustBePositive->clone(),
    "The number of Y (vertical) adjacent pixels to average together. Only for instruments with RectangularDetectors. " );
  setPropertySettings("AdjY", new EnabledWhenProperty(this, "Radius", IS_DEFAULT) );

  declareProperty("ZeroEdgePixels", 0, mustBePositive->clone(),
    "The number of pixels to zero at edges. Only for instruments with RectangularDetectors. " );
  setPropertySettings("ZeroEdgePixels", new EnabledWhenProperty(this, "Radius", IS_DEFAULT) );

  setPropertyGroup("AdjX", "Rectangular Detectors Only");
  setPropertyGroup("AdjY", "Rectangular Detectors Only");
  setPropertyGroup("ZeroEdgePixels", "Rectangular Detectors Only");

  declareProperty("PreserveEvents", true,
    "If the InputWorkspace is an EventWorkspace, this will preserve the full event list (warning: this will use much more memory!).");

}


//--------------------------------------------------------------------------------------------
/** Fill the neighbours list given the AdjX AdjY parameters and an
 * instrument with rectangular detectors.
 */
void SmoothNeighbours::findNeighboursRectangular()
{
  m_prog->resetNumSteps(inWS->getNumberHistograms(), 0.2, 0.5);

  Instrument_const_sptr inst = inWS->getInstrument();

  //To get the workspace index from the detector ID
  detid2index_map * pixel_to_wi = inWS->getDetectorIDToWorkspaceIndexMap(true);

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

  if (detList.size() == 0)
    throw std::runtime_error("This instrument does not have any RectangularDetector's. SmoothNeighbours cannot operate on this instrument at this time.");

  // Resize the vector we are setting
  m_neighbours.resize(inWS->getNumberHistograms());

  //Loop through the RectangularDetector's we listed before.
  for (int i=0; i < static_cast<int>(detList.size()); i++)
  {
    boost::shared_ptr<RectangularDetector> det = detList[i];
    std::string det_name = det->getName();
    if (det)
    {
      size_t outWI = 0;
      for (int j=Edge; j < det->xpixels()-Edge; j++)
      {
        for (int k=Edge; k < det->ypixels()-Edge; k++)
        {
          double totalWeight = 0;
          // Neighbours and weights
          std::vector< weightedNeighbour > neighbours;

          for (int ix=-AdjX; ix <= AdjX; ix++)
            for (int iy=-AdjY; iy <= AdjY; iy++)
            {
              //Weights for corners=1; higher for center and adjacent pixels
              double smweight = WeightedSum->weightAt(AdjX, ix, AdjY, iy);

              //Find the pixel ID at that XY position on the rectangular detector
              if(j+ix >= det->xpixels() || j+ix < 0) continue;
              if(k+iy >= det->ypixels() || k+iy < 0) continue;
              int pixelID = det->getAtXY(j+ix,k+iy)->getID();

              //Find the corresponding workspace index, if any
              if (pixel_to_wi->find(pixelID) != pixel_to_wi->end())
              {
                size_t wi = (*pixel_to_wi)[pixelID];
                // Find the output workspace index at the center
                if( ix==0 && iy==0) outWI = wi;
                neighbours.push_back( weightedNeighbour(wi, smweight) );
                // Count the total weight
                totalWeight += smweight;
              }
            }

          // Adjust the weights of each neighbour to normalize to unity
          for (size_t q=0; q<neighbours.size(); q++)
            neighbours[q].second /= totalWeight;

          // Save the list of neighbours for this output workspace index.
          m_neighbours[outWI] = neighbours;

          m_prog->report("Finding Neighbours");
        }
      }
    }
    prog.report(det_name);
  }


  delete pixel_to_wi;

}


//--------------------------------------------------------------------------------------------
/** Use NearestNeighbours to find the neighbours for any instrument
 */
void SmoothNeighbours::findNeighboursUbiqutious()
{
  /*
    This will cause the Workspace to rebuild the nearest neighbours map, so that we can pick-up any of the properties specified
    for this algorithm in the constructor for the NearestNeighboursObject.
  */
  inWS->rebuildNearestNeighbours();

  m_prog->resetNumSteps(inWS->getNumberHistograms(), 0.2, 0.5);
  this->progress(0.2, "Building Neighbour Map");

  Instrument_const_sptr inst = inWS->getInstrument();
  spec2index_map * spec2index = inWS->getSpectrumToWorkspaceIndexMap();

  // Resize the vector we are setting
  m_neighbours.resize(inWS->getNumberHistograms());

  int nNeighbours = getProperty("NumberOfNeighbours");
  bool ignoreMaskedDetectors = getProperty("IgnoreMaskedDetectors");

  IDetector_const_sptr det;
  // Go through every input workspace pixel
  for (size_t wi=0; wi < inWS->getNumberHistograms(); wi++)
  {
    // We want to skip monitors
    try
    {
      det = inWS->getDetector(wi);
      if( det->isMonitor() ) continue; //skip monitor
      if( det->isMasked() ) continue; //skip masked detectors
    }
    catch(Kernel::Exception::NotFoundError&)
    {
      continue; //skip missing detector
    }

    specid_t inSpec = inWS->getSpectrum(wi)->getSpectrumNo();
    typedef std::map<specid_t, double>  SpectraDistanceMap;

    //Get the number of specified neighbours
    SpectraDistanceMap insideGrid  = inWS->getNeighboursExact(inSpec, nNeighbours, ignoreMaskedDetectors); 
    SpectraDistanceMap::iterator it = insideGrid.begin();
    SpectraDistanceMap neighbSpectra;
    while(it != insideGrid.end())
    {
      //Strip out spectra that don't meet the radius criteria.
      if(it->second <= Radius)
      {
        neighbSpectra.insert(std::make_pair(it->first, it->second));
      }
      it++;
    }
    
    // Force the central pixel to always be there
    // There seems to be a bug in nearestNeighbours, returns distance != 0.0 for the central pixel. So we force distance = 0
    neighbSpectra[inSpec] = 0.0;

    // Neighbours and weights list
    double totalWeight = 0;
    std::vector< weightedNeighbour > neighbours;

    // Convert from spectrum numbers to workspace indices
    for (std::map<specid_t, double>::iterator it = neighbSpectra.begin(); it != neighbSpectra.end(); ++it)
    {
      specid_t spec = it->first;

      //Use the weighting strategy to calculate the weight.
      double weight = WeightedSum->weightAt(it->second); 

      if (weight > 0) 
      {
        // Find the corresponding workspace index
        spec2index_map::iterator mapIt = spec2index->find(spec);
        if (mapIt != spec2index->end())
        {
          size_t neighWI = mapIt->second;
          neighbours.push_back( weightedNeighbour(neighWI, weight) );
          totalWeight += weight;
        }
      }
    } // each neighbour
//    std::cout << "\n";

    // Adjust the weights of each neighbour to normalize to unity
    for (size_t q=0; q<neighbours.size(); q++)
      neighbours[q].second /= totalWeight;

    // Save the list of neighbours for this output workspace index.
    m_neighbours[wi] = neighbours;

    m_prog->report("Finding Neighbours");
  } // each workspace index

  delete spec2index;
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
  }
  else if(strategyName == "Linear")
  {
    boost::scoped_ptr<WeightingStrategy> flatStrategy(new LinearWeighting(cutOff));
    WeightedSum.swap(flatStrategy);
  }
}

/**
Translate the radius into meters.
@param radiusUnits : The name of the radius units
@param enteredUnits : The numerical value of the radius in whatever units have been specified
@param inWS : The input workspace
*/
double SmoothNeighbours::translateToMeters(const std::string radiusUnits, const double& enteredRadius, Mantid::API::MatrixWorkspace_const_sptr ws)
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

    // Get the first available detector
    index2detid_map* map = inWS->getWorkspaceIndexToDetectorIDMap();
    IDetector_const_sptr firstDet = instrument->getDetector(map->at(0));
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


//--------------------------------------------------------------------------------------------
/** Executes the algorithm
 *
 */
void SmoothNeighbours::exec()
{
  // Get the input workspace
  inWS = getProperty("InputWorkspace");

  // Retrieve the optional properties
  double enteredRadius = getProperty("Radius");

  // Use the unit type to translate the entered radius into meters.
  Radius = translateToMeters(getProperty("RadiusUnits"), enteredRadius, inWS);

  std::string strategy  = getProperty("WeightedSum");

  setWeightingStrategy(strategy, Radius);

  AdjX = getProperty("AdjX");
  AdjY = getProperty("AdjY");
  Edge = getProperty("ZeroEdgePixels");
  PreserveEvents = getProperty("PreserveEvents");

  // Progress reporting, first for the sorting
  m_prog = new Progress(this, 0.0, 0.2, inWS->getNumberHistograms());

  // Collect the neighbours with either method.

  if (Radius <= 0.0)
    findNeighboursRectangular();
  else
    findNeighboursUbiqutious();

  // Find the right method to exec
  EventWorkspace_sptr wsEvent = boost::dynamic_pointer_cast<EventWorkspace>(inWS);
  if (wsEvent)
    wsEvent->sortAll(TOF_SORT, m_prog);

  if (!wsEvent || !PreserveEvents)
    this->execWorkspace2D(inWS);
  else if (wsEvent)
    this->execEvent(wsEvent);
  else
    throw std::runtime_error("This algorithm requires a Workspace2D or EventWorkspace as its input.");
}


//--------------------------------------------------------------------------------------------
/** Execute the algorithm for a Workspace2D/don't preserve events input */
void SmoothNeighbours::execWorkspace2D(Mantid::API::MatrixWorkspace_sptr ws)
{
  m_prog->resetNumSteps(inWS->getNumberHistograms(), 0.5, 1.0);

  //Get some stuff from the input workspace
  const size_t numberOfSpectra = inWS->getNumberHistograms();
  
  const size_t YLength = inWS->blocksize();

  MatrixWorkspace_sptr outWS;
  //Make a brand new Workspace2D
  outWS = boost::dynamic_pointer_cast<MatrixWorkspace>( API::WorkspaceFactory::Instance().create("Workspace2D", numberOfSpectra, YLength+1, YLength));
  this->setProperty("OutputWorkspace", outWS);
  //Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(ws, outWS, false);

  // Go through all the output workspace
  PARALLEL_FOR2(ws, outWS)
  for (int outWIi=0; outWIi<int(ws->getNumberHistograms()); outWIi++)
  {
    size_t outWI = size_t(outWIi);
    PARALLEL_START_INTERUPT_REGION

    ISpectrum * outSpec = outWS->getSpectrum(outWI);
    // Reset the Y and E vectors
    outSpec->clearData();
    MantidVec & outY = outSpec->dataY();
    // We will temporarily carry the squared error
    MantidVec & outE = outSpec->dataE();
    // tmp to carry the X Data.
    MantidVec & outX = outSpec->dataX();

    // Which are the neighbours?
    std::vector< weightedNeighbour > & neighbours = m_neighbours[outWI];
    std::vector< weightedNeighbour >::iterator it;
    for (it = neighbours.begin(); it != neighbours.end(); ++it)
    {
      size_t inWI = it->first;
      double weight = it->second;

      const MantidVec & inY = ws->readY(inWI);
      const MantidVec & inE = ws->readE(inWI);
      const MantidVec & inX = ws->readX(inWI);

      for (size_t i=0; i<YLength; i++)
      {
        // Add the weighted signal
        outY[i] += inY[i] * weight;
        // Square the error, scale by weight, then add
        double errorSquared = inE[i];
        errorSquared *= errorSquared;
        errorSquared *= weight;
        outE[i] += errorSquared;
        outX[i] = inX[i];
      }
      if(ws->isHistogramData())
      {
        outX[YLength] = inX[YLength];
      }
    } //(each neighbour)

    // Now un-square the error
    for (size_t i=0; i<YLength; i++)
      outE[i] = sqrt(outE[i]);

    //Copy the single detector ID (of the center) and spectrum number from the input workspace
    outSpec->copyInfoFrom(*ws->getSpectrum(outWI));

    m_prog->report("Summing");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

}




//--------------------------------------------------------------------------------------------
/** Execute the algorithm for a EventWorkspace input
 * @param ws :: EventWorkspace
 */
void SmoothNeighbours::execEvent(Mantid::DataObjects::EventWorkspace_sptr ws)
{
  m_prog->resetNumSteps(inWS->getNumberHistograms(), 0.5, 1.0);

  //Get some stuff from the input workspace
  const size_t numberOfSpectra = inWS->getNumberHistograms();
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
  for (int outWIi=0; outWIi<int(ws->getNumberHistograms()); outWIi++)
  {
    size_t outWI = size_t(outWIi);
    PARALLEL_START_INTERUPT_REGION

    // Create the output event list (empty)
    EventList & outEL = outWS->getOrAddEventList(outWI);

    // Which are the neighbours?
    std::vector< weightedNeighbour > & neighbours = m_neighbours[outWI];
    std::vector< weightedNeighbour >::iterator it;
    for (it = neighbours.begin(); it != neighbours.end(); ++it)
    {
      size_t inWI = it->first;
      double weight = it->second;
      // Copy the event list
      EventList tmpEL = ws->getEventList(inWI);
      // Scale it
      tmpEL *= weight;
      // Add it
      outEL += tmpEL;
    }

    //Copy the single detector ID (of the center) and spectrum number from the input workspace
    outEL.copyInfoFrom(*ws->getSpectrum(outWI));

    m_prog->report("Summing");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  //Finalize the data
  outWS->doneAddingEventLists();

  //Give the 0-th X bins to all the output spectra.
  Kernel::cow_ptr<MantidVec> outX = inWS->refX(0);
  outWS->setAllX( outX );
}



} // namespace Algorithms
} // namespace Mantid
