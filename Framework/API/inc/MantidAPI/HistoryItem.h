// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/WorkspaceHistory.h"

namespace Mantid {
namespace API {

/** @class HistoryView

    This class wraps an algorithm history pointer to add additional
   functionality when creating a HistoryView.

    @author Samuel Jackson, ISIS, RAL
    @date 21/01/2008
    */

class MANTID_API_DLL HistoryItem {
public:
  HistoryItem(AlgorithmHistory_const_sptr algHist);
  virtual ~HistoryItem() = default;
  bool isUnrolled() const { return m_unrolled; }
  void unrolled(bool unrolled) { m_unrolled = unrolled; }
  AlgorithmHistory_const_sptr getAlgorithmHistory() const { return m_algorithmHistory; }
  size_t numberOfChildren() const { return m_algorithmHistory->childHistorySize(); }

private:
  AlgorithmHistory_const_sptr m_algorithmHistory;
  bool m_unrolled;
};

} // namespace API
} // namespace Mantid
