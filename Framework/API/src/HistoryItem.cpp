//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/HistoryItem.h"

namespace Mantid {
namespace API {

HistoryItem::HistoryItem(AlgorithmHistory_const_sptr algHist)
    : m_algorithmHistory(std::move(algHist)), m_unrolled(false) {}

} // namespace API
} // namespace Mantid
