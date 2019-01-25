#include "MantidMDAlgorithms/ConvToMDEventsWSIndexing.h"

namespace Mantid {
namespace MDAlgorithms {
#if BOOST_VERSION <= 106100
void ConvToMDEventsWSIndexing::appendEventsFromInputWS(
    API::Progress *, const API::BoxController_sptr &) {}
#else
template <>
void ConvToMDEventsWSIndexing::appendEventsFromInputWS<2>(
    API::Progress *pProgress, const API::BoxController_sptr &bc) {
  if (m_OutWSWrapper->nDimensions() == 2)
    appendEvents<2>(pProgress, bc);
}

void ConvToMDEventsWSIndexing::appendEventsFromInputWS(
    API::Progress *pProgress, const API::BoxController_sptr &bc) {
  appendEventsFromInputWS<8>(pProgress, bc);
}
#endif // BOOST_VERSION <= 106100
} // namespace MDAlgorithms
} // namespace Mantid
