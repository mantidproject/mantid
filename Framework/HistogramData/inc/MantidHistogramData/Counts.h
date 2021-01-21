// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/Addable.h"
#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/Offsetable.h"
#include "MantidHistogramData/Scalable.h"
#include "MantidHistogramData/VectorOf.h"

namespace Mantid {
namespace HistogramData {
class BinEdges;
class Frequencies;

/** Counts

  Container for the counts in a histogram. A copy-on-write mechanism saves
  memory and makes copying cheap. The implementation is based on
  detail::VectorOf, a wrapper around a cow_ptr. Mixins such as detail::Iterable
  provide iterators and other operations.

  @author Simon Heybrock
  @date 2016
*/
class MANTID_HISTOGRAMDATA_DLL Counts : public detail::VectorOf<Counts, HistogramY>,
                                        public detail::Addable<Counts>,
                                        public detail::Iterable<Counts>,
                                        public detail::Offsetable<Counts>,
                                        public detail::Scalable<Counts> {
public:
  using VectorOf<Counts, HistogramY>::VectorOf;
  using VectorOf<Counts, HistogramY>::operator=;
  // Multiple inheritance causes ambiguous overload, bring operators into scope.
  using detail::Addable<Counts>::operator+;
  using detail::Addable<Counts>::operator+=;
  using detail::Addable<Counts>::operator-;
  using detail::Addable<Counts>::operator-=;
  using detail::Offsetable<Counts>::operator+;
  using detail::Offsetable<Counts>::operator+=;
  using detail::Offsetable<Counts>::operator-;
  using detail::Offsetable<Counts>::operator-=;
  /// Default constructor, creates a NULL object.
  Counts() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  Counts(const Counts &) = default;
  /// Move constructor.
  Counts(Counts &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  Counts &operator=(const Counts &) & = default;
  /// Move assignment.
  Counts &operator=(Counts &&) & = default;

  Counts(const Frequencies &frequencies, const BinEdges &edges);
  Counts(Frequencies &&frequencies, const BinEdges &edges);
};

} // namespace HistogramData
} // namespace Mantid
