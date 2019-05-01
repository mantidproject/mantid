// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ChangeBinOffset.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/EventWorkspaceAccess.h"
#include "MantidAlgorithms/MatrixWorkspaceAccess.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(ChangeBinOffset)

void ChangeBinOffset::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the input workspace");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace");
  auto isDouble = boost::make_shared<BoundedValidator<double>>();
  declareProperty("Offset", 0.0, isDouble,
                  "The amount to change each time bin by");

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
  EventWorkspace_sptr eventWS =
      boost::dynamic_pointer_cast<EventWorkspace>(outputW);
  if (eventWS) {
    this->for_each<Indices::FromProperty>(
        *eventWS, std::make_tuple(EventWorkspaceAccess::eventList),
        [offset](EventList &eventList) { eventList.addTof(offset); });
  } else {
    this->for_each<Indices::FromProperty>(
        *outputW, std::make_tuple(MatrixWorkspaceAccess::x),
        [offset](std::vector<double> &dataX) {
          for (auto &x : dataX)
            x += offset;
        });
  }
}

} // namespace Algorithms
} // namespace Mantid
