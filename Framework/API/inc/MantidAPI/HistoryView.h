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
#include "MantidAPI/HistoryItem.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/DateAndTime.h"

#include <list>
#include <vector>

namespace Mantid {
namespace API {

/** @class HistoryView

    This class builds a view of the algorithm history by "unrolling" parent
   algorithms.

    @author Samuel Jackson, ISIS, RAL
    @date 21/01/2008
    */

class MANTID_API_DLL HistoryView {
public:
  HistoryView(const WorkspaceHistory &wsHist);
  virtual ~HistoryView() = default;
  void unroll(size_t index);
  void unrollAll();
  void roll(size_t index);
  void rollAll();
  void filterBetweenExecDate(Mantid::Types::Core::DateAndTime start,
                             Mantid::Types::Core::DateAndTime end = Mantid::Types::Core::DateAndTime::getCurrentTime());
  /**
   * Get the list of History Items for this view.
   *
   * @returns vector of history items for this view.
   */
  const std::vector<HistoryItem> &getAlgorithmsList() const { return m_historyItems; };

  size_t size() const { return m_historyItems.size(); }

private:
  void unroll(std::vector<HistoryItem>::iterator &it);
  void roll(std::vector<HistoryItem>::iterator &it);
  void rollChildren(std::vector<HistoryItem>::iterator it);

  const WorkspaceHistory m_wsHist;
  std::vector<HistoryItem> m_historyItems;
};

} // namespace API
} // namespace Mantid
