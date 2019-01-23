// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/HistoryItem.h"

namespace Mantid {
namespace API {

HistoryItem::HistoryItem(AlgorithmHistory_const_sptr algHist)
    : m_algorithmHistory(algHist), m_unrolled(false) {}

} // namespace API
} // namespace Mantid
