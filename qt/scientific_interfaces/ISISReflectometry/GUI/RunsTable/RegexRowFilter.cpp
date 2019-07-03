// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RegexRowFilter.h"

namespace MantidQt {
namespace CustomInterfaces {

using MantidQt::MantidWidgets::Batch::IJobTreeView;
using MantidQt::MantidWidgets::Batch::RowLocation;

RegexFilter::RegexFilter(boost::regex regex, IJobTreeView const &view,
                         ReductionJobs const &jobs)
    : m_filter(std::move(regex)), m_view(view), m_jobs(jobs) {}

bool RegexFilter::rowMeetsCriteria(RowLocation const &location) const {
  if (location.isRoot()) {
    return true;
  } else if (isGroupLocation(location)) {
    auto cellText = m_view.cellAt(location, RUNS_COLUMN_INDEX).contentText();
    return boost::regex_search(cellText, m_filter);
  } else {
    assert(isRowLocation(location));
    auto cellText = m_view.cellAt(location, RUNS_COLUMN_INDEX).contentText();
    auto groupText = groupName(m_jobs, groupOf(location));
    return boost::regex_search(cellText, m_filter) ||
           boost::regex_search(groupText, m_filter);
  }
}

std::unique_ptr<RegexFilter> filterFromRegexString(std::string const &regex,
                                                   IJobTreeView const &view,
                                                   ReductionJobs const &jobs) {
  return std::make_unique<RegexFilter>(boost::regex(regex), view, jobs);
}
} // namespace CustomInterfaces
} // namespace MantidQt
