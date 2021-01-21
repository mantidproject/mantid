// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Batch/RowPredicate.h"
#include "MantidQtWidgets/Common/Batch/StrictQModelIndices.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

bool RowPredicate::operator()(RowLocation const &rowLocation) const { return rowMeetsCriteria(rowLocation); }
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
