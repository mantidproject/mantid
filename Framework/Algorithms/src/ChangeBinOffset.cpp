#include "MantidAlgorithms/ChangeBinOffset.h"
#include "MantidAPI/MatrixWorkspace.h"
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
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                                         Direction::Input),
                  "Name of the input workspace");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "Name of the output workspace");
  auto isDouble = boost::make_shared<BoundedValidator<double>>();
  declareProperty("Offset", 0.0, isDouble,
                  "The amount to change each time bin by");

  declareSpectrumIndexSetProperties();
}

void ChangeBinOffset::exec() {
  const MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputW = getProperty("OutputWorkspace");
  if (outputW != inputW) {
    outputW = MatrixWorkspace_sptr(inputW->clone().release());
    setProperty("OutputWorkspace", outputW);
  }

  double offset = getProperty("Offset");
  EventWorkspace_sptr eventWS =
      boost::dynamic_pointer_cast<EventWorkspace>(outputW);
  if (eventWS) {
    this->for_each(*eventWS, std::make_tuple(Getters::eventList),
                   [=](EventList &eventList) { eventList.addTof(offset); });
  } else {
    this->for_each(*outputW, std::make_tuple(Getters::x),
                   [=](std::vector<double> &dataX) {
                     for (auto &x : dataX)
                       x += offset;
                   });
  }
}

} // namespace Algorithm
} // namespace Mantid
