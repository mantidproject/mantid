// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/Offsetable.h"
#include "MantidHistogramData/Scalable.h"
#include "MantidHistogramData/VectorOf.h"

namespace Mantid {
namespace HistogramData {

class Points;

/** BinEdges

  Container for the bin edges of a histogram. A copy-on-write mechanism saves
  memory and makes copying cheap. The implementation is based on
  detail::VectorOf, a wrapper around a cow_ptr. Mixins such as detail::Iterable
  provide iterators and other operations.

  @author Simon Heybrock
  @date 2016
*/
class MANTID_HISTOGRAMDATA_DLL BinEdges : public detail::VectorOf<BinEdges, HistogramX>,
                                          public detail::Iterable<BinEdges>,
                                          public detail::Offsetable<BinEdges>,
                                          public detail::Scalable<BinEdges> {
public:
  using VectorOf<BinEdges, HistogramX>::VectorOf;
  using VectorOf<BinEdges, HistogramX>::operator=;
  /// Default constructor, creates a NULL object.
  BinEdges() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  BinEdges(const BinEdges &) = default;
  /// Move constructor.
  BinEdges(BinEdges &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  BinEdges &operator=(const BinEdges &) & = default;
  /// Move assignment.
  BinEdges &operator=(BinEdges &&) & = default;

  explicit BinEdges(const Points &points);
};

} // namespace HistogramData
} // namespace Mantid
