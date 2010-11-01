//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SumNeighbours.h"
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
DECLARE_ALGORITHM(SumNeighbours)

using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;

/** Initialisation method.
 *
 */
void SumNeighbours::init()
{
  declareProperty(
    new WorkspaceProperty<>("InputWorkspace","",Direction::Input, new EventWorkspaceValidator<>),
                            "The workspace containing the spectra to be summed." );
  declareProperty(
    new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm." );

  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(1);

  declareProperty("SumX", 4, mustBePositive,
    "The number of X (horizontal) pixels to sum together. This must evenly divide the number of X pixels in a detector." );

  declareProperty("SumY", 4, mustBePositive->clone(),
    "The number of Y (vertical) pixels to sum together. This must evenly divide the number of Y pixels in a detector" );

  //declareProperty("DetectorNames", "", "Comma-separated list of the names of the detectors in the Mantid geometry file." );


}

/** Executes the algorithm
 *
 */
void SumNeighbours::exec()
{
  // Try and retrieve the optional properties
  SumX = getProperty("SumX");
  SumY = getProperty("SumY");

//  if ((XPixels % SumX != 0) || (YPixels % SumY != 0))
//    throw std::invalid_argument("SumX must evenly divide XPixels and SumY must evenly divide YPixels for SumNeighbours to work.");

  // Get the input workspace
  MatrixWorkspace_const_sptr matrixInWS = getProperty("InputWorkspace");
  EventWorkspace_const_sptr inWS = boost::dynamic_pointer_cast<const EventWorkspace>( matrixInWS );
  if (!inWS)
    throw std::invalid_argument("InputWorkspace should be an EventWorkspace.");

  //Get some stuff from the input workspace
  //const int numberOfSpectra = inWS->getNumberHistograms();
  const int YLength = inWS->blocksize();
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
//  std::string det_name_list = getPropertyValue("DetectorNames");
//  boost::split(det_names, det_name_list, boost::is_any_of(", "));

  //To get the workspace index from the detector ID
  IndexToIndexMap * pixel_to_wi = inWS->getDetectorIDToWorkspaceIndexMap(true);

  int outWI = 0;
  //std::cout << " inst->nelements() " << inst->nelements() << "\n";
  Progress prog(this,0.0,1.0,inst->nelements());

  //Build a list of Rectangular Detectors
  std::vector<boost::shared_ptr<IRectangularDetector> > detList;
  for (int i=0; i < inst->nelements(); i++)
  {
    boost::shared_ptr<IRectangularDetector> det;
    boost::shared_ptr<ICompAssembly> assem;

    det = boost::dynamic_pointer_cast<IRectangularDetector>( (*inst)[i] );
    if (det)
      detList.push_back(det);
    else
    {
      //Also, look in the first sub-level for RectangularDetectors (e.g. PG3).
      // We are not doing a full recursive search since that will be very long for lots of pixels.
      assem = boost::dynamic_pointer_cast<ICompAssembly>( (*inst)[i] );
      if (assem)
      {
        for (int j=0; j < assem->nelements(); j++)
        {
          det = boost::dynamic_pointer_cast<IRectangularDetector>( (*assem)[j] );
          if (det) detList.push_back(det);
        }
      }

    }
  }

  if (detList.size() == 0)
    throw std::runtime_error("This instrument does not have any RectangularDetector's. SumNeighbors cannot operate on this instrument at this time.");

  //Loop through the RectangularDetector's we listed before.
  for (int i=0; i < static_cast<int>(detList.size()); i++)
  {
    std::string det_name("");
    boost::shared_ptr<IRectangularDetector> det;
    det = detList[i];
    if (det)
    {
      int x, y;
      det_name = det->getName();
      //TODO: Check validity of the parameters

      //det->getAtXY()
      for (x=0; x<det->xpixels(); x += SumX)
        for (y=0; y<det->ypixels(); y += SumY)
        {
          //Initialize the output event list
          EventList outEL;
//          Kernel::cow_ptr<MantidVec> outX;
//          bool outX_set = false;
          for (int ix=0; ix < SumX; ix++)
            for (int iy=0; iy < SumY; iy++)
            {
              //Find the pixel ID at that XY position on the rectangular detector
              int pixelID = det->getAtXY(x+ix,y+iy)->getID();

              //Find the corresponding workspace index, if any
              if (pixel_to_wi->find(pixelID) != pixel_to_wi->end())
              {
                int wi = (*pixel_to_wi)[pixelID];
                //Get the event list on the input and add it
                outEL += inWS->getEventList(wi);

//                if (!outX_set)
//                {
//                  //We'll keep the first X axis
//                  outX = inWS->refX(wi);
//                  outX_set = true;
//                }
              }
            }

          //Add all these events to the (empty) list returned by this
          outWS->getOrAddEventList(outWI) += outEL;

//          //Set the X binning, if any was found.
//          if (outX_set)
//            outWS->setX(outWI, outX);
//          else
//            outWS->setX(outWI, inWS->refX(0)); //get the 0th x-axis if nothing was set.

          outWI++;

          //std::cout << pixelID << ", ";
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
