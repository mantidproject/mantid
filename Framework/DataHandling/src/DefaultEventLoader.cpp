#include "MantidDataHandling/DefaultEventLoader.h"

namespace Mantid {
namespace DataHandling {

DefaultEventLoader::DefaultEventLoader(EventWorkspaceCollection &ws,
                                       bool haveWeights, bool event_id_is_spec)
    : m_ws(ws), event_id_is_spec(event_id_is_spec) {
  // This map will be used to find the workspace index
  if (event_id_is_spec)
    pixelID_to_wi_vector =
        m_ws.getSpectrumToWorkspaceIndexVector(pixelID_to_wi_offset);
  else
    pixelID_to_wi_vector =
        m_ws.getDetectorIDToWorkspaceIndexVector(pixelID_to_wi_offset, true);

  // Cache a map for speed.
  if (!haveWeights) {
    makeMapToEventLists(eventVectors);
  } else {
    // Convert to weighted events
    for (size_t i = 0; i < m_ws.getNumberHistograms(); i++) {
      m_ws.getSpectrum(i).switchTo(API::WEIGHTED);
    }
    makeMapToEventLists(weightedEventVectors);
  }
}

} // namespace DataHandling
} // namespace Mantid
