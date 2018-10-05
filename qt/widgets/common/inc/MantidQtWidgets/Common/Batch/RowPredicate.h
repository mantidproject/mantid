// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#ifndef MANTIDQTMANTIDWIDGETS_ROWPREDICATE_H_
#define MANTIDQTMANTIDWIDGETS_ROWPREDICATE_H_
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include <memory>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {
class RowPredicate {
public:
  bool operator()(RowLocation const &row) const;
  virtual ~RowPredicate() = default;

protected:
  virtual bool rowMeetsCriteria(RowLocation const &row) const = 0;
};
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQTMANTIDWIDGETS_ROWPREDICATE_H_
