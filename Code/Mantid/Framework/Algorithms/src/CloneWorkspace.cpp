#include "MantidAlgorithms/CloneWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CloneWorkspace)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

void CloneWorkspace::init() {
  declareProperty(
      new WorkspaceProperty<Workspace>("InputWorkspace", "", Direction::Input),
      "Name of the input workspace. Must be a MatrixWorkspace (2D or "
      "EventWorkspace), a PeaksWorkspace or a MDEventWorkspace.");
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Name of the newly created cloned workspace.");
}

void CloneWorkspace::exec() {
  Workspace_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr inputMatrix =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(inputWorkspace);
  EventWorkspace_const_sptr inputEvent =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWorkspace);
  IMDWorkspace_sptr inputMD =
      boost::dynamic_pointer_cast<IMDWorkspace>(inputWorkspace);
  PeaksWorkspace_const_sptr inputPeaks =
      boost::dynamic_pointer_cast<const PeaksWorkspace>(inputWorkspace);
  TableWorkspace_const_sptr tableWS =
      boost::dynamic_pointer_cast<const TableWorkspace>(inputWorkspace);

  if (inputEvent) {
    // Handle an EventWorkspace as the input.
    // Make a brand new EventWorkspace
    EventWorkspace_sptr outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create(
            "EventWorkspace", inputEvent->getNumberHistograms(), 2, 1));

    // Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputEvent, outputWS,
                                                           false);

    // You need to copy over the data as well.
    outputWS->copyDataFrom((*inputEvent));

    // Cast to the matrixOutputWS and save it
    setProperty("OutputWorkspace",
                boost::dynamic_pointer_cast<Workspace>(outputWS));
  } else if (inputMatrix) {
    // Create the output workspace. This will copy many aspects fron the input
    // one.
    MatrixWorkspace_sptr outputWorkspace =
        WorkspaceFactory::Instance().create(inputMatrix);

    // ...but not the data, so do that here.

    const int64_t numHists =
        static_cast<int64_t>(inputMatrix->getNumberHistograms());
    Progress prog(this, 0.0, 1.0, numHists);

    PARALLEL_FOR2(inputMatrix, outputWorkspace)
    for (int64_t i = 0; i < int64_t(numHists); ++i) {
      PARALLEL_START_INTERUPT_REGION

      outputWorkspace->setX(i, inputMatrix->refX(i));
      outputWorkspace->dataY(i) = inputMatrix->readY(i);
      outputWorkspace->dataE(i) = inputMatrix->readE(i);
      // Be sure to not break sharing between Dx vectors by assigning pointers
      outputWorkspace->getSpectrum(i)
          ->setDx(inputMatrix->getSpectrum(i)->ptrDx());

      prog.report();

      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
    setProperty("OutputWorkspace",
                boost::dynamic_pointer_cast<Workspace>(outputWorkspace));
  } else if (inputMD) {
    // Call the CloneMDWorkspace algo to handle MDEventWorkspace
    IAlgorithm_sptr alg =
        this->createChildAlgorithm("CloneMDWorkspace", 0.0, 1.0, true);
    alg->setProperty("InputWorkspace", inputMD);
    alg->setPropertyValue("OutputWorkspace",
                          getPropertyValue("OutputWorkspace"));
    alg->executeAsChildAlg();
    IMDWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");
    setProperty("OutputWorkspace",
                boost::dynamic_pointer_cast<Workspace>(outputWS));
  } else if (inputPeaks) {
    PeaksWorkspace_sptr outputWS(inputPeaks->clone());
    setProperty("OutputWorkspace",
                boost::dynamic_pointer_cast<Workspace>(outputWS));
  } else if (tableWS) {
    TableWorkspace_sptr outputWS(tableWS->clone());
    setProperty("OutputWorkspace",
                boost::dynamic_pointer_cast<Workspace>(outputWS));
  } else
    throw std::runtime_error("Expected a MatrixWorkspace, PeaksWorkspace, "
                             "MDEventWorkspace, or a MDHistoWorkspace. Cannot "
                             "clone this type of workspace.");
}

} // namespace Algorithms
} // namespace Mantid
