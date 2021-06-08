// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ConvToMDEventsWSIndexing.h"

namespace Mantid {
namespace MDAlgorithms {

size_t ConvToMDEventsWSIndexing::initialize(const MDWSDescription &WSD, std::shared_ptr<MDEventWSWrapper> inWSWrapper,
                                            bool ignoreZeros) {
  size_t numSpec = ConvToMDEventsWS::initialize(WSD, inWSWrapper, ignoreZeros);

  // check if split parameters are valid
  auto &split_into = m_OutWSWrapper->pWorkspace()->getBoxController()->getSplitIntoAll();

  bool validSplitInfo = isSplitValid(split_into);
  if (!validSplitInfo) {
    std::string arg;
    for (auto &i : split_into)
      arg += std::to_string(i) + " ";
    throw std::invalid_argument("SplitInto can't be [" + arg + "]" +
                                " ,all splits have to be the same and equal the power of 2.");
  }
  return numSpec;
}

template <>
void ConvToMDEventsWSIndexing::appendEventsFromInputWS<2>(API::Progress *pProgress, const API::BoxController_sptr &bc) {
  if (m_OutWSWrapper->nDimensions() == 2)
    appendEvents<2>(pProgress, bc);
}

void ConvToMDEventsWSIndexing::appendEventsFromInputWS(API::Progress *pProgress, const API::BoxController_sptr &bc) {
  appendEventsFromInputWS<8>(pProgress, bc);
}
} // namespace MDAlgorithms
} // namespace Mantid
