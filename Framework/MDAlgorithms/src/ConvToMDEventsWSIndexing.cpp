#include "MantidMDAlgorithms/ConvToMDEventsWSIndexing.h"

namespace Mantid {
namespace MDAlgorithms {

template <>
void ConvToMDEventsWSIndexing::appendEventsFromInputWS<2>(API::Progress *pProgress, const API::BoxController_sptr &bc) {
  if(m_OutWSWrapper->nDimensions() == 2)
    appendEvents<2>(pProgress, bc);
}

void ConvToMDEventsWSIndexing::appendEventsFromInputWS(API::Progress *pProgress, const API::BoxController_sptr &bc) {
  appendEventsFromInputWS<8>(pProgress, bc);
}

} // namespace MDAlgorithms
} // namespace Mantid
