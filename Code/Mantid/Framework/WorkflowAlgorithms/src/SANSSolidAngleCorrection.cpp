/*WIKI* 

Performs a solid angle correction on all detector (non-monitor) spectra. 

See [http://www.mantidproject.org/Reduction_for_HFIR_SANS SANS Reduction] documentation for details.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/SANSSolidAngleCorrection.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/IDetector.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SANSSolidAngleCorrection)

/// Sets documentation strings for this algorithm
void SANSSolidAngleCorrection::initDocs()
{
  this->setWikiSummary("Performs solid angle correction on SANS 2D data.");
  this->setOptionalMessage("Performs solid angle correction on SANS 2D data.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void SANSSolidAngleCorrection::init()
{
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  declareProperty("OutputMessage","",Direction::Output);
  declareProperty("ReductionProperties","__sans_reduction_properties", Direction::Input);
}

void SANSSolidAngleCorrection::exec()
{
  // Reduction property manager
   const std::string reductionManagerName = getProperty("ReductionProperties");
   boost::shared_ptr<PropertyManager> reductionManager;
   if (PropertyManagerDataService::Instance().doesExist(reductionManagerName))
   {
     reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
   }
   else
   {
     reductionManager = boost::make_shared<PropertyManager>();
     PropertyManagerDataService::Instance().addOrReplace(reductionManagerName, reductionManager);
   }

   // If the solid angle algorithm isn't in the reduction properties, add it
   if (!reductionManager->existsProperty("SolidAngleAlgorithm"))
   {
     AlgorithmProperty *algProp = new AlgorithmProperty("SolidAngleAlgorithm");
     algProp->setValue(toString());
     reductionManager->declareProperty(algProp);
   }

  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  DataObjects::EventWorkspace_const_sptr inputEventWS = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (inputEventWS)
    return execEvent();

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if ( outputWS != inputWS )
  {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    outputWS->isDistribution(true);
    outputWS->setYUnit("");
    outputWS->setYUnitLabel("Steradian");
    setProperty("OutputWorkspace",outputWS);
  }

  const int numHists = static_cast<int>(inputWS->getNumberHistograms());
  Progress progress(this,0.0,1.0,numHists);

  // Number of X bins
  const int xLength = static_cast<int>(inputWS->readY(0).size());

  PARALLEL_FOR2(outputWS, inputWS)
  for (int i = 0; i < numHists; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    outputWS->dataX(i) = inputWS->readX(i);

    IDetector_const_sptr det;
    try {
      det = inputWS->getDetector(i);
    } catch (Exception::NotFoundError&) {
      g_log.warning() << "Spectrum index " << i << " has no detector assigned to it - discarding" << std::endl;
      // Catch if no detector. Next line tests whether this happened - test placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a catch
      // in an openmp block.
    }
    // If no detector found, skip onto the next spectrum
    if( !det ) continue;

    // Skip if we have a monitor or if the detector is masked.
    if ( det->isMonitor() || det->isMasked() ) continue;

    const MantidVec& YIn = inputWS->readY(i);
    const MantidVec& EIn = inputWS->readE(i);

    MantidVec& YOut = outputWS->dataY(i);
    MantidVec& EOut = outputWS->dataE(i);

    // Compute solid angle correction factor
    const double tanTheta = tan( inputWS->detectorTwoTheta(det) );
    const double term = sqrt(tanTheta*tanTheta + 1.0);
    const double corr = term*term*term;

    // Correct data for all X bins
    for (int j = 0; j < xLength; j++)
    {
      YOut[j] = YIn[j]*corr;
      EOut[j] = fabs(EIn[j]*corr);
    }
    progress.report("Solid Angle Correction");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  setProperty("OutputMessage", "Solid angle correction applied");
}

void SANSSolidAngleCorrection::execEvent()
{
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  EventWorkspace_sptr inputEventWS = boost::dynamic_pointer_cast<EventWorkspace>(inputWS);

  const int numberOfSpectra = static_cast<int>(inputEventWS->getNumberHistograms());
  Progress progress(this,0.0,1.0,inputEventWS->getNumberHistograms());

  // generate the output workspace pointer
  MatrixWorkspace_sptr outputWS = this->getProperty("OutputWorkspace");
  EventWorkspace_sptr outputEventWS;
  if (outputWS == inputWS)
    outputEventWS = boost::dynamic_pointer_cast<EventWorkspace>(outputWS);
  else
  {
    //Make a brand new EventWorkspace
    outputEventWS = boost::dynamic_pointer_cast<EventWorkspace>(
        WorkspaceFactory::Instance().create("EventWorkspace", inputEventWS->getNumberHistograms(), 2, 1));
    //Copy geometry over.
    WorkspaceFactory::Instance().initializeFromParent(inputEventWS, outputEventWS, false);
    //You need to copy over the data as well.
    outputEventWS->copyDataFrom( (*inputEventWS) );

    //Cast to the matrixOutputWS and save it
    outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputEventWS);
    this->setProperty("OutputWorkspace", outputWS);
  }

  progress.report("Solid Angle Correction");

  PARALLEL_FOR2(inputEventWS, outputEventWS)
  for (int i = 0; i < numberOfSpectra; i++)
  {
    PARALLEL_START_INTERUPT_REGION
    IDetector_const_sptr det;
    try {
      det = inputEventWS->getDetector(i);
    } catch (Exception::NotFoundError&) {
      g_log.warning() << "Spectrum index " << i << " has no detector assigned to it - discarding" << std::endl;
      // Catch if no detector. Next line tests whether this happened - test placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a catch
      // in an openmp block.
    }
    if( !det ) continue;

    // Skip if we have a monitor or if the detector is masked.
    if ( det->isMonitor() || det->isMasked() ) continue;

    // Compute solid angle correction factor
    const double tanTheta = tan( inputEventWS->detectorTwoTheta(det) );
    const double term = sqrt(tanTheta*tanTheta + 1.0);
    const double corr = term*term*term;
    EventList& el = outputEventWS->getEventList(i);
    el*=corr;
    progress.report("Solid Angle Correction");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputMessage", "Solid angle correction applied");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

