// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ChangeBinOffset.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/EventWorkspaceAccess.h"
#include "MantidAlgorithms/MatrixWorkspaceAccess.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid::Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(ChangeBinOffset)

void ChangeBinOffset::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Name of the input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace");
  auto isDouble = std::make_shared<BoundedValidator<double>>();
  declareProperty("Offset", 0.0, isDouble, "The amount to adjust the time bins. Usually in microseconds");
  declareWorkspaceIndexSetProperties();
}

void ChangeBinOffset::exec() {
  MatrixWorkspace_const_sptr inputW = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputW = getProperty("OutputWorkspace");
  if (outputW != inputW) {
    outputW = inputW->clone();
    setProperty("OutputWorkspace", outputW);
  }

  const double offset = getProperty("Offset");
  EventWorkspace_sptr eventWS = std::dynamic_pointer_cast<EventWorkspace>(outputW);
  if (eventWS) {
    this->for_each<Indices::FromProperty>(*eventWS, std::make_tuple(EventWorkspaceAccess::eventList),
                                          [offset](EventList &eventList) { eventList.addTof(offset); });
  } else {
    this->for_each<Indices::FromProperty>(
        *outputW, std::make_tuple(MatrixWorkspaceAccess::x), [offset](std::vector<double> &dataX) {
          std::transform(dataX.begin(), dataX.end(), dataX.begin(), [offset](double x) { return x + offset; });
        });
  }
}

} // namespace Mantid::Algorithms
