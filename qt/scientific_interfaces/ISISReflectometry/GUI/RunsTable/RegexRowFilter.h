// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"
#include "MantidQtWidgets/Common/Batch/RowPredicate.h"
#include "Reduction/ReductionJobs.h"
#include "Reduction/RowLocation.h"
#include <boost/regex.hpp>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class RegexFilter : public MantidQt::MantidWidgets::Batch::RowPredicate {
public:
  RegexFilter(const boost::regex &regex, MantidQt::MantidWidgets::Batch::IJobTreeView const &view,
              ReductionJobs const &jobs);
  bool rowMeetsCriteria(MantidQt::MantidWidgets::Batch::RowLocation const &row) const override;

private:
  static auto constexpr RUNS_COLUMN_INDEX = 0;
  boost::regex m_filter;
  MantidQt::MantidWidgets::Batch::IJobTreeView const &m_view;
  ReductionJobs const &m_jobs;
};

std::unique_ptr<RegexFilter> filterFromRegexString(std::string const &regex,
                                                   MantidQt::MantidWidgets::Batch::IJobTreeView const &view,
                                                   ReductionJobs const &jobs);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
