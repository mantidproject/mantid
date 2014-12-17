//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/HistoryItem.h"

namespace Mantid {
namespace API {

HistoryItem::HistoryItem(AlgorithmHistory_const_sptr algHist)
    : m_algorithmHistory(algHist), m_unrolled(false) {}

HistoryItem::HistoryItem(const HistoryItem &A)
    : m_algorithmHistory(A.m_algorithmHistory), m_unrolled(A.m_unrolled) {}

HistoryItem &HistoryItem::operator=(const HistoryItem &A) {
  if (&A != this) {
    m_algorithmHistory = A.m_algorithmHistory;
    m_unrolled = A.m_unrolled;
  }

  return *this;
}

} // namespace API
} // namespace Mantid
