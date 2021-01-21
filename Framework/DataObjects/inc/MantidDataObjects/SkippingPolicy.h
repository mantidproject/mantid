// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IMDIterator.h"
#include "MantidDataObjects/SkippingPolicy.h"
#include "MantidKernel/System.h"
#include <boost/scoped_ptr.hpp>

namespace Mantid {
namespace DataObjects {

/** SkippingPolicy : Policy types for skipping in MDiterators.

  @date 2012-03-05
*/

class DLLExport SkippingPolicy {
public:
  virtual bool keepGoing() const = 0;
  virtual ~SkippingPolicy() = default;
};

/// Policy that indicates skipping of masked bins.
class DLLExport SkipMaskedBins : public SkippingPolicy {
private:
  Mantid::API::IMDIterator *m_iterator;

public:
  SkipMaskedBins(Mantid::API::IMDIterator *const iterator) : m_iterator(iterator) {}
  /**
  Keep going as long as the current iterator bin is masked.
  @return True to keep going.
  */
  bool keepGoing() const override { return m_iterator->getIsMasked(); };
};

/// Policy that indicates no skipping should be applied.
class DLLExport SkipNothing : public SkippingPolicy {
public:
  /**
  Always returns false to cancel skipping.
  @return false to cancel continuation
  */
  bool keepGoing() const override { return false; }
};

using SkippingPolicy_scptr = boost::scoped_ptr<SkippingPolicy>;

} // namespace DataObjects
} // namespace Mantid
