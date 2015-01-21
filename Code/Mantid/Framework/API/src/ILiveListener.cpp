#include "MantidAPI/ILiveListener.h"

namespace Mantid {
namespace API {

ILiveListener::ILiveListener() : m_dataReset(false) {}

ILiveListener::~ILiveListener() {}

bool ILiveListener::dataReset() {
  const bool retval = m_dataReset;
  m_dataReset = false; // Should this be done here or should extractData do it?
  return retval;
}

} // namespace API
} // namespace Mantid
