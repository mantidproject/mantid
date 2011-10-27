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
#include "MantidKernel/EnabledWhenProperty.h"
#include <boost/algorithm/string.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SmoothNeighbours)

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


  BoundedValidator<double> *mustBePositiveDouble = new BoundedValidator<double>();
  mustBePositiveDouble->setLower(0.0);
  declareProperty("Radius", 0.0, mustBePositiveDouble,
    "The radius around a pixel to look for nearest neighbours to average. \n"
    "If 0, will use the AdjX and AdjY parameters for rectangular detectors instead." );

  declareProperty("WeightedSum", true,
    "Adjust the weight of neighboring pixels when summing them, based on their distance.");

  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);

  declareProperty("AdjX", 1, mustBePositive,
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
              // Default to unity weight
              double smweight = 1.0;

              //Weights for corners=1; higher for center and adjacent pixels
              if (WeightedSum)
                smweight = static_cast<double>(AdjX - std::abs(ix) + AdjY - std::abs(iy) + 1);

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
void SmoothNeighbours::findNeighboursRadius()
{
  m_prog->resetNumSteps(inWS->getNumberHistograms(), 0.2, 0.5);
  this->progress(0.2, "Building Neighbour Map");

  Instrument_const_sptr inst = inWS->getInstrument();
  spec2index_map * spec2index = inWS->getSpectrumToWorkspaceIndexMap();

  // Resize the vector we are setting
  m_neighbours.resize(inWS->getNumberHistograms());

  // Go through every input workspace pixel
  for (size_t wi=0; wi < inWS->getNumberHistograms(); wi++)
  {
    specid_t inSpec = inWS->getSpectrum(wi)->getSpectrumNo();
    std::map<specid_t, double> neighbSpectra = inWS->getNeighbours(inSpec, Radius);

    // Force the central pixel to always be there
    // There seems to be a bug in nearestNeighbours, returns distance != 0.0 for the central pixel. So we force distance = 0
    neighbSpectra[inSpec] = 0.0;

    // Neighbours and weights list
    double totalWeight = 0;
    std::vector< weightedNeighbour > neighbours;
//    std::cout << neighbSpectra.size() << " neighbours to " << wi << ": ";

    // Convert from spectrum numbers to workspace indices
    for (std::map<specid_t, double>::iterator it = neighbSpectra.begin(); it != neighbSpectra.end(); ++it)
    {
      specid_t spec = it->first;
      double r = it->second;
//      std::cout << spec << " at " << r << ",";

      // Default weight of 1, or do a linear weighting scheme with center = 1.0 and edges = 0.0
      double weight = 1.0;
      if (WeightedSum)
        weight = 1.0 - (r / Radius);

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


//--------------------------------------------------------------------------------------------
/** Executes the algorithm
 *
 */
void SmoothNeighbours::exec()
{
  // Retrieve the optional properties
  Radius = getProperty("Radius");
  WeightedSum = getProperty("WeightedSum");
  AdjX = getProperty("AdjX");
  AdjY = getProperty("AdjY");
  Edge = getProperty("ZeroEdgePixels");
  PreserveEvents = getProperty("PreserveEvents");

  // Get the input workspace
  inWS = getProperty("InputWorkspace");

  // Progress reporting, first for the sorting
  m_prog = new Progress(this, 0.0, 0.2, inWS->getNumberHistograms());

  // Collect the neighbours with either method.
  if (Radius <= 0.0)
    findNeighboursRectangular();
  else
    findNeighboursRadius();

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
  for (size_t outWI=0; outWI<ws->getNumberHistograms(); outWI++)
  {
    PARALLEL_START_INTERUPT_REGION

    ISpectrum * outSpec = outWS->getSpectrum(outWI);
    // Reset the Y and E vectors
    outSpec->clearData();
    MantidVec & outY = outSpec->dataY();
    // We will temporarily carry the squared error
    MantidVec & outE = outSpec->dataE();

    // Which are the neighbours?
    std::vector< weightedNeighbour > & neighbours = m_neighbours[outWI];
    std::vector< weightedNeighbour >::iterator it;
    for (it = neighbours.begin(); it != neighbours.end(); ++it)
    {
      size_t inWI = it->first;
      double weight = it->second;

      const MantidVec & inY = ws->readY(inWI);
      const MantidVec & inE = ws->readE(inWI);

      for (size_t i=0; i<YLength; i++)
      {
        // Add the weighted signal
        outY[i] += inY[i] * weight;
        // Square the error, scale by weight, then add
        double errorSquared = inE[i];
        errorSquared *= errorSquared;
        errorSquared *= weight;
        outE[i] += errorSquared;
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
  for (size_t outWI=0; outWI<ws->getNumberHistograms(); outWI++)
  {
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
