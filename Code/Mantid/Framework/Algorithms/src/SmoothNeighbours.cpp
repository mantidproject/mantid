//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SmoothNeighbours.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <boost/algorithm/string.hpp>

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SmoothNeighbours)

/// Sets documentation strings for this algorithm
void SmoothNeighbours::initDocs()
{
  this->setWikiSummary("Sum event lists from neighboring pixels in rectangular area detectors - e.g. to reduce the signal-to-noise of individual spectra. Each spectrum in the output workspace is a sum of a block of AdjX*AdjY pixels. Only works on EventWorkspaces and for instruments with RectangularDetector's. ");
  this->setOptionalMessage("Sum event lists from neighboring pixels in rectangular area detectors - e.g. to reduce the signal-to-noise of individual spectra. Each spectrum in the output workspace is a sum of a block of AdjX*AdjY pixels. Only works on EventWorkspaces and for instruments with RectangularDetector's.");
}


using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;

/** Initialisation method.
 *
 */
void SmoothNeighbours::init()
{
  declareProperty(
    new WorkspaceProperty<>("InputWorkspace","",Direction::Input, new EventWorkspaceValidator<>),
                            "The workspace containing the spectra to be averaged." );
  declareProperty(
    new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm." );

  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(1);

  declareProperty("AdjX", 1, mustBePositive,
    "The number of X (horizontal) adjacent pixels to average together. " );

  declareProperty("AdjY", 1, mustBePositive->clone(),
    "The number of Y (vertical) adjacent pixels to average together. " );


}

/** Executes the algorithm
 *
 */
void SmoothNeighbours::exec()
{
  // Try and retrieve the optional properties
  AdjX = getProperty("AdjX");
  AdjY = getProperty("AdjY");

  // Get the input workspace
  MatrixWorkspace_const_sptr matrixInWS = getProperty("InputWorkspace");
  EventWorkspace_const_sptr inWS = boost::dynamic_pointer_cast<const EventWorkspace>( matrixInWS );
  if (!inWS)
    throw std::invalid_argument("InputWorkspace should be an EventWorkspace.");

  //Get some stuff from the input workspace
  //const size_t numberOfSpectra = inWS->getNumberHistograms();
  const int YLength = static_cast<int>(inWS->blocksize());
  IInstrument_sptr inst = inWS->getInstrument();
  if (!inst)
    throw std::runtime_error("The InputWorkspace does not have a valid instrument attached to it!");

  API::MatrixWorkspace_sptr matrixOutputWS = this->getProperty("OutputWorkspace");
  EventWorkspace_sptr outWS;
  //Make a brand new EventWorkspace
  outWS = boost::dynamic_pointer_cast<EventWorkspace>( API::WorkspaceFactory::Instance().create("EventWorkspace", 1, YLength, 1));
  //Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(inWS, outWS, false);

  //Cast to the matrixOutputWS and save it
  matrixOutputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outWS);
  this->setProperty("OutputWorkspace", matrixOutputWS);

//  //Split the detector names string.
//  std::vector<std::string> det_names;
//  boost::split(det_names, det_name_list, boost::is_any_of(", "));

  //To get the workspace index from the detector ID
  detid2index_map * pixel_to_wi = inWS->getDetectorIDToWorkspaceIndexMap(true);

  int outWI = 0;
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

  //Loop through the RectangularDetector's we listed before.
  for (int i=0; i < static_cast<int>(detList.size()); i++)
  {
    std::string det_name("");
    boost::shared_ptr<RectangularDetector> det;
    det = detList[i];
    for (int j=0; j < det->xpixels(); j++)
      for (int k=0; k < det->ypixels(); k++)
      {
        if (det)
        {
          det_name = det->getName();

          int count = 0;
          //Initialize the output event list
          EventList outEL;
          for (int ix=-AdjX; ix <= AdjX; ix++)
            for (int iy=-AdjY; iy <= AdjY; iy++)
            {
              //Cut corners
              if(std::abs(ix) == AdjX && std::abs(iy) == AdjY)continue;
              //Find the pixel ID at that XY position on the rectangular detector
              if(j+ix >= det->xpixels() || j+ix < 0)continue;
              if(k+iy >= det->ypixels() || k+iy < 0)continue;
              int pixelID = det->getAtXY(j+ix,k+iy)->getID();

              //Find the corresponding workspace index, if any
              if (pixel_to_wi->find(pixelID) != pixel_to_wi->end())
              {
                size_t wi = (*pixel_to_wi)[pixelID];
                //Get the event list on the input and add it
                outEL += inWS->getEventList(wi);
                count++;

              }
            }
            //Add all these events to the (empty) list returned by this
            outEL /= count;
            outWS->getOrAddEventList(outWI) += outEL;
            outWI++;
        }
      }
    prog.report(det_name);
  }

  //Finalize the data
  outWS->doneAddingEventLists();

  //Give the 0-th X bins to all the output spectra.
  Kernel::cow_ptr<MantidVec> outX = inWS->refX(0);
  outWS->setAllX( outX );


  //Clean up memory
  delete pixel_to_wi;

}

} // namespace Algorithms
} // namespace Mantid
