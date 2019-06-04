// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertToMatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToMatrixWorkspace)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using std::size_t;

void ConvertToMatrixWorkspace::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "An input EventWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "An output Workspace2D.");
}

void ConvertToMatrixWorkspace::exec() {
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  // Let's see if we have to do anything first. Basically we want to avoid the
  // data copy if we can
  DataObjects::EventWorkspace_const_sptr eventW =
      boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(
          inputWorkspace);
  MatrixWorkspace_sptr outputWorkspace;
  if (eventW) {
    g_log.information() << "Converting EventWorkspace to Workspace2D.\n";

    const size_t numHists = inputWorkspace->getNumberHistograms();
    Progress prog(this, 0.0, 1.0, numHists * 2);

    // Sort the input workspace in-place by TOF. This can be faster if there are
    // few event lists.
    eventW->sortAll(TOF_SORT, &prog);

    // Create the output workspace. This will copy many aspects fron the input
    // one.
    outputWorkspace = create<Workspace2D>(*inputWorkspace);

    // ...but not the data, so do that here.
    PARALLEL_FOR_IF(Kernel::threadSafe(*inputWorkspace, *outputWorkspace))
    for (int64_t i = 0; i < static_cast<int64_t>(numHists); ++i) {
      PARALLEL_START_INTERUPT_REGION
      const auto &inSpec = inputWorkspace->getSpectrum(i);
      auto &outSpec = outputWorkspace->getSpectrum(i);

      outSpec.copyInfoFrom(inSpec);
      outSpec.setHistogram(inSpec.histogram());

      prog.report("Binning");

      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  } else {
    outputWorkspace = getProperty("OutputWorkspace");
    if (inputWorkspace == outputWorkspace) {
      g_log.information("InputWorkspace does not need converting. Pointing "
                        "OutputWorkspace property to input.");
      outputWorkspace =
          boost::const_pointer_cast<MatrixWorkspace>(inputWorkspace);
    } else {
      g_log.information(
          "InputWorkspace does not need converting. Cloning InputWorkspace.");
      outputWorkspace = inputWorkspace->clone();
    }
  }

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid
