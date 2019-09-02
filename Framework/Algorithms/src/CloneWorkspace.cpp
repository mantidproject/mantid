// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CloneWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CloneWorkspace)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

void CloneWorkspace::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("InputWorkspace", "",
                                                     Direction::Input),
      "Name of the input workspace. Must be a MatrixWorkspace (2D or "
      "EventWorkspace), a PeaksWorkspace or a MDEventWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the newly created cloned workspace.");
}

void CloneWorkspace::exec() {
  Workspace_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr inputMatrix =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(inputWorkspace);
  IMDWorkspace_sptr inputMD =
      boost::dynamic_pointer_cast<IMDWorkspace>(inputWorkspace);
  ITableWorkspace_const_sptr iTableWS =
      boost::dynamic_pointer_cast<const ITableWorkspace>(inputWorkspace);

  if (inputMatrix || iTableWS) {
    // Workspace::clone() is polymorphic, we can use the same for all types
    Workspace_sptr outputWS(inputWorkspace->clone());
    setProperty("OutputWorkspace", outputWS);
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
  } else
    throw std::runtime_error("Expected a MatrixWorkspace, PeaksWorkspace, "
                             "MDEventWorkspace, or a MDHistoWorkspace. Cannot "
                             "clone this type of workspace.");
}

} // namespace Algorithms
} // namespace Mantid
