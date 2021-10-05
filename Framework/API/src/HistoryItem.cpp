// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <utility>

#include "MantidAPI/HistoryItem.h"

namespace Mantid::API {

HistoryItem::HistoryItem(AlgorithmHistory_const_sptr algHist)
    : m_algorithmHistory(std::move(algHist)), m_unrolled(false) {}

} // namespace Mantid::API
