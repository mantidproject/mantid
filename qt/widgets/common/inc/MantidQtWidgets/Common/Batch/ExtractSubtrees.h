// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#pragma once
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "MantidQtWidgets/Common/Batch/Row.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/Subtree.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <boost/optional.hpp>
#include <string>
#include <vector>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON ExtractSubtrees {
public:
  using RandomAccessRowIterator = std::vector<Row>::iterator;
  using RandomAccessConstRowIterator = std::vector<Row>::const_iterator;
  boost::optional<std::vector<Subtree>> operator()(std::vector<Row> region) const;

private:
  RandomAccessConstRowIterator findEndOfSubtree(RandomAccessConstRowIterator subtreeBegin,
                                                RandomAccessConstRowIterator regionEnd, int subtreeRootDepth) const;

  Subtree makeSubtreeFromRows(RowLocation subtreeRootLocation, RandomAccessConstRowIterator subtreeBegin,
                              RandomAccessConstRowIterator subtreeEnd) const;

  std::vector<Subtree> makeSubtreesFromRows(std::vector<Row> const &rows, int subtreeRootDepth) const;
};

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
