/**
Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#ifndef MANTID_CUSTOMINTERFACES_ROWFILTER_H_
#define MANTID_CUSTOMINTERFACES_ROWFILTER_H_

#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"
#include "MantidQtWidgets/Common/Batch/RowPredicate.h"
#include "Reduction/ReductionJobs.h"
#include "RowLocation.h"
#include <boost/regex.hpp>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class RegexFilter : public MantidQt::MantidWidgets::Batch::RowPredicate {
public:
  RegexFilter(boost::regex regex,
              MantidQt::MantidWidgets::Batch::IJobTreeView const &view,
              Jobs const &jobs);
  bool rowMeetsCriteria(
      MantidQt::MantidWidgets::Batch::RowLocation const &row) const override;

private:
  static auto constexpr RUNS_COLUMN_INDEX = 0;
  boost::regex m_filter;
  MantidQt::MantidWidgets::Batch::IJobTreeView const &m_view;
  Jobs const &m_jobs;
};

std::unique_ptr<RegexFilter>
filterFromRegexString(std::string const &regex,
                      MantidQt::MantidWidgets::Batch::IJobTreeView const &view,
                      Jobs const &jobs);
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_ROWFILTER_H_
